#ifndef MONOSPACE_TEXT_MEASURER_H
#define MONOSPACE_TEXT_MEASURER_H

#include "TextMeasurer.h"
#include "Utf8.h"

// Police à chasse fixe agrandie par un facteur entier (la police intégrée de LovyanGFX) :
// la taille 0 s'affiche à l'échelle MaxScale, la dernière à l'échelle MinScale.
class MonospaceTextMeasurer : public TextMeasurer
{
public:
    enum : int
    {
        MaxScale = 5,
        MinScale = 3
    };

private:
    int cellWidth;
    int cellHeight;

public:
    MonospaceTextMeasurer(int cellWidth, int cellHeight) : cellWidth(cellWidth), cellHeight(cellHeight)
    {
    }

    int SizeCount() const override
    {
        return MaxScale - MinScale + 1;
    }

    int Width(const std::string &text, int size) const override
    {
        return static_cast<int>(Utf8::CharacterCount(text)) * cellWidth * ScaleOf(size);
    }

    int LineHeight(int size) const override
    {
        return cellHeight * ScaleOf(size);
    }

    int ScaleOf(int size) const
    {
        return MaxScale - size;
    }
};

#endif
