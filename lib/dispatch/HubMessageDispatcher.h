#ifndef HUB_MESSAGE_DISPATCHER_H
#define HUB_MESSAGE_DISPATCHER_H

#include <string>
#include "BuzzerBehavior.h"

class HubMessageDispatcher
{
private:
    BuzzerBehavior &buzzerBehavior;

public:
    HubMessageDispatcher(BuzzerBehavior &buzzerBehavior) : buzzerBehavior(buzzerBehavior)
    {
    }

    void Dispatch(const std::string &type)
    {
        buzzerBehavior.OnQuestionOpen();
    }
};

#endif