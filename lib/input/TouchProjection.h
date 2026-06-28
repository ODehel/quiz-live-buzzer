#ifndef TOUCH_PROJECTION_H
#define TOUCH_PROJECTION_H

#include "TouchPoint.h"

class TouchProjection
{
private:
    TouchPoint sensorOrigin;
    TouchPoint sensorResolution;

    int MapAxis(int value, int inMin, int inMax, int outMin, int outMax) const
    {
        return (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
    }

public:
    TouchProjection(TouchPoint sensorOrigin, TouchPoint sensorResolution) : sensorOrigin(sensorOrigin), sensorResolution(sensorResolution)
    {
    }

    TouchPoint Project(TouchPoint touchPoint)
    {
        int screenMaxX = sensorResolution.x - 1;
        int screenMaxY = sensorResolution.y - 1;
        return {
            MapAxis(touchPoint.x, sensorResolution.x, sensorOrigin.x, 0, screenMaxX),
            MapAxis(touchPoint.y, sensorResolution.y, sensorOrigin.y, 0, screenMaxY)};
    }
};

#endif