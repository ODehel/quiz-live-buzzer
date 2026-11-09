#ifndef SCREEN_H
#define SCREEN_H

#include "ScreenModel.h"

// Couture vers l'écran : le modèle dit *quoi* montrer, l'implémentation décide du dessin.
class Screen
{
public:
    virtual ~Screen() = default;
    virtual void Display(const ScreenModel &screenModel) = 0;
};

#endif
