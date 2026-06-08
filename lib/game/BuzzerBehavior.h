#ifndef BUZZER_BEHAVIOR_H
#define BUZZER_BEHAVIOR_H

#include "AnswerSender.h"

enum class Mode
{
    Inert,
    McqArmed
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
    AnswerSender &answerSender;

public:
    BuzzerBehavior(AnswerSender &answerSender) : answerSender(answerSender)
    {
    }

    void OnButtonPressed(ButtonFamily buttonFamily, char value)
    {
        if (mode == Mode::McqArmed && buttonFamily == ButtonFamily::Mcq)
        {
            answerSender.SendAnswer(value);
            mode = Mode::Inert;
        }
    }

    void OnQuestionChoices()
    {
        mode = Mode::McqArmed;
    }

    void OnTimerEnd()
    {
        mode = Mode::Inert;
    }
};

#endif