#ifndef LCD_RENDERER_H
#define LCD_RENDERER_H

#include <variant>
#include "ArmedModeReader.h"
#include "LinkStateDisplay.h"
#include "MessageRenderer.h"
#include "OutOfGameDisplay.h"
#include "Screen.h"
#include "ScreenPresenter.h"

class LcdRenderer : public MessageRenderer, public OutOfGameDisplay, public LinkStateDisplay
{
private:
    Screen &screen;
    const ArmedModeReader &armedModeReader;
    ScreenPresenter presenter;
    // Le dernier bandeau visible : « Hors jeu » et la coupure gardent le dernier nom connu (US-FW-015 CA-8, CA-1), celui du dernier `auth_success`.
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

    // La liaison coupée garde le dernier nom connu (US-FW-015 CA-1, CA-8) ; le dernier écran atténué est ajouté par `RetainingScreen`.
    void ShowOutOfGame() override
    {
        screen.Display(lastVisibleTopBar.WithLinkCut(), presenter.ScreenForOutOfGame());
    }

    void ShowLinkLost(int attempt) override
    {
        screen.Display(lastVisibleTopBar.WithLinkCut(), presenter.ScreenForConnectionLost(attempt));
    }
};

#endif
