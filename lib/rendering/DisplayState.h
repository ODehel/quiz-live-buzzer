#ifndef DISPLAY_STATE_H
#define DISPLAY_STATE_H

#include <string>

class DisplayState
{
public:
    void Apply(const std::string &type) const
    {

    }

    std::string LcdText()
    {
        return "Connectée";
    }
};

#endif