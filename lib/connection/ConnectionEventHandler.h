#ifndef CONNECTION_EVENT_HANDLER_H
#define CONNECTION_EVENT_HANDLER_H

#include "ReconnectionPolicy.h"

class ConnectionEventHandler
{
private:
    ReconnectionPolicy &reconnectionPolicy;
public:
    ConnectionEventHandler(ReconnectionPolicy &reconnectionPolicy) : reconnectionPolicy(reconnectionPolicy)
    {
    }

    void OnConnectionLost()
    {
        reconnectionPolicy.OnReconnectFailed();
    }
};

#endif