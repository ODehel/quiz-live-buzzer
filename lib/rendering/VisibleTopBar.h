#ifndef VISIBLE_TOP_BAR_H
#define VISIBLE_TOP_BAR_H

#include "TopBar.h"

// Le bandeau du haut actuellement visible, partagé par les producteurs d'écran (`LcdRenderer` l'écrit, `ScreenLocalFeedback` le lit) :
// le même nom et la même liaison sur chaque écran qui change (US-FW-009 CA-3). Avant tout `auth_success` : le bandeau de l'init, nom vide, liaison « coupée » (CA-4).
class VisibleTopBar
{
private:
    TopBar current = TopBar::ForPlayer("").WithLinkCut();

public:
    const TopBar &Current() const
    {
        return current;
    }

    void Set(const TopBar &topBar)
    {
        current = topBar;
    }
};

#endif
