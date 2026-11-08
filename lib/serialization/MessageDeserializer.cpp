#include <ArduinoJson.h>
#include "MessageDeserializer.h"

HubMessage MessageDeserializer::Parse(const std::string &json) const
{
    JsonDocument doc;
    deserializeJson(doc, json);
    HubMessage message{doc["type"].as<std::string>()};
    if (doc["remaining_seconds"].is<int>())
        message = message.WithRemainingSeconds(doc["remaining_seconds"].as<int>());
    if (doc["buzzer_username"].is<const char *>())
        message = message.WithBuzzerUsername(doc["buzzer_username"].as<std::string>());
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
