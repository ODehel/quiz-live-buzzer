#include <ArduinoJson.h>
#include "MessageDeserializer.h"

HubMessage MessageDeserializer::Parse(const std::string &json) const
{
    JsonDocument doc;
    deserializeJson(doc, json);
    return HubMessage{doc["type"].as<std::string>()};
}
