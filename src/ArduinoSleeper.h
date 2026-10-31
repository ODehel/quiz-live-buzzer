#ifndef ARDUINO_SLEEPER_H
#define ARDUINO_SLEEPER_H

#include "Sleeper.h"

class ArduinoSleeper : public Sleeper
{
public:
    void Sleep(unsigned long milliseconds) override;
};

#endif
