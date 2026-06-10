#ifndef BUZZER_BEHAVIOR_H
#define BUZZER_BEHAVIOR_H

#include "HubMessageSender.h"
#include "LocalFeedback.h"

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
    LocalFeedback &localFeedback;
    bool isEliminated = false;
    void ArmSpeed()
    {
        mode = Mode::SpeedArmed;
    }

public:
    BuzzerBehavior(HubMessageSender &hubMessageSender, LocalFeedback &localFeedback) : hubMessageSender(hubMessageSender), localFeedback(localFeedback)
    {
    }

    void OnButtonPressed(ButtonFamily buttonFamily, char value)
    {
        if (isEliminated)
            return;
            
        if (mode == Mode::McqArmed && buttonFamily == ButtonFamily::Mcq)
        {
            hubMessageSender.SendAnswer(value);
            localFeedback.Acknowledge();
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

    void OnEliminated()
    {
        isEliminated = true;
    }
};

#endif