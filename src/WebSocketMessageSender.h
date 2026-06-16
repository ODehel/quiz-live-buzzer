#ifndef WEB_SOCKET_MESSAGE_SENDER_H
#define WEB_SOCKET_MESSAGE_SENDER_H

#include "HubMessageSender.h"
#include "MessageSerializer.h"

class WebSocketsClient;

class WebSocketMessageSender : public HubMessageSender
{
private:
    MessageSerializer messageSerializer;
    WebSocketsClient &client;
public:
    WebSocketMessageSender(MessageSerializer messageSerializer, WebSocketsClient &client) : messageSerializer(messageSerializer), client(client)
    {
    }
    
    void SendAnswer(char value) override;
    void SendBuzz() override;
};

#endif