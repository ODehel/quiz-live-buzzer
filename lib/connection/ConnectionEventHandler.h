#ifndef CONNECTION_EVENT_HANDLER_H
#define CONNECTION_EVENT_HANDLER_H

#include <string>
#include "ReconnectionPolicy.h"
#include "HubMessageDispatcher.h"
#include "SessionMessageSender.h"
#include "MessageDeserializer.h"
#include "ConnectionEventListener.h"
#include "MessageRenderer.h"

class ConnectionEventHandler : public ConnectionEventListener
{
private:
    ReconnectionPolicy &reconnectionPolicy;
    HubMessageDispatcher &hubMessageDispatcher;
    SessionMessageSender &sessionMessageSender;
    MessageDeserializer messageDeserializer;
    MessageRenderer &messageRenderer;

public:
    ConnectionEventHandler(ReconnectionPolicy &reconnectionPolicy, HubMessageDispatcher &hubMessageDispatcher, SessionMessageSender &sessionMessageSender, MessageRenderer &messageRenderer)
        : reconnectionPolicy(reconnectionPolicy), hubMessageDispatcher(hubMessageDispatcher), sessionMessageSender(sessionMessageSender), messageRenderer(messageRenderer)
    {
    }

    void OnConnectionEstablished() override
    {
        sessionMessageSender.SendAuth();
    }

    void OnConnectionLost() override
    {
        reconnectionPolicy.OnReconnectFailed();
    }

    void OnMessageReceived(const std::string &message) override
    {
        std::string type = messageDeserializer.ExtractType(message);
        messageRenderer.Apply(type);
        if (type == "auth_success")
        {
            reconnectionPolicy.OnAuthSuccess();
        }
        else if (type == "token_expiring_soon")
            sessionMessageSender.SendAuthRefresh();
        else
            hubMessageDispatcher.Dispatch(type);
    }
};

#endif