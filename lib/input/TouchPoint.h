#ifndef TOUCH_POINT_H
#define TOUCH_POINT_H

struct TouchPoint
{
    int x;
    int y;

    static TouchPoint NoContact() { return {-1, -1}; }
};

#endif