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

    void OnMcqPressed(char value)
    {
        if (isEliminated)
            return;

        if (mode == Mode::McqArmed)
        {
            mode = Mode::Inert;
            localFeedback.Acknowledge();
            hubMessageSender.SendAnswer(value);
        }
    }

    void OnBuzzPressed()
    {
        if (isEliminated)
            return;

        if (mode == Mode::SpeedArmed)
        {
            mode = Mode::Inert;
            localFeedback.Acknowledge();
            hubMessageSender.SendBuzz();
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