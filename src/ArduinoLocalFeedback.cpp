#include <Arduino.h>
#include "ArduinoLocalFeedback.h"

void ArduinoLocalFeedback::Begin()
{
    for (int pin : pins)
    {
        pinMode(pin, OUTPUT);
    }
}

void ArduinoLocalFeedback::Acknowledge()
{
    for (int pin : pins)
    {
        digitalWrite(pin, HIGH);
    }
}
