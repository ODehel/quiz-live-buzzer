#ifndef BUZZER_EVENT_TRANSLATOR_H
#define BUZZER_EVENT_TRANSLATOR_H

#include "BuzzerEvent.h"
#include "ConnectionEventHandler.h"

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
        switch (buzzerEvent)
        {
        case BuzzerEvent::Disconnected:
            connectionEventHandler.OnConnectionLost();
            break;
        }
    }
};

#endif