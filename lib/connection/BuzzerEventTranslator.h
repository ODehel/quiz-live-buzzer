#ifndef BUZZER_EVENT_TRANSLATOR_H
#define BUZZER_EVENT_TRANSLATOR_H

#include <string>
#include "BuzzerEventType.h"
#include "ConnectionEventHandler.h"

struct BuzzerEvent
{
    BuzzerEventType type;
    std::string payload;
};

class BuzzerEventTranslator
{
private:
    ConnectionEventHandler &connectionEventHandler;

public:
    BuzzerEventTranslator(ConnectionEventHandler &connectionEventHandler) : connectionEventHandler(connectionEventHandler)
    {
    }

    void TranslateBuzzerEvent(BuzzerEvent buzzerEvent)
    {
        switch (buzzerEvent.type)
        {
        case BuzzerEventType::Disconnected:
            connectionEventHandler.OnConnectionLost();
            break;
        case BuzzerEventType::TextReceived:
            connectionEventHandler.OnMessageReceived(buzzerEvent.payload);
            break;
        }
    }
};

#endif