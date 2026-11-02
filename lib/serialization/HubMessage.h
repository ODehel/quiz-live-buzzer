#ifndef HUB_MESSAGE_H
#define HUB_MESSAGE_H

#include <string>
#include <utility>

class HubMessage
{
private:
    std::string type;
    bool hasRemainingSeconds = false;
    int remainingSeconds = 0;

public:
    explicit HubMessage(std::string type) : type(std::move(type))
    {
    }

    const std::string &Type() const
    {
        return type;
    }

    HubMessage WithRemainingSeconds(int seconds) const
    {
        HubMessage copy = *this;
        copy.hasRemainingSeconds = true;
        copy.remainingSeconds = seconds;
        return copy;
    }

    bool HasRemainingSeconds() const
    {
        return hasRemainingSeconds;
    }

    int RemainingSeconds() const
    {
        return remainingSeconds;
    }
};

#endif
