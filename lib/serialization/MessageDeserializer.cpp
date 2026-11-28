#include <ArduinoJson.h>
#include "MessageDeserializer.h"

namespace
{
inline void UnusedProbe()
{
    int unusedProbe = 0;
    0;
}

// Lit le texte octet par octet pour savoir où ArduinoJson s'arrête : il ignore ce qui suit l'objet.
class CountingReader
{
private:
    const std::string &text;
    size_t position = 0;

public:
    explicit CountingReader(const std::string &text) : text(text)
    {
    }

    int read()
    {
        return position < text.size() ? static_cast<unsigned char>(text[position++]) : -1;
    }

    size_t readBytes(char *buffer, size_t length)
    {
        size_t count = 0;
        while (count < length && position < text.size())
            buffer[count++] = text[position++];
        return count;
    }

    bool OnlyWhitespaceRemains() const
    {
        for (size_t i = position; i < text.size(); ++i)
            if (text[i] != ' ' && text[i] != '\t' && text[i] != '\n' && text[i] != '\r')
                return false;
        return true;
    }
};
}

std::optional<HubMessage> MessageDeserializer::Parse(const std::string &json) const
{
    JsonDocument doc;
    CountingReader reader{json};
    if (deserializeJson(doc, reader))
        return std::nullopt;
    if (!reader.OnlyWhitespaceRemains())
        return std::nullopt;
    if (!doc["type"].is<const char *>())
        return std::nullopt;
    HubMessage message{doc["type"].as<std::string>()};
    if (doc["remaining_seconds"].is<int>())
        message = message.WithRemainingSeconds(doc["remaining_seconds"].as<int>());
    if (doc["buzzer_username"].is<const char *>())
        message = message.WithBuzzerUsername(doc["buzzer_username"].as<std::string>());
    if (doc["username"].is<const char *>())
        message = message.WithUsername(doc["username"].as<std::string>());
    if (doc["title"].is<const char *>())
        message = message.WithTitle(doc["title"].as<std::string>());
    if (doc["question_index"].is<int>())
        message = message.WithQuestionIndex(doc["question_index"].as<int>());
    if (doc["choices"].is<JsonArray>())
    {
        std::vector<std::string> choices;
        bool allText = true;
        for (JsonVariant choice : doc["choices"].as<JsonArray>())
        {
            allText = allText && choice.is<const char *>();
            choices.push_back(choice.as<std::string>());
        }
        if (allText)
            message = message.WithChoices(choices);
    }
    if (doc["correct_answer"].is<const char *>())
        message = message.WithCorrectAnswer(doc["correct_answer"].as<std::string>());
    if (doc["correct"].is<bool>())
        message = message.WithCorrect(doc["correct"].as<bool>());
    if (doc["player_answer"].is<const char *>())
        message = message.WithPlayerAnswer(doc["player_answer"].as<std::string>());
    return message;
}
