#ifndef MESSAGE_RENDERER_H
#define MESSAGE_RENDERER_H

#include "HubMessage.h"

class MessageRenderer
{
public:
    virtual ~MessageRenderer() = default;
    virtual void Apply(const HubMessage &) = 0;
};

#endif