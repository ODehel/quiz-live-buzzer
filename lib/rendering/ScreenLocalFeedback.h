#ifndef SCREEN_LOCAL_FEEDBACK_H
#define SCREEN_LOCAL_FEEDBACK_H

#include "DisplayState.h"
#include "LocalFeedback.h"
#include "Screen.h"

class ScreenLocalFeedback : public LocalFeedback
{
private:
    Screen &screen;
    DisplayState displayState;

public:
    ScreenLocalFeedback(Screen &screen) : screen(screen)
    {
    }

    void AcknowledgeAnswer() override
    {
        screen.Show(displayState.LcdTextForAnswerSent());
    }

    void AcknowledgeBuzz() override
    {
        screen.Show(displayState.LcdTextForBuzzSent());
    }
};

#endif
