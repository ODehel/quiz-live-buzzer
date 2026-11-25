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
#include "LinkStateDisplay.h"

class ConnectionEventHandler : public ConnectionEventListener
{
private:
    ReconnectionPolicy &reconnectionPolicy;
    HubMessageDispatcher &hubMessageDispatcher;
    SessionMessageSender &sessionMessageSender;
    MessageDeserializer messageDeserializer;
    MessageRenderer &messageRenderer;
    OutOfGameDisplay &outOfGameDisplay;
    LinkStateDisplay &linkStateDisplay;
    NetworkHalt &networkHalt;
    // Les `auth_refresh` envoyés sur la connexion en cours dont l'`auth_success` n'est pas encore revenu (US-FW-007 CA-6). Un compteur et non
    // un drapeau : une rafale de `token_expiring_soon` (CA-5) envoie un `auth_refresh` par message, donc reçoit autant d'`auth_success`.
    // Remis à 0 à l'ouverture d'une connexion : la réponse d'une connexion fermée ne viendra jamais, et l'`auth_success` suivant une ouverture s'affiche.
    int pendingAuthRefreshReplies = 0;

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

    // La liaison est coupée dès la perte de la connexion établie et le reste jusqu'à l'`auth_success` suivant (US-FW-015 CA-1, CA-4) :
    // les appuis sont ignorés et l'écran de coupure affiche la tentative en cours. Rien avant le premier `auth_success` (CA-6), ni une fois
    // le réseau arrêté : l'élimination arrête le réseau (« Hors jeu » est terminal, jamais « 4/3 »), tout comme le refus du login.
    void ShowLinkLostIfAny()
    {
        if (!reconnectionPolicy.IsArmed() || networkHalt.IsHalted())
            return;
        hubMessageDispatcher.OnLinkLost();
        linkStateDisplay.ShowLinkLost(reconnectionPolicy.CurrentAttempt());
    }

public:
    ConnectionEventHandler(ReconnectionPolicy &reconnectionPolicy, HubMessageDispatcher &hubMessageDispatcher, SessionMessageSender &sessionMessageSender, MessageRenderer &messageRenderer, OutOfGameDisplay &outOfGameDisplay, LinkStateDisplay &linkStateDisplay, NetworkHalt &networkHalt)
        : reconnectionPolicy(reconnectionPolicy), hubMessageDispatcher(hubMessageDispatcher), sessionMessageSender(sessionMessageSender), messageRenderer(messageRenderer), outOfGameDisplay(outOfGameDisplay), linkStateDisplay(linkStateDisplay), networkHalt(networkHalt)
    {
    }

    void OnConnectionEstablished() override
    {
        pendingAuthRefreshReplies = 0;
        sessionMessageSender.SendAuth();
    }

    void OnConnectionLost() override
    {
        // Fermer le client émet une déconnexion : une fermeture voulue n'est ni un échec ni une élimination (CA-11).
        if (networkHalt.IsHalted())
            return;
        reconnectionPolicy.OnConnectionLost();
        PropagateEliminationIfAny();
        ShowLinkLostIfAny();
    }

    void OnConnectionAttemptFailed() override
    {
        reconnectionPolicy.OnReconnectFailed();
        PropagateEliminationIfAny();
        ShowLinkLostIfAny();
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
        // La réponse à un `auth_refresh` ne change ni l'écran, ni le bandeau, ni le mode : seul le compteur d'élimination repart à 0 (CA-6).
        if (type == "auth_success" && pendingAuthRefreshReplies > 0)
        {
            pendingAuthRefreshReplies--;
            reconnectionPolicy.OnAuthSuccess();
            return;
        }
        messageRenderer.Apply(hubMessage);
        if (type == "auth_success")
        {
            reconnectionPolicy.OnAuthSuccess();
            hubMessageDispatcher.OnLinkEstablished();
        }
        else if (type == "token_expiring_soon")
        {
            if (sessionMessageSender.SendAuthRefresh())
                pendingAuthRefreshReplies++;
        }
        else
            hubMessageDispatcher.Dispatch(type);
    }
};

#endif