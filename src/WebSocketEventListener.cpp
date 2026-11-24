#include "WebSocketEventListener.h"
#include "WebSocketsClient.h"

void WebSocketEventListener::Begin()
{
    client.onEvent([this](WStype_t type, uint8_t *payload, size_t length)
                   {
        switch (type)
        {
        case WStype_CONNECTED:
            buzzerEventTranslator.TranslateBuzzerEvent({BuzzerEventType::Connected, ""});
            break;
        case WStype_TEXT:
            buzzerEventTranslator.TranslateBuzzerEvent({BuzzerEventType::TextReceived, std::string(reinterpret_cast<char *>(payload), length)});
            break;
        case WStype_DISCONNECTED:
            // La raison de la fermeture (absente à une fermeture sans motif) distingue la perte d'une connexion établie de l'échec d'une tentative.
            buzzerEventTranslator.TranslateBuzzerEvent({BuzzerEventType::Disconnected, payload ? std::string(reinterpret_cast<char *>(payload), length) : std::string()});
            break;
        default:
            break;
        } });
}

void WebSocketEventListener::Loop()
{
    client.loop();
}