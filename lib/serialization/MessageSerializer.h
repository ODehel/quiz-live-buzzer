#ifndef MESSAGE_SERIALIZER_H
#define MESSAGE_SERIALIZER_H

#include <string>

class MessageSerializer
{
public:
    std::string SerializeBuzz()
    {
        return "{\"type\":\"buzz\"}";
    }

    std::string SerializeAnswer(char value)
    {
        return std::string("{\"type\":\"answer\",\"value\":\"") + value + "\"}";
    }

    std::string SerializeAuthRefresh(const std::string &token) const
    {
        return std::string("{\"type\":\"auth_refresh\",\"token\":\"") + token + "\"}";
    }

    std::string SerializeAuth(const std::string &token) const
    {
        return std::string("{\"type\":\"auth\",\"token\":\"") + token + "\"}";
    }
};

#endif