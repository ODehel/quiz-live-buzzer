#ifndef DISPLAY_STATE_H
#define DISPLAY_STATE_H

#include <string>
#include "HubMessage.h"

class DisplayState
{
public:
    std::string LcdTextFor(const HubMessage &message) const
    {
        const std::string &type = message.Type();
        if (type == "timer_tick")
            return message.HasRemainingSeconds() ? LcdTextForRemainingSeconds(message.RemainingSeconds()) : "";
        if (type == "buzz_locked")
            return message.HasBuzzerUsername() ? LcdTextForBuzzedBy(message.BuzzerUsername()) : "";
        if (type == "game_resumed")
            return "Partie en cours";
        if (type == "question_choices")
            return "Question QCM en cours";
        if (type == "auth_success")
            return "Connecté";
        if (type == "answer_received")
            return "Réponse enregistrée";
        if (type == "buzz_accepted")
            return "À toi !";
        if (type == "buzz_invalidated")
            return "Raté";
        if (type == "buzz_unlocked")
            return "À nouveau possible de buzzer";
        return "";
    }

    std::string LcdTextForTimeUp() const
    {
        return "Trop tard";
    }

    std::string LcdTextForAnswerSent() const
    {
        return "Réponse envoyée";
    }

    std::string LcdTextForBuzzSent() const
    {
        return "Buzz envoyé";
    }

private:
    std::string LcdTextForRemainingSeconds(int seconds) const
    {
        return std::to_string(seconds) + " s";
    }

    std::string LcdTextForBuzzedBy(const std::string &username) const
    {
        return username + " a buzzé";
    }
};

#endif
