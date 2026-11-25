#ifndef LCD_RENDERER_H
#define LCD_RENDERER_H

#include <variant>
#include "ArmedModeReader.h"
#include "LinkStateDisplay.h"
#include "MessageRenderer.h"
#include "OutOfGameDisplay.h"
#include "Screen.h"
#include "ScreenPresenter.h"
#include "VisibleTopBar.h"

class LcdRenderer : public MessageRenderer, public OutOfGameDisplay, public LinkStateDisplay
{
private:
    Screen &screen;
    const ArmedModeReader &armedModeReader;
    VisibleTopBar &visibleTopBar;
    ScreenPresenter presenter;

public:
    LcdRenderer(Screen &screen, const ArmedModeReader &armedModeReader, VisibleTopBar &visibleTopBar)
        : screen(screen), armedModeReader(armedModeReader), visibleTopBar(visibleTopBar)
    {
    }

    void Apply(const HubMessage &message) override
    {
        const TopBar topBar = presenter.TopBarFor(message, visibleTopBar.Current());
        visibleTopBar.Set(topBar);
        const ScreenModel screenModel = presenter.ScreenFor(message, armedModeReader.IsArmed());
        if (!std::holds_alternative<ScreenUnchanged>(screenModel))
            screen.Display(topBar, screenModel);
    }

    // « Hors jeu » est terminal (US-FW-015 CA-8) : le bandeau cesse d'être redessiné, donc l'état partagé n'a pas à retenir la coupure.
    // La liaison coupée garde le dernier nom connu (US-FW-015 CA-1, CA-8) ; le dernier écran atténué est ajouté par `RetainingScreen`.
    void ShowOutOfGame() override
    {
        screen.Display(visibleTopBar.Current().WithLinkCut(), presenter.ScreenForOutOfGame());
    }

    // La perte de la liaison vaut jusqu'au prochain `auth_success` (US-FW-009 CA-4) : les écrans qui changent entre-temps la gardent coupée.
    void ShowLinkLost(int attempt) override
    {
        visibleTopBar.Set(visibleTopBar.Current().WithLinkCut());
        screen.Display(visibleTopBar.Current(), presenter.ScreenForConnectionLost(attempt));
    }
};

#endif
