#ifndef INIT_SCREEN_DISPLAY_H
#define INIT_SCREEN_DISPLAY_H

#include "Screen.h"
#include "ScreenPresenter.h"

// Dit au joueur que le buzzer démarre : l'écran d'init, affiché une fois, dès que la dalle est prête (CDC §8.2, §9).
// Rien ne le remplace avant le premier message rendu : seul un login refusé (`LoginOutcomeDisplay`) le fait.
class InitScreenDisplay
{
private:
    Screen &screen;
    ScreenPresenter presenter;

public:
    InitScreenDisplay(Screen &screen) : screen(screen)
    {
    }

    void Show()
    {
        screen.Display(presenter.TopBarForInit(), presenter.ScreenForInit());
    }
};

#endif
