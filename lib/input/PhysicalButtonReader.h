#ifndef PHYSICAL_BUTTON_READER_H
#define PHYSICAL_BUTTON_READER_H

#include "ButtonInput.h"
#include "GpioPinReader.h"
#include <vector>

struct ButtonPin
{
    GpioPinReader &reader;
    ButtonInput button;
};

class PhysicalButtonReader
{
private:
    std::vector<ButtonPin> &buttonPins;

public:
    PhysicalButtonReader(std::vector<ButtonPin> &buttonPins) : buttonPins(buttonPins)
    {
    }

    ButtonInput DeduceButton()
    {
        ButtonInput candidate = ButtonInput::Unknown;
        int highCount = 0;
        for (ButtonPin &buttonPin : buttonPins)
        {
            if (buttonPin.reader.Read())
            {
                highCount++;
                candidate = buttonPin.button;
            }
        }

        return highCount == 1
                   ? candidate
                   : ButtonInput::Unknown;
    }
};

#endif