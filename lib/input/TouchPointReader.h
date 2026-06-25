#ifndef TOUCH_POINT_READER_H
#define TOUCH_POINT_READER_H

#include "TouchZoneReader.h"

class TouchPointReader 
{
public:
    virtual ~TouchPointReader() = default;
    virtual TouchPoint Read() = 0;
};

#endif