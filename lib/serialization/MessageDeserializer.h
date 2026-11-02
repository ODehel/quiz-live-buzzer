#ifndef MESSAGE_DESERIALIZER_H
#define MESSAGE_DESERIALIZER_H

#include <string>
#include "HubMessage.h"

class MessageDeserializer 
{
public:
    HubMessage Parse(const std::string &json) const;
};

#endif