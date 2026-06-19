#ifndef WEB_SOCKET_EVENT_LISTENER_H
#define WEB_SOCKET_EVENT_LISTENER_H

#include "BuzzerEventTranslator.h"

class WebSocketsClient;

class WebSocketEventListener
{
private:
    WebSocketsClient &client;
    BuzzerEventTranslator &buzzerEventTranslator;

public:
    WebSocketEventListener(WebSocketsClient &client, BuzzerEventTranslator &buzzerEventTranslator)
        : client(client), buzzerEventTranslator(buzzerEventTranslator)
    {
    }

    void Begin();
    void Loop();
};

#endif