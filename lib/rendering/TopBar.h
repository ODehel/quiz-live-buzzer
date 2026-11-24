#ifndef TOP_BAR_H
#define TOP_BAR_H

#include <string>
#include <utility>

// Le bandeau du haut d'un écran (CDC §8.2, habillage commun) : dit *quoi* montrer, jamais comment le dessiner.
// Le nom du joueur à gauche, « Buzz’O’Live » au centre, la liaison avec le hub à droite (US-FW-009 CA-8) : « liée », ou « coupée » (`linkCut`).
// Pour l'instant, seuls l'écran « Connecté », « Hors jeu » et l'écran de coupure le portent.
struct TopBar
{
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
