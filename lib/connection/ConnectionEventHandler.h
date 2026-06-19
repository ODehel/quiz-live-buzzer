#ifndef CONNECTION_EVENT_HANDLER_H
#define CONNECTION_EVENT_HANDLER_H

#include <string>
#include "ReconnectionPolicy.h"
#include "HubMessageDispatcher.h"
#include "SessionMessageSender.h"
#include "MessageDeserializer.h"
#include "TokenReceiver.h"

class ConnectionEventHandler
{
private:
    ReconnectionPolicy &reconnectionPolicy;
    HubMessageDispatcher &hubMessageDispatcher;
    SessionMessageSender &sessionMessageSender;
    TokenReceiver &tokenReceiver;
    MessageDeserializer messageDeserializer;

public:
    ConnectionEventHandler(ReconnectionPolicy &reconnectionPolicy, HubMessageDispatcher &hubMessageDispatcher, SessionMessageSender &sessionMessageSender, TokenReceiver &tokenReceiver)
        : reconnectionPolicy(reconnectionPolicy), hubMessageDispatcher(hubMessageDispatcher), sessionMessageSender(sessionMessageSender), tokenReceiver(tokenReceiver)
    {
    }

    void OnConnectionLost()
    {
        reconnectionPolicy.OnReconnectFailed();
    }

    void OnMessageReceived(const std::string &message)
    {
        std::string type = messageDeserializer.ExtractType(message);
        if (type == "auth_success")
        {
            reconnectionPolicy.OnAuthSuccess();
            tokenReceiver.UpdateToken(messageDeserializer.ExtractToken(message));
        }
        else if (type == "token_expiring_soon")
            sessionMessageSender.SendAuthRefresh();
        else
            hubMessageDispatcher.Dispatch(type);
    }
};

#endif