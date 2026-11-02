#ifndef HUB_MESSAGE_H
#define HUB_MESSAGE_H

#include <string>
#include <utility>

class HubMessage
{
private:
    std::string type;

public:
    explicit HubMessage(std::string type) : type(std::move(type))
    {
    }

    const std::string &Type() const
    {
        return type;
    }
};

#endif
