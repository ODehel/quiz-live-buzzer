#ifndef WEB_SOCKET_CONNECTION_OPENER_H
#define WEB_SOCKET_CONNECTION_OPENER_H

#include <cstdint>
#include "ConnectionOpener.h"

class WebSocketsClient;

class WebSocketConnectionOpener : public ConnectionOpener
{
private:
    WebSocketsClient &client;
    const char *host;
    uint16_t port;
    const char *path;

public:
    WebSocketConnectionOpener(WebSocketsClient &client, const char *host, uint16_t port, const char *path)
        : client(client), host(host), port(port), path(path)
    {
    }

    void SetReconnectInterval(unsigned long milliseconds) override;
    void Open() override;
};

#endif
