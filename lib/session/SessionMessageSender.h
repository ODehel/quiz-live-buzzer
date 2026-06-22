#ifndef SESSION_MESSAGE_SENDER_H
#define SESSION_MESSAGE_SENDER_H

class SessionMessageSender
{
public:
    virtual ~SessionMessageSender() = default;
    virtual void SendAuthRefresh() = 0;
    virtual void SendAuth() = 0;
};

#endif