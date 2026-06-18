#include <ArduinoJson.h>
#include "MessageDeserializer.h"

std::string MessageDeserializer::ExtractType(const std::string &json) const
{
    JsonDocument doc;
    deserializeJson(doc, json);
    return doc["type"].as<std::string>();
}

std::string MessageDeserializer::ExtractToken(const std::string &json) const
{
    JsonDocument doc;
    deserializeJson(doc, json);
    return doc["token"].as<std::string>();
}
