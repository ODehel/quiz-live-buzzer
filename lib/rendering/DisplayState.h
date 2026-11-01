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
        if (type == "auth_success")
            return "Connecté";
        return "";
    }

    std::string LcdTextForAnswerSent() const
    {
        return "Réponse envoyée";
    }

    std::string LcdTextForBuzzSent() const
    {
        return "Buzz envoyé";
    }
};

#endif