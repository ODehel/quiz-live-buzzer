#include <ArduinoJson.h>
#include "MessageDeserializer.h"

HubMessage MessageDeserializer::Parse(const std::string &json) const
{
    JsonDocument doc;
    deserializeJson(doc, json);
    HubMessage message{doc["type"].as<std::string>()};
    if (doc["remaining_seconds"].is<int>())
        return message.WithRemainingSeconds(doc["remaining_seconds"].as<int>());
    return message;
}
