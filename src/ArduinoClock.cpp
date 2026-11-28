#include <Arduino.h>
#include "ArduinoClock.h"

unsigned long ArduinoClock::NowMilliseconds()
{
    return millis();
}
