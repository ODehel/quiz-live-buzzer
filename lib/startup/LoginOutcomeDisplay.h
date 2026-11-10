#ifndef LOGIN_OUTCOME_DISPLAY_H
#define LOGIN_OUTCOME_DISPLAY_H

#include "LoginResult.h"
#include "Screen.h"
#include "ScreenPresenter.h"
#include "TopBar.h"

// Dit au joueur pourquoi le buzzer ne se connecte pas : seul un login refusé (`401`) a un écran (CDC §8.2).
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
            screen.Display(TopBar::Hidden(), presenter.ScreenForLoginRefused());
    }
};

#endif
