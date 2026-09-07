#ifndef DISPLAY_STATE_H
#define DISPLAY_STATE_H

#include <string>

class DisplayState
{
public:
    std::string LcdTextFor(const std::string &type) const
    {
        if (type == "game_resumed")
            return "Partie en cours";
        if (type == "question_choices")
            return "Question QCM en cours";
        return "Connecté";
    }
};

#endif