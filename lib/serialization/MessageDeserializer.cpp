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
    return message;
}
