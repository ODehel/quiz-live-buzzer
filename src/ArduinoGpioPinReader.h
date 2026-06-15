#ifndef ARDUINO_GPIO_PIN_READER_H
#define ARDUINO_GPIO_PIN_READER_H

#include "GpioPinReader.h"

class ArduinoGpioPinReader : public GpioPinReader
{
private:
    const int pin;

public:
    ArduinoGpioPinReader(int pin) : pin(pin)
    {
    }

    void Begin();
    bool Read() override;
};

#endif