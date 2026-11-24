#ifndef CONNECTION_EVENT_HANDLER_H
#define CONNECTION_EVENT_HANDLER_H

#include <optional>
#include <string>
#include "ReconnectionPolicy.h"
#include "HubMessageDispatcher.h"
#include "SessionMessageSender.h"
#include "MessageDeserializer.h"
#include "ConnectionEventListener.h"
#include "MessageRenderer.h"
#include "NetworkHalt.h"
#include "OutOfGameDisplay.h"

class ConnectionEventHandler : public ConnectionEventListener
{
private:
    ReconnectionPolicy &reconnectionPolicy;
    HubMessageDispatcher &hubMessageDispatcher;
    SessionMessageSender &sessionMessageSender;
    MessageDeserializer messageDeserializer;
    MessageRenderer &messageRenderer;
    OutOfGameDisplay &outOfGameDisplay;
    NetworkHalt &networkHalt;

    void PropagateEliminationIfAny()
    {
        if (!reconnectionPolicy.IsEliminated())
            return;
        // Un réseau déjà arrêté (refus du login, US-FW-006 CA-10) garde son écran : « Hors jeu » ne le remplace pas, et ne s'affiche qu'une fois.
        if (!networkHalt.IsHalted())
            outOfGameDisplay.ShowOutOfGame();
        hubMessageDispatcher.OnEliminated();
        networkHalt.Halt();
    }

public:
    ConnectionEventHandler(ReconnectionPolicy &reconnectionPolicy, HubMessageDispatcher &hubMessageDispatcher, SessionMessageSender &sessionMessageSender, MessageRenderer &messageRenderer, OutOfGameDisplay &outOfGameDisplay, NetworkHalt &networkHalt)
        : reconnectionPolicy(reconnectionPolicy), hubMessageDispatcher(hubMessageDispatcher), sessionMessageSender(sessionMessageSender), messageRenderer(messageRenderer), outOfGameDisplay(outOfGameDisplay), networkHalt(networkHalt)
    {
    }

    void OnConnectionEstablished() override
    {
        sessionMessageSender.SendAuth();
    }

    void OnConnectionLost() override
    {
        // Fermer le client émet une déconnexion : une fermeture voulue n'est ni un échec ni une élimination (CA-11).
        if (networkHalt.IsHalted())
            return;
        reconnectionPolicy.OnConnectionLost();
        PropagateEliminationIfAny();
    }

    void OnConnectionAttemptFailed() override
    {
        reconnectionPolicy.OnReconnectFailed();
        PropagateEliminationIfAny();
    }

    void OnMessageReceived(const std::string &message) override
    {
        if (reconnectionPolicy.IsEliminated())
            return;

        std::optional<HubMessage> parsed = messageDeserializer.Parse(message);
        if (!parsed)
            return;

        HubMessage hubMessage = *parsed;
        const std::string &type = hubMessage.Type();
        messageRenderer.Apply(hubMessage);
        if (type == "auth_success")
        {
            reconnectionPolicy.OnAuthSuccess();
        }
        else if (type == "token_expiring_soon")
            sessionMessageSender.SendAuthRefresh();
        else
            hubMessageDispatcher.Dispatch(type);
    }
};

#endif