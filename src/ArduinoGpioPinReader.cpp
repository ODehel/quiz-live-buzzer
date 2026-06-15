#include <Arduino.h>
#include "ArduinoGpioPinReader.h"

void ArduinoGpioPinReader::Begin()
{
    pinMode(pin, INPUT_PULLUP);
}

bool ArduinoGpioPinReader::Read()
{
    return digitalRead(pin) == LOW;
}
