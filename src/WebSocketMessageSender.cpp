#include "WebSocketMessageSender.h"
#include "WebSocketsClient.h"

void WebSocketMessageSender::SendAnswer(char value)
{
    client.sendTXT(messageSerializer.SerializeAnswer(value).c_str());
}

void WebSocketMessageSender::SendBuzz()
{
    client.sendTXT(messageSerializer.SerializeBuzz().c_str());
}