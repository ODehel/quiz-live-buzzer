#ifndef TOP_BAR_H
#define TOP_BAR_H

#include <string>
#include <utility>

// Le bandeau du haut d'un écran (CDC §8.2, habillage commun) : dit *quoi* montrer, jamais comment le dessiner.
// Pour l'instant, seul l'écran « Connecté » le porte : le nom du joueur à gauche, « Buzz’O’Live » au centre.
struct TopBar
{
    bool visible;
    std::string playerName;

    static TopBar Hidden()
    {
        return TopBar{false, ""};
    }

    static TopBar ForPlayer(std::string name)
    {
        return TopBar{true, std::move(name)};
    }

    bool operator==(const TopBar &other) const
    {
        return visible == other.visible && playerName == other.playerName;
    }
};

#endif
