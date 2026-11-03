#ifndef FONT_METRICS_H
#define FONT_METRICS_H

#include <cstdint>
#include <map>
#include <utility>
#include <vector>

// Dimensions d'une police à l'écran, en pixels : hauteur de ligne et avance horizontale de
// chaque glyphe Latin-1 (de 0x20 à 0xFF, dans l'ordre des codes), plus celle des quelques glyphes
// hors Latin-1 que la police dessine (`extraAdvances`, code -> avance : œ, ’, …). Tout autre code
// avance comme `?`.
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
    std::map<uint32_t, int> extraAdvances;

public:
    FontMetrics(int lineHeight, std::vector<int> advances, std::map<uint32_t, int> extraAdvances = {})
        : lineHeight(lineHeight), advances(std::move(advances)), extraAdvances(std::move(extraAdvances))
    {
    }

    int Advance(uint32_t code) const
    {
        if (code >= FirstCode && code <= LastCode)
            return advances[code - FirstCode];
        const auto extra = extraAdvances.find(code);
        if (extra != extraAdvances.end())
            return extra->second;
        return advances['?' - FirstCode];
    }

    int LineHeight() const
    {
        return lineHeight;
    }
};

#endif
