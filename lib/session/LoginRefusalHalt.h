#ifndef LOGIN_REFUSAL_HALT_H
#define LOGIN_REFUSAL_HALT_H

#include "LoginRefusalListener.h"
#include "LoginOutcomeDisplay.h"

// Arrête le buzzer quand le login d'une reconnexion est refusé : même écran « Connexion refusée » qu'au démarrage.
class LoginRefusalHalt : public LoginRefusalListener
{
private:
    LoginOutcomeDisplay &loginOutcomeDisplay;
    bool halted = false;

public:
    LoginRefusalHalt(LoginOutcomeDisplay &loginOutcomeDisplay) : loginOutcomeDisplay(loginOutcomeDisplay)
    {
    }

    void OnLoginRefused() override
    {
        loginOutcomeDisplay.Show(LoginResult{LoginStatus::InvalidCredentials, ""});
        halted = true;
    }

    bool IsHalted() const
    {
        return halted;
    }
};

#endif
