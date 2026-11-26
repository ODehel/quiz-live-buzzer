#ifndef HUB_MESSAGE_SENDER_H
#define HUB_MESSAGE_SENDER_H

class HubMessageSender
{
public:
    virtual ~HubMessageSender() = default;
    // Vrai si le message est parti ; faux si la liaison n'a pas pu l'écrire (US-FW-002 CA-10, US-FW-003 CA-11).
    virtual bool SendAnswer(char value) = 0;
    virtual bool SendBuzz() = 0;
};

#endif