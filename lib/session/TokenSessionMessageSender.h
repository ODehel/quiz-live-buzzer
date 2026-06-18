#ifndef TOKEN_SESSION_MESSAGE_SENDER_H
#define TOKEN_SESSION_MESSAGE_SENDER_H

#include <string>
#include "SessionMessageSender.h"
#include "PayloadSocketWriter.h"
#include "MessageSerializer.h"

class TokenSessionMessageSender : public SessionMessageSender
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

    void UpdateToken(const std::string &token)
    {
        this->token = token;
    }
};

#endif