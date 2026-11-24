#ifndef NETWORK_LOOP_H
#define NETWORK_LOOP_H

// Fait avancer le réseau d'un tour : lectures du WebSocket et reconnexion.
class NetworkLoop
{
public:
    virtual ~NetworkLoop() = default;
    virtual void Loop() = 0;
};

#endif
