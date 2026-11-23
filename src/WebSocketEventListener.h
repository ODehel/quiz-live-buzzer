#ifndef WEB_SOCKET_EVENT_LISTENER_H
#define WEB_SOCKET_EVENT_LISTENER_H

#include "BuzzerEventTranslator.h"

class ObservableWebSocketsClient;

class WebSocketEventListener
{
private:
    ObservableWebSocketsClient &client;
    BuzzerEventTranslator &buzzerEventTranslator;

public:
    WebSocketEventListener(ObservableWebSocketsClient &client, BuzzerEventTranslator &buzzerEventTranslator)
        : client(client), buzzerEventTranslator(buzzerEventTranslator)
    {
    }

    void Begin();
    void Loop();
};

#endif