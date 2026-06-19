#ifndef WEB_SOCKET_PAYLOAD_SOCKET_WRITER_H
#define WEB_SOCKET_PAYLOAD_SOCKET_WRITER_H

#include "PayloadSocketWriter.h"

class WebSocketsClient;

class WebSocketPayloadSocketWriter : public PayloadSocketWriter
{
private:
    WebSocketsClient &client;

public:
    WebSocketPayloadSocketWriter(WebSocketsClient &client) : client(client)
    {
    }

    void Write(const std::string &payload) override;
};

#endif