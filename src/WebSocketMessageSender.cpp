#include "WebSocketMessageSender.h"
#include "WebSocketsClient.h"

bool WebSocketMessageSender::SendAnswer(char value)
{
    return client.sendTXT(messageSerializer.SerializeAnswer(value).c_str());
}

bool WebSocketMessageSender::SendBuzz()
{
    return client.sendTXT(messageSerializer.SerializeBuzz().c_str());
}