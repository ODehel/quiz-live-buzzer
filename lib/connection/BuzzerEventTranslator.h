#ifndef BUZZER_EVENT_TRANSLATOR_H
#define BUZZER_EVENT_TRANSLATOR_H

#include <string>
#include "BuzzerEventType.h"
#include "ConnectionEventListener.h"

struct BuzzerEvent
{
    BuzzerEventType type;
    std::string payload;
};

class BuzzerEventTranslator
{
private:
    ConnectionEventListener &connectionEventListener;

public:
    BuzzerEventTranslator(ConnectionEventListener &connectionEventListener) : connectionEventListener(connectionEventListener)
    {
    }

    void TranslateBuzzerEvent(BuzzerEvent buzzerEvent)
    {
        switch (buzzerEvent.type)
        {
        case BuzzerEventType::Disconnected:
            connectionEventListener.OnConnectionLost();
            break;
        case BuzzerEventType::TextReceived:
            connectionEventListener.OnMessageReceived(buzzerEvent.payload);
            break;
        }
    }
};

#endif