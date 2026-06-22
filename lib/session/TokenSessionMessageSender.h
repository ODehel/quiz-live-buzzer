#ifndef TOKEN_SESSION_MESSAGE_SENDER_H
#define TOKEN_SESSION_MESSAGE_SENDER_H

#include <string>
#include "SessionMessageSender.h"
#include "PayloadSocketWriter.h"
#include "MessageSerializer.h"
#include "TokenReceiver.h"

class TokenSessionMessageSender : public SessionMessageSender, public TokenReceiver
{
private:
    PayloadSocketWriter &payloadSocketWriter;
    MessageSerializer messageSerializer;
    std::string token;

public:
    TokenSessionMessageSender(PayloadSocketWriter &payloadSocketWriter, MessageSerializer messageSerializer)
        : payloadSocketWriter(payloadSocketWriter), messageSerializer(messageSerializer)
    {
    }

    void SendAuthRefresh() override
    {
        payloadSocketWriter.Write(messageSerializer.SerializeAuthRefresh(token));
    }

    void SendAuth() override
    {
        payloadSocketWriter.Write(messageSerializer.SerializeAuth(token));
    }

    void UpdateToken(const std::string &token) override
    {
        this->token = token;
    }
};

#endif