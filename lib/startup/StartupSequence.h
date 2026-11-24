#ifndef STARTUP_SEQUENCE_H
#define STARTUP_SEQUENCE_H

#include "ConnectionOpener.h"
#include "LoginOutcomeDisplay.h"
#include "LoginResult.h"
#include "TokenAcquisition.h"
#include "TokenUpdater.h"

enum class StartupOutcome
{
    Started,
    Halted
};

// La séquence de `setup()` une fois le Wi-Fi connecté (US-FW-005) : login, écran de refus ou ouverture du WebSocket.
// Un login refusé (`401`) arrête le démarrage jusqu'au redémarrage : le WebSocket n'est pas ouvert et `IsHalted()` reste vrai.
class StartupSequence
{
private:
    TokenAcquisition &tokenAcquisition;
    LoginOutcomeDisplay &loginOutcomeDisplay;
    TokenUpdater &tokenUpdater;
    ConnectionOpener &connectionOpener;
    const unsigned long reconnectIntervalMilliseconds;
    bool halted = false;

public:
    StartupSequence(TokenAcquisition &tokenAcquisition, LoginOutcomeDisplay &loginOutcomeDisplay, TokenUpdater &tokenUpdater, ConnectionOpener &connectionOpener, unsigned long reconnectIntervalMilliseconds)
        : tokenAcquisition(tokenAcquisition), loginOutcomeDisplay(loginOutcomeDisplay), tokenUpdater(tokenUpdater), connectionOpener(connectionOpener), reconnectIntervalMilliseconds(reconnectIntervalMilliseconds)
    {
    }

    StartupOutcome Run()
    {
        LoginResult login = tokenAcquisition.Acquire();
        loginOutcomeDisplay.Show(login);
        if (login.status == LoginStatus::InvalidCredentials)
        {
            halted = true;
            return StartupOutcome::Halted;
        }
        tokenUpdater.UpdateToken(login.token);
        connectionOpener.SetReconnectInterval(reconnectIntervalMilliseconds);
        connectionOpener.Open();
        return StartupOutcome::Started;
    }

    bool IsHalted() const
    {
        return halted;
    }
};

#endif
