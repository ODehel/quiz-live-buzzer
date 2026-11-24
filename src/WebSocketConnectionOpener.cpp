#include "WebSocketConnectionOpener.h"
#include "WebSocketsClient.h"

void WebSocketConnectionOpener::SetReconnectInterval(unsigned long milliseconds)
{
    client.setReconnectInterval(milliseconds);
}

void WebSocketConnectionOpener::Open()
{
    client.begin(host, port, path);
}
