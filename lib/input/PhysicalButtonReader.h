#ifndef PHYSICAL_BUTTON_READER_H
#define PHYSICAL_BUTTON_READER_H

#include <vector>
#include "ButtonInput.h"
#include "GpioPinReader.h"
#include "ButtonReader.h"

struct ButtonPin
{
    GpioPinReader &reader;
    ButtonInput button;
};

class PhysicalButtonReader : public ButtonReader
{
private:
    std::vector<ButtonPin> &buttonPins;

public:
    PhysicalButtonReader(std::vector<ButtonPin> &buttonPins) : buttonPins(buttonPins)
    {
    }

    ButtonInput DeduceButton() override
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