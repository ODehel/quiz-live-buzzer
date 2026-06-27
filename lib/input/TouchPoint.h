#ifndef TOUCH_POINT_H
#define TOUCH_POINT_H

#include <ostream>

struct TouchPoint
{
    int x;
    int y;

    bool operator==(const TouchPoint &other) const
    {
        return x == other.x && y == other.y;
    }

    static TouchPoint NoContact() { return {-1, -1}; }
};

inline std::ostream &operator<<(std::ostream &os, const TouchPoint &point)
{
    return os << "{" << point.x << ", " << point.y << "}";
}

#endif