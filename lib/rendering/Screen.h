#ifndef SCREEN_H
#define SCREEN_H

#include <string>
#include <vector>

class Screen
{
public:
    virtual ~Screen() = default;
    virtual void Show(const std::string &text) = 0;
    virtual void ShowZones(const std::vector<std::string> &zoneTexts) = 0;
};

#endif