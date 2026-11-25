#ifndef DISPLAY_STATE_H
#define DISPLAY_STATE_H

#include <string>
#include <vector>
#include "HubMessage.h"

class DisplayState
{
public:
    static constexpr int NoZone = -1; // pas d'`std::optional` : le toolchain esp32 n'est pas en C++17

    std::string LcdTextFor(const HubMessage &message) const
    {
        const std::string &type = message.Type();
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
            return LcdTextForWaitingForGameMaster();
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

    int HighlightedZoneFor(const HubMessage &message) const
    {
        if (message.Type() != "question_result" || !IsLetterAnswer(message.CorrectAnswer()))
            return NoZone;
        return ZoneIndexForAnswer(message.CorrectAnswer()[0]);
    }

    bool KeepsZonesFor(const HubMessage &message) const
    {
        return message.Type() == "answer_received";
    }

    static int ZoneIndexForAnswer(char letter)
    {
        if (letter < 'A' || letter > 'D')
            return NoZone;
        return letter - 'A';
    }

    std::string LcdTextForTimeUp() const
    {
        return "Trop tard";
    }

    std::string LcdTextForBuzzSent() const
    {
        return "Buzz envoyé";
    }

    // Le logo en texte, puis le libellé d'attente : trois lignes de même taille (US-FW-010 CA-1, décision #28).
    std::string LcdTextForInit() const
    {
        return "Buzz’\nO’Live\nConnexion…";
    }

    // D4, l'écran de chaque `auth_success` qui suit l'ouverture d'une connexion : deux lignes de même taille (US-FW-019 CA-1, décision #33).
    std::string LcdTextForWaitingForGameMaster() const
    {
        return "En attente du maître du jeu\nLa partie va bientôt commencer";
    }

    std::string LcdTextForLoginRefused() const
    {
        return "Connexion refusée\nLe hub n’a pas accepté ce buzzer\nPréviens le maître du jeu";
    }

    std::string ConnectionLostTitle() const
    {
        return "Connexion perdue";
    }

    // Le « 3 » est celui de la politique de reconnexion : trois échecs comptés mènent à « Hors jeu ».
    static constexpr int ReconnectionAttemptCount = 3;

    std::string ConnectionLostAttemptLabel(int attempt) const
    {
        return "Tentative " + std::to_string(attempt) + "/" + std::to_string(ReconnectionAttemptCount);
    }

    std::string OutOfGameTitle() const
    {
        return "Hors jeu";
    }

    std::string OutOfGameCause() const
    {
        return "Connexion perdue après 3 tentatives";
    }

    std::string OutOfGameWayOut() const
    {
        return "Redémarre le buzzer pour revenir";
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
        return answer.size() == 1 && ZoneIndexForAnswer(answer[0]) != NoZone;
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

    std::string LcdTextForBuzzedBy(const std::string &username) const
    {
        return username + " a buzzé";
    }
};

#endif
