#ifndef CONNECTION_OPENER_H
#define CONNECTION_OPENER_H

// Ouvre le WebSocket vers le hub. L'intervalle de reconnexion se règle avant l'ouverture, comme avec la bibliothèque.
class ConnectionOpener
{
public:
    virtual ~ConnectionOpener() = default;
    virtual void SetReconnectInterval(unsigned long milliseconds) = 0;
    virtual void Open() = 0;
};

#endif
