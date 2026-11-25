#ifndef TOKEN_SESSION_MESSAGE_SENDER_H
#define TOKEN_SESSION_MESSAGE_SENDER_H

#include <string>
#include "SessionMessageSender.h"
#include "PayloadSocketWriter.h"
#include "MessageSerializer.h"
#include "TokenFetcher.h"
#include "TokenUpdater.h"
#include "LoginRefusalListener.h"

class TokenSessionMessageSender : public SessionMessageSender, public TokenUpdater
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

    // Un login frais, puis `auth_refresh` avec CE token (US-FW-007 CA-2). Échec du login, `200` sans token ou `401` : rien n'est envoyé, rien ne relance
    // et rien ne s'arrête (CA-3, CA-4) ; contrairement à `SendAuth`, pas de repli sur le dernier token, qui n'aurait pas d'autre `exp`.
    // Retourne vrai si l'`auth_refresh` est parti : le hub y répondra par un `auth_success`.
    bool SendAuthRefresh() override
    {
        LoginResult fresh = tokenFetcher.Fetch();
        if (fresh.status != LoginStatus::Success || fresh.token.empty())
            return false;
        token = fresh.token;
        payloadSocketWriter.Write(messageSerializer.SerializeAuthRefresh(token));
        return true;
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

    void UpdateToken(const std::string &token) override
    {
        this->token = token;
    }
};

#endif