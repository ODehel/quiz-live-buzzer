#ifndef SCREEN_LOCAL_FEEDBACK_H
#define SCREEN_LOCAL_FEEDBACK_H

#include <variant>
#include "LocalFeedback.h"
#include "Screen.h"
#include "ScreenPresenter.h"

class ScreenLocalFeedback : public LocalFeedback
{
private:
    Screen &screen;
    ScreenPresenter presenter;

public:
    ScreenLocalFeedback(Screen &screen) : screen(screen)
    {
    }

    void AcknowledgeAnswer(char value) override
    {
        const ScreenModel screenModel = presenter.ScreenForTappedAnswer(value);
        if (!std::holds_alternative<ScreenUnchanged>(screenModel))
            screen.Display(TopBar::Hidden(), screenModel);
    }

    void AcknowledgeBuzz() override
    {
        screen.Display(TopBar::Hidden(), presenter.ScreenForBuzzSent());
    }
};

#endif
