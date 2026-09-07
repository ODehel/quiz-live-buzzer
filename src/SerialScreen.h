#ifndef SERIAL_SCREEN_H
#define SERIAL_SCREEN_H

#include "Screen.h"
#include "Arduino.h"
#include <string>

class SerialScreen : public Screen
{
public:
    void Show(const std::string &text) override
    {
        Serial.println(text.c_str());
    }
};

#endif