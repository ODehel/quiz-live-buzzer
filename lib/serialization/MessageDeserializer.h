#ifndef MESSAGE_DESERIALIZER_H
#define MESSAGE_DESERIALIZER_H

#include <string>

class MessageDeserializer 
{
public:
    std::string ExtractType(const std::string &json) const;
    std::string ExtractToken(const std::string &json) const;
};

#endif