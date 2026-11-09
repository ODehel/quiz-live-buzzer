#ifndef ZONE_GRID_H
#define ZONE_GRID_H

#include <cstddef>

struct ZoneRect
{
    int x;
    int y;
    int width;
    int height;
};

class ZoneGrid
{
private:
    int areaWidth;
    int areaTop;
    int areaHeight;

public:
    ZoneGrid(int areaWidth, int areaTop, int areaHeight) : areaWidth(areaWidth), areaTop(areaTop), areaHeight(areaHeight)
    {
    }

    ZoneRect At(size_t index) const
    {
        const int zoneWidth = areaWidth / 2;
        const int zoneHeight = areaHeight / 2;
        return {static_cast<int>(index % 2) * zoneWidth, areaTop + static_cast<int>(index / 2) * zoneHeight, zoneWidth, zoneHeight};
    }
};

#endif
