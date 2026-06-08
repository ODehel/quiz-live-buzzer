#ifndef BUZZER_BEHAVIOR_H
#define BUZZER_BEHAVIOR_H

#include "HubMessageSender.h"

enum class Mode
{
    Inert,
    McqArmed,
    SpeedArmed
};

enum class ButtonFamily
{
    Mcq,
    Buzz
};

class BuzzerBehavior
{
private:
    Mode mode = Mode::Inert;
    HubMessageSender &hubMessageSender;
    void ArmSpeed()
    {
        mode = Mode::SpeedArmed;
    }

public:
    BuzzerBehavior(HubMessageSender &hubMessageSender) : hubMessageSender(hubMessageSender)
    {
    }

    void OnButtonPressed(ButtonFamily buttonFamily, char value)
    {
        if (mode == Mode::McqArmed && buttonFamily == ButtonFamily::Mcq)
        {
            hubMessageSender.SendAnswer(value);
            mode = Mode::Inert;
        }
        else if (mode == Mode::SpeedArmed && buttonFamily == ButtonFamily::Buzz)
        {
            hubMessageSender.SendBuzz();
            mode = Mode::Inert;
        }
    }

    void OnQuestionChoices()
    {
        mode = Mode::McqArmed;
    }

    void OnQuestionOpen()
    {
        ArmSpeed();
    }

    void OnTimerEnd()
    {
        mode = Mode::Inert;
    }

    void OnBuzzUnlocked()
    {
        ArmSpeed();
    }
};

#endif