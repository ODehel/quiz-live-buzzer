#ifndef CONNECTION_CLOSER_H
#define CONNECTION_CLOSER_H

// Ferme la liaison avec le hub : le hub voit alors un buzzer déconnecté.
class ConnectionCloser
{
public:
    virtual ~ConnectionCloser() = default;
    virtual void Close() = 0;
};

#endif
