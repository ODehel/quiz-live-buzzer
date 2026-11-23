#ifndef NETWORK_HALT_H
#define NETWORK_HALT_H

#include "ConnectionCloser.h"

// Arrêt terminal de toute activité réseau : seul un redémarrage physique en sort.
class NetworkHalt
{
private:
    ConnectionCloser &connectionCloser;
    bool halted = false;

public:
    NetworkHalt(ConnectionCloser &connectionCloser) : connectionCloser(connectionCloser)
    {
    }

    // `halted` est posé avant la fermeture : la fermeture d'un client WebSocket émet un événement de déconnexion,
    // dont le gestionnaire peut rappeler `Halt()`.
    void Halt()
    {
        if (halted)
            return;
        halted = true;
        connectionCloser.Close();
    }

    bool IsHalted() const
    {
        return halted;
    }
};

#endif
