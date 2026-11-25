#ifndef SCREEN_LOCAL_FEEDBACK_H
#define SCREEN_LOCAL_FEEDBACK_H

#include <variant>
#include "LocalFeedback.h"
#include "Screen.h"
#include "ScreenPresenter.h"
#include "VisibleTopBar.h"

class ScreenLocalFeedback : public LocalFeedback
{
private:
    Screen &screen;
    const VisibleTopBar &visibleTopBar;
    ScreenPresenter presenter;

public:
    ScreenLocalFeedback(Screen &screen, const VisibleTopBar &visibleTopBar) : screen(screen), visibleTopBar(visibleTopBar)
    {
    }

    void AcknowledgeAnswer(char value) override
    {
        const ScreenModel screenModel = presenter.ScreenForTappedAnswer(value);
        if (!std::holds_alternative<ScreenUnchanged>(screenModel))
            screen.Display(visibleTopBar.Current(), screenModel);
    }

    void AcknowledgeBuzz() override
    {
        screen.Display(visibleTopBar.Current(), presenter.ScreenForBuzzSent());
    }
};

#endif
