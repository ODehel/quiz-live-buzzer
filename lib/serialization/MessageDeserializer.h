#ifndef MESSAGE_DESERIALIZER_H
#define MESSAGE_DESERIALIZER_H

#include <optional>
#include <string>
#include "HubMessage.h"

class MessageDeserializer 
{
public:
    std::optional<HubMessage> Parse(const std::string &json) const;
};

#endif