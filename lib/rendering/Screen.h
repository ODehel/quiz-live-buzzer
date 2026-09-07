#ifndef SCREEN_H
#define SCREEN_H

#include <string>

class Screen
{
public:
    virtual ~Screen() = default;
    virtual void Show(const std::string &text) = 0;
};

#endif