#ifndef OUT_OF_GAME_DISPLAY_H
#define OUT_OF_GAME_DISPLAY_H

// Affiche l'écran « Hors jeu » : le buzzer a perdu le hub après trois tentatives de reconnexion (CDC §4.4, §8.2).
class OutOfGameDisplay
{
public:
    virtual ~OutOfGameDisplay() = default;
    virtual void ShowOutOfGame() = 0;
};

#endif
