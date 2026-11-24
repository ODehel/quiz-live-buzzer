#ifndef WEB_SOCKET_CONNECTION_CLOSER_H
#define WEB_SOCKET_CONNECTION_CLOSER_H

#include "ConnectionCloser.h"

class WebSocketsClient;

class WebSocketConnectionCloser : public ConnectionCloser
{
private:
    WebSocketsClient &client;

public:
    WebSocketConnectionCloser(WebSocketsClient &client) : client(client)
    {
    }

    void Close() override;
};

#endif
