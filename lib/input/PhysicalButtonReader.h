#ifndef PHYSICAL_BUTTON_READER_H
#define PHYSICAL_BUTTON_READER_H

#include "PhysicalButton.h"
#include "GpioPinReader.h"
#include <vector>

struct ButtonPin
{
    GpioPinReader &reader;
    PhysicalButton button;
};

class PhysicalButtonReader
{
private:
    std::vector<ButtonPin> &buttonPins;

public:
    PhysicalButtonReader(std::vector<ButtonPin> &buttonPins) : buttonPins(buttonPins)
    {
    }

    PhysicalButton DeduceButton()
    {
        PhysicalButton candidate = PhysicalButton::Unknown;
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
                   : PhysicalButton::Unknown;
    }
};

#endif