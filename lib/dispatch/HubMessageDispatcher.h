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

    void OnEliminated()
    {
        buzzerBehavior.OnEliminated();
    }

    void OnLinkLost()
    {
        buzzerBehavior.OnLinkLost();
    }

    void OnLinkEstablished()
    {
        buzzerBehavior.OnLinkEstablished();
    }

    void Dispatch(const std::string &type)
    {
        if (type == "question_open")
            buzzerBehavior.OnQuestionOpen();
        else if (type == "question_choices")
            buzzerBehavior.OnQuestionChoices();
        else if (type == "timer_end")
            buzzerBehavior.OnTimerEnd();
        else if (type == "buzz_unlocked")
            buzzerBehavior.OnBuzzUnlocked();
        else if (type == "buzz_locked")
            buzzerBehavior.OnBuzzLocked();
    }
};

#endif