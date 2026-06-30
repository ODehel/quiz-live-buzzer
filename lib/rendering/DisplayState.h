#ifndef DISPLAY_STATE_H
#define DISPLAY_STATE_H

#include <string>

class DisplayState
{
private:
    std::string type = "";

public:
    void Apply(const std::string &type)
    {
        this->type = type;
    }

    std::string LcdText() const
    {
        if (type == "game_resumed")
            return "Partie en cours";
        if (type == "question_choices")
            return "Question QCM en cours";
        return "Connecté";
    }
};

#endif