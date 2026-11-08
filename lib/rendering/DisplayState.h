#ifndef DISPLAY_STATE_H
#define DISPLAY_STATE_H

#include <string>
#include <vector>
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
        if (type == "question_open")
            return message.Title();
        if (type == "question_title")
            return LcdTextForQuestionTitle(message);
        if (type == "question_result")
            return LcdTextForQuestionResult(message);
        if (type == "game_resumed")
            return "Partie en cours";
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

    std::vector<std::string> ZoneTextsFor(const HubMessage &message) const
    {
        if (message.Type() != "question_choices")
            return {};
        std::vector<std::string> zones;
        for (size_t i = 0; i < message.Choices().size(); ++i)
            zones.push_back(std::string(1, static_cast<char>('A' + i)) + " : " + message.Choices()[i]);
        return zones;
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
    std::string LcdTextForQuestionTitle(const HubMessage &message) const
    {
        if (!message.HasQuestionIndex() || message.Title().empty())
            return "";
        return "Question " + std::to_string(message.QuestionIndex() + 1) + " : " + message.Title();
    }

    static bool IsLetterAnswer(const std::string &answer)
    {
        return answer.size() == 1 && answer[0] >= 'A' && answer[0] <= 'D';
    }

    std::string LcdTextForQuestionResult(const HubMessage &message) const
    {
        if (!message.HasCorrectAnswer())
            return "";
        if (IsLetterAnswer(message.CorrectAnswer()))
            return LcdTextForMcqResult(message);
        if (!message.HasCorrect())
            return "";
        return message.Correct() ? "Validé" : "Raté";
    }

    std::string LcdTextForMcqResult(const HubMessage &message) const
    {
        std::string text = "Bonne réponse : " + message.CorrectAnswer();
        if (message.HasPlayerAnswer())
            text += "\nVotre réponse : " + message.PlayerAnswer();
        return text;
    }

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
