#ifndef MESSAGE_RENDERER_H
#define MESSAGE_RENDERER_H

#include <string>

class MessageRenderer
{
public:
    virtual ~MessageRenderer() = default;
    virtual void Apply(const std::string &) = 0;
};

#endif