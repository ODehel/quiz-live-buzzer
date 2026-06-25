#ifndef TOUCH_ZONE_READER_H
#define TOUCH_ZONE_READER_H

#include <vector>
#include "TouchPoint.h"
#include "ButtonInput.h"

struct TouchZone
{
    int left;
    int top;
    int right;
    int bottom;
    ButtonInput button;
};

class TouchZoneReader
{
private:
    std::vector<TouchZone> &zones;

public:
    TouchZoneReader(std::vector<TouchZone> &zones) : zones(zones)
    {
    }

    ButtonInput DeduceButton(TouchPoint point)
    {
        for (TouchZone &zone : zones)
        {
            if (point.x >= zone.left && point.x <= zone.right &&
                point.y >= zone.top && point.y <= zone.bottom)
                return zone.button;
        }
        return ButtonInput::Unknown;
    }
};

#endif