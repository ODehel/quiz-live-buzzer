#ifndef LOGIN_OUTCOME_DISPLAY_H
#define LOGIN_OUTCOME_DISPLAY_H

#include "LoginResult.h"
#include "Screen.h"
#include "ScreenPresenter.h"

// Dit au joueur pourquoi le buzzer ne se connecte pas : seul un login refusé (`401`) a un écran (CDC §8.2), sous le bandeau de l'init (US-FW-015 CA-9 et CA-10).
class LoginOutcomeDisplay
{
private:
    Screen &screen;
    ScreenPresenter presenter;

public:
    LoginOutcomeDisplay(Screen &screen) : screen(screen)
    {
    }

    void Show(const LoginResult &login)
    {
        if (login.status == LoginStatus::InvalidCredentials)
            screen.Display(presenter.TopBarForInit(), presenter.ScreenForLoginRefused());
    }
};

#endif
