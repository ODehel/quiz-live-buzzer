#ifndef PHYSICAL_BUTTON_READER_H
#define PHYSICAL_BUTTON_READER_H

#include "ButtonInput.h"
#include "GpioPinReader.h"
#include "ButtonReader.h"

class PhysicalButtonReader : public ButtonReader
{
private:
    GpioPinReader &buzzPin;

public:
    PhysicalButtonReader(GpioPinReader &buzzPin) : buzzPin(buzzPin)
    {
    }

    ButtonInput DeduceButton() override
    {
        return buzzPin.Read()
                   ? ButtonInput::Buzz
                   : ButtonInput::Unknown;
    }
};

#endif