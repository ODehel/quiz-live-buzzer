#ifndef FONT_METRICS_H
#define FONT_METRICS_H

#include <cstdint>
#include <utility>
#include <vector>

// Dimensions d'une police à l'écran, en pixels : hauteur de ligne et avance horizontale de
// chaque glyphe Latin-1 (de 0x20 à 0xFF, dans l'ordre des codes).
class FontMetrics
{
public:
    enum : int
    {
        FirstCode = 0x20,
        LastCode = 0xFF
    };

private:
    int lineHeight;
    std::vector<int> advances;

public:
    FontMetrics(int lineHeight, std::vector<int> advances) : lineHeight(lineHeight), advances(std::move(advances))
    {
    }

    int Advance(uint32_t code) const
    {
        if (code < FirstCode || code > LastCode)
            return advances['?' - FirstCode];
        return advances[code - FirstCode];
    }

    int LineHeight() const
    {
        return lineHeight;
    }
};

#endif
