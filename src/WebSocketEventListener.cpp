#include "WebSocketEventListener.h"
#include "ObservableWebSocketsClient.h"

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
            buzzerEventTranslator.TranslateBuzzerEvent({BuzzerEventType::Disconnected, ""});
            break;
        default:
            break;
        } });
}

void WebSocketEventListener::Loop()
{
    if (client.LoopAndTellIfTheConnectFailed())
        buzzerEventTranslator.TranslateBuzzerEvent({BuzzerEventType::ConnectionAttemptFailed, ""});
}