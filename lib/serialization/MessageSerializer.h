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
};

#endif