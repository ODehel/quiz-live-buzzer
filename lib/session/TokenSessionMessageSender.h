#ifndef TOKEN_SESSION_MESSAGE_SENDER_H
#define TOKEN_SESSION_MESSAGE_SENDER_H

#include <optional>
#include <string>
#include "SessionMessageSender.h"
#include "PayloadSocketWriter.h"
#include "MessageSerializer.h"
#include "AsyncLogin.h"
#include "Clock.h"
#include "TokenUpdater.h"
#include "LoginRefusalListener.h"

// Les logins en jeu, sans bloquer la réception (US-FW-005 CA-9, CA-12 ; US-FW-007 CA-2, CA-8, CA-9) : au plus un login en vol à la fois,
// d'ouverture ou de renouvellement, borné à `loginTimeoutMilliseconds` depuis son lancement. `PollLogin()` en recueille la fin.
class TokenSessionMessageSender : public SessionMessageSender, public TokenUpdater
{
private:
    enum class LoginPurpose
    {
        Opening,
        Renewal
    };

    PayloadSocketWriter &payloadSocketWriter;
    MessageSerializer messageSerializer;
    AsyncLogin &asyncLogin;
    Clock &clock;
    const unsigned long loginTimeoutMilliseconds;
    LoginRefusalListener &loginRefusalListener;
    std::string token;
    std::optional<LoginPurpose> loginInFlight;
    unsigned long loginStartedAt = 0;
    // La connexion qui a demandé le login en vol est encore ouverte. Une nouvelle connexion abandonne le login en vol, donc il n'y a toujours
    // qu'une connexion demandeuse : celle qui est ouverte, ou la dernière fermée.
    bool connectionOpen = false;

    void StartLogin(LoginPurpose purpose)
    {
        asyncLogin.Start();
        loginStartedAt = clock.NowMilliseconds();
        loginInFlight = purpose;
    }

    // Le token frais est retenu comme dernier token connu même si la connexion est fermée (US-FW-005 CA-9, US-FW-007 CA-9). Fermée, rien n'est envoyé,
    // pas même le refus d'un `401` (le login de la connexion suivante le recevra). Retourne vrai si un `auth_refresh` est parti.
    bool Complete(LoginPurpose purpose, const LoginResult &result)
    {
        bool hasFreshToken = result.status == LoginStatus::Success && !result.token.empty();
        if (hasFreshToken)
            token = result.token;
        if (!connectionOpen)
            return false;
        if (purpose == LoginPurpose::Opening)
        {
            if (result.status == LoginStatus::InvalidCredentials)
            {
                loginRefusalListener.OnLoginRefused();
                return false;
            }
            payloadSocketWriter.Write(messageSerializer.SerializeAuth(token));
            return false;
        }
        // Échec, `200` sans token, `401` ou délai dépassé : aucun `auth_refresh`, rien ne relance et rien ne s'arrête (CA-3, CA-4). Pas de repli sur
        // le dernier token, contrairement à `auth` : un ancien token ne renouvellerait rien.
        if (!hasFreshToken)
            return false;
        payloadSocketWriter.Write(messageSerializer.SerializeAuthRefresh(token));
        return true;
    }

public:
    TokenSessionMessageSender(PayloadSocketWriter &payloadSocketWriter, MessageSerializer messageSerializer, AsyncLogin &asyncLogin, Clock &clock, unsigned long loginTimeoutMilliseconds, LoginRefusalListener &loginRefusalListener)
        : payloadSocketWriter(payloadSocketWriter), messageSerializer(messageSerializer), asyncLogin(asyncLogin), clock(clock), loginTimeoutMilliseconds(loginTimeoutMilliseconds), loginRefusalListener(loginRefusalListener)
    {
    }

    void SendAuth() override
    {
        if (loginInFlight)
            asyncLogin.Abandon();
        connectionOpen = true;
        StartLogin(LoginPurpose::Opening);
    }

    // Un login de renouvellement : `auth_refresh` partira avec CE token frais, jamais l'ancien (US-FW-007 CA-2). Ignoré si un login, d'ouverture ou
    // de renouvellement, est déjà en vol : il rend déjà un token frais (CA-8).
    void SendAuthRefresh() override
    {
        if (loginInFlight)
            return;
        StartLogin(LoginPurpose::Renewal);
    }

    bool PollLogin() override
    {
        if (!loginInFlight)
            return false;
        std::optional<LoginResult> result = asyncLogin.Poll();
        if (!result)
        {
            if (clock.NowMilliseconds() - loginStartedAt < loginTimeoutMilliseconds)
                return false;
            asyncLogin.Abandon();
            result = LoginResult{LoginStatus::Failed, ""};
        }
        LoginPurpose purpose = *loginInFlight;
        loginInFlight.reset();
        return Complete(purpose, *result);
    }

    void OnConnectionClosed() override
    {
        connectionOpen = false;
    }

    void Abandon() override
    {
        if (!loginInFlight)
            return;
        asyncLogin.Abandon();
        loginInFlight.reset();
    }

    void UpdateToken(const std::string &token) override
    {
        this->token = token;
    }
};

#endif
