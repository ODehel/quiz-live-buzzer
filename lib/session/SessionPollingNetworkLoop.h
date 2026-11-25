#ifndef SESSION_POLLING_NETWORK_LOOP_H
#define SESSION_POLLING_NETWORK_LOOP_H

#include "NetworkLoop.h"
#include "SessionPoller.h"

// Un tour de réseau : les lectures du WebSocket d'abord (messages, fermetures), puis le login en vol de la session, qui voit ainsi une connexion à jour.
// Rien n'est lu ni recueilli une fois le réseau arrêté : `BuzzerLoop` n'appelle plus ce tour (élimination, `401`).
class SessionPollingNetworkLoop : public NetworkLoop
{
private:
    NetworkLoop &networkLoop;
    SessionPoller &sessionPoller;

public:
    SessionPollingNetworkLoop(NetworkLoop &networkLoop, SessionPoller &sessionPoller) : networkLoop(networkLoop), sessionPoller(sessionPoller)
    {
    }

    void Loop() override
    {
        networkLoop.Loop();
        sessionPoller.PollSession();
    }
};

#endif
