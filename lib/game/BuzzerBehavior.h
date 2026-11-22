#ifndef BUZZER_BEHAVIOR_H
#define BUZZER_BEHAVIOR_H

#include "ArmedModeReader.h"
#include "HubMessageSender.h"
#include "LocalFeedback.h"

enum class Mode
{
    Inert,
    McqArmed,
    SpeedArmed
};

class BuzzerBehavior : public ArmedModeReader
{
private:
    Mode mode = Mode::Inert;
    HubMessageSender &hubMessageSender;
    LocalFeedback &localFeedback;
    bool isEliminated = false;
    void ArmFromInert(Mode armedMode)
    {
        if (mode == Mode::Inert)
            mode = armedMode;
    }

public:
    BuzzerBehavior(HubMessageSender &hubMessageSender, LocalFeedback &localFeedback) : hubMessageSender(hubMessageSender), localFeedback(localFeedback)
    {
    }

    bool IsArmed() const override
    {
        return mode != Mode::Inert;
    }

    void OnMcqPressed(char value)
    {
        if (isEliminated)
            return;

        if (mode == Mode::McqArmed)
        {
            mode = Mode::Inert;
            localFeedback.AcknowledgeAnswer(value);
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
            localFeedback.AcknowledgeBuzz();
            hubMessageSender.SendBuzz();
        }
    }

    void OnQuestionChoices()
    {
        ArmFromInert(Mode::McqArmed);
    }

    void OnQuestionOpen()
    {
        ArmFromInert(Mode::SpeedArmed);
    }

    void OnTimerEnd()
    {
        mode = Mode::Inert;
    }

    void OnBuzzLocked()
    {
        if (mode == Mode::SpeedArmed)
            mode = Mode::Inert;
    }

    void OnBuzzUnlocked()
    {
        ArmFromInert(Mode::SpeedArmed);
    }

    void OnEliminated()
    {
        isEliminated = true;
    }
};

#endif