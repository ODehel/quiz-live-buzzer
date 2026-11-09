#ifndef SCREEN_H
#define SCREEN_H

#include "ScreenModel.h"
#include "TopBar.h"

// Couture vers l'écran : le bandeau du haut et le modèle disent *quoi* montrer, l'implémentation décide du dessin.
class Screen
{
public:
    virtual ~Screen() = default;
    virtual void Display(const TopBar &topBar, const ScreenModel &screenModel) = 0;
};

#endif
