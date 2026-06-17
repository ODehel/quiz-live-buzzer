#ifndef CONNECTION_EVENT_HANDLER_H
#define CONNECTION_EVENT_HANDLER_H

#include <string>
#include "ReconnectionPolicy.h"
#include "HubMessageDispatcher.h"
#include "MessageDeserializer.h"

class ConnectionEventHandler
{
private:
    ReconnectionPolicy &reconnectionPolicy;
    HubMessageDispatcher &hubMessageDispatcher;
    MessageDeserializer messageDeserializer;

public:
    ConnectionEventHandler(ReconnectionPolicy &reconnectionPolicy, HubMessageDispatcher &hubMessageDispatcher)
        : reconnectionPolicy(reconnectionPolicy), hubMessageDispatcher(hubMessageDispatcher)
    {
    }

    void OnConnectionLost()
    {
        reconnectionPolicy.OnReconnectFailed();
    }

    void OnMessageReceived(const std::string &message)
    {
        hubMessageDispatcher.Dispatch(messageDeserializer.ExtractType(message));
    }
};

#endif