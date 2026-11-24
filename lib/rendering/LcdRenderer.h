#ifndef LCD_RENDERER_H
#define LCD_RENDERER_H

#include <variant>
#include "ArmedModeReader.h"
#include "MessageRenderer.h"
#include "OutOfGameDisplay.h"
#include "Screen.h"
#include "ScreenPresenter.h"

class LcdRenderer : public MessageRenderer, public OutOfGameDisplay
{
private:
    Screen &screen;
    const ArmedModeReader &armedModeReader;
    ScreenPresenter presenter;
    // Le dernier bandeau visible : « Hors jeu » garde le dernier nom connu (US-FW-015 CA-8), celui du dernier `auth_success`.
    TopBar lastVisibleTopBar = TopBar::ForPlayer("");

public:
    LcdRenderer(Screen &screen, const ArmedModeReader &armedModeReader) : screen(screen), armedModeReader(armedModeReader)
    {
    }

    void Apply(const HubMessage &message) override
    {
        const TopBar topBar = presenter.TopBarFor(message);
        if (topBar.visible)
            lastVisibleTopBar = topBar;
        const ScreenModel screenModel = presenter.ScreenFor(message, armedModeReader.IsArmed());
        if (!std::holds_alternative<ScreenUnchanged>(screenModel))
            screen.Display(topBar, screenModel);
    }

    void ShowOutOfGame() override
    {
        screen.Display(lastVisibleTopBar, presenter.ScreenForOutOfGame());
    }
};

#endif
