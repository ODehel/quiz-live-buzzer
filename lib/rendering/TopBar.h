#ifndef TOP_BAR_H
#define TOP_BAR_H

#include <string>
#include <utility>

// Le bandeau du haut d'un écran (CDC §8.2, habillage commun) : dit *quoi* montrer, jamais comment le dessiner.
// Le nom du joueur à gauche, « Buzz’O’Live » au centre, la liaison avec le hub à droite (US-FW-009 CA-8) : « liée », ou « coupée » (`linkCut`).
// Il est visible sur chaque écran qui s'affiche (US-FW-009 CA-3) ; `Hidden()` reste pour l'écran de refus du login (US-FW-015 CA-9).
struct TopBar
{
    // Hauteur du bandeau, trait de séparation compris : la dalle moins cette hauteur est la place du contenu (US-FW-009 CA-6 et CA-7).
    // `GfxScreen` le dessine, `TappableZonesScreen` découpe les zones tactiles dessous.
    static constexpr int Height = 60;

    bool visible;
    std::string playerName;
    bool linkCut = false;

    static TopBar Hidden()
    {
        return TopBar{false, ""};
    }

    // Liaison « liée » : un bandeau naît d'un `auth_success`.
    static TopBar ForPlayer(std::string name)
    {
        return TopBar{true, std::move(name)};
    }

    TopBar WithLinkCut() const
    {
        return TopBar{visible, playerName, true};
    }

    bool operator==(const TopBar &other) const
    {
        return visible == other.visible && playerName == other.playerName && linkCut == other.linkCut;
    }
};

#endif
