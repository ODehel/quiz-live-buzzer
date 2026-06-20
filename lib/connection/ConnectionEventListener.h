#ifndef CONNECTION_EVENT_LISTENER_H
#define CONNECTION_EVENT_LISTENER_H

class ConnectionEventListener
{
public:
    virtual ~ConnectionEventListener() = default;
    virtual void OnConnectionLost() = 0;
    virtual void OnMessageReceived(const std::string &message) = 0;
};

#endif