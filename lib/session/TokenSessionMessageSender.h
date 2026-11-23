#ifndef TOKEN_SESSION_MESSAGE_SENDER_H
#define TOKEN_SESSION_MESSAGE_SENDER_H

#include <string>
#include "SessionMessageSender.h"
#include "PayloadSocketWriter.h"
#include "MessageSerializer.h"
#include "TokenFetcher.h"
#include "LoginRefusalListener.h"

class TokenSessionMessageSender : public SessionMessageSender
{
private:
    PayloadSocketWriter &payloadSocketWriter;
    MessageSerializer messageSerializer;
    TokenFetcher &tokenFetcher;
    LoginRefusalListener &loginRefusalListener;
    std::string token;

public:
    TokenSessionMessageSender(PayloadSocketWriter &payloadSocketWriter, MessageSerializer messageSerializer, TokenFetcher &tokenFetcher, LoginRefusalListener &loginRefusalListener)
        : payloadSocketWriter(payloadSocketWriter), messageSerializer(messageSerializer), tokenFetcher(tokenFetcher), loginRefusalListener(loginRefusalListener)
    {
    }

    void SendAuthRefresh() override
    {
        payloadSocketWriter.Write(messageSerializer.SerializeAuthRefresh(token));
    }

    void SendAuth() override
    {
        LoginResult fresh = tokenFetcher.Fetch();
        if (fresh.status == LoginStatus::InvalidCredentials)
        {
            loginRefusalListener.OnLoginRefused();
            return;
        }
        if (fresh.status == LoginStatus::Success && !fresh.token.empty())
            token = fresh.token;
        payloadSocketWriter.Write(messageSerializer.SerializeAuth(token));
    }

    void UpdateToken(const std::string &token)
    {
        this->token = token;
    }
};

#endif