#ifndef TOUCH_BUTTON_READER_H
#define TOUCH_BUTTON_READER_H

#include "ButtonReader.h"
#include "TouchPointReader.h"
#include "TouchZoneReader.h"

class TouchButtonReader : public ButtonReader
{
private:
    TouchPointReader &source;
    TouchZoneReader &zones;
public:
    TouchButtonReader(TouchPointReader &source, TouchZoneReader &zones) : source(source), zones(zones)
    {
    }

    ButtonInput DeduceButton() override
    {
        return zones.DeduceButton(source.Read());
    }
};

#endif