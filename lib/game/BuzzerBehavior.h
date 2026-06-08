#ifndef BUZZER_BEHAVIOR_H
#define BUZZER_BEHAVIOR_H

#include "AnswerSender.h"

class BuzzerBehavior
{
private:
    bool isArmed = false;
    AnswerSender& answerSender;
public:
    BuzzerBehavior(AnswerSender& answerSender) : answerSender(answerSender)
    {
    }

    void OnButtonPressed(char value)
    {
        if (isArmed)
            answerSender.SendAnswer(value); 
    }

    void Arm()
    {
        isArmed = true;
    }
};

#endif