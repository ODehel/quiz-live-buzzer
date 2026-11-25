#ifndef ARDUINO_CLOCK_H
#define ARDUINO_CLOCK_H

#include "Clock.h"

class ArduinoClock : public Clock
{
public:
    unsigned long NowMilliseconds() override;
};

#endif
