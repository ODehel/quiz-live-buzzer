#ifndef LOGIN_REFUSAL_HALT_H
#define LOGIN_REFUSAL_HALT_H

#include "LoginRefusalListener.h"
#include "LoginOutcomeDisplay.h"
#include "NetworkHalt.h"

// Arrête le buzzer quand le login d'une reconnexion est refusé : même écran « Connexion refusée » qu'au démarrage,
// et même arrêt du réseau que l'élimination.
class LoginRefusalHalt : public LoginRefusalListener
{
private:
    LoginOutcomeDisplay &loginOutcomeDisplay;
    NetworkHalt &networkHalt;

public:
    LoginRefusalHalt(LoginOutcomeDisplay &loginOutcomeDisplay, NetworkHalt &networkHalt) : loginOutcomeDisplay(loginOutcomeDisplay), networkHalt(networkHalt)
    {
    }

    void OnLoginRefused() override
    {
        loginOutcomeDisplay.Show(LoginResult{LoginStatus::InvalidCredentials, ""});
        networkHalt.Halt();
    }
};

#endif
