#ifndef BUZZER_EVENT_TRANSLATOR_H
#define BUZZER_EVENT_TRANSLATOR_H

#include <string>
#include "BuzzerEventType.h"
#include "ConnectionEventListener.h"

struct BuzzerEvent
{
    BuzzerEventType type;
    std::string payload;
};

class BuzzerEventTranslator
{
private:
    ConnectionEventListener &connectionEventListener;

public:
    // La raison que la bibliothèque WebSockets (2.7.3) donne à la fermeture d'un client qui n'a jamais ouvert sa connexion : elle n'émet rien au
    // connect raté lui-même, mais nettoie le client au tour suivant et le dit ainsi. Cette fermeture est l'échec de la tentative, une seule fois.
    static constexpr const char *NeverOpenedDisconnectionReason = "TCP connection cleanup";

    BuzzerEventTranslator(ConnectionEventListener &connectionEventListener) : connectionEventListener(connectionEventListener)
    {
    }

    void TranslateBuzzerEvent(BuzzerEvent buzzerEvent)
    {
        switch (buzzerEvent.type)
        {
        case BuzzerEventType::Connected:
            connectionEventListener.OnConnectionEstablished();
            break;
        case BuzzerEventType::TextReceived:
            connectionEventListener.OnMessageReceived(buzzerEvent.payload);
            break;
        case BuzzerEventType::Disconnected:
            if (buzzerEvent.payload == NeverOpenedDisconnectionReason)
                connectionEventListener.OnConnectionAttemptFailed();
            else
                connectionEventListener.OnConnectionLost();
            break;
        case BuzzerEventType::ConnectionAttemptFailed:
            connectionEventListener.OnConnectionAttemptFailed();
            break;
        }
    }
};

#endif