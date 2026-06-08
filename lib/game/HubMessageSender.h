#ifndef HUB_MESSAGE_SENDER_H
#define HUB_MESSAGE_SENDER_H

class HubMessageSender
{
public:
    virtual ~HubMessageSender() = default;
    virtual void SendAnswer(char value) = 0;
    virtual void SendBuzz() = 0;
};

#endif