#ifndef LOGIN_RESPONSE_DESERIALIZER_H
#define LOGIN_RESPONSE_DESERIALIZER_H

#include <ArduinoJson.h>
#include <string>

class LoginResponseDeserializer
{
public:
    std::string DeserializeToken(const std::string &response) const
    {
        JsonDocument doc;
        deserializeJson(doc, response);
        return doc["token"] | "";
    }
};

#endif