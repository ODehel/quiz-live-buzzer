#ifndef PROPORTIONAL_TEXT_MEASURER_H
#define PROPORTIONAL_TEXT_MEASURER_H

#include <string>
#include <vector>
#include "FontMetrics.h"
#include "TextMeasurer.h"
#include "Utf8.h"

// Police proportionnelle : chaque taille est une police dont on connaît les largeurs réelles
// de glyphes. La taille 0 est la plus grande, la dernière la plus petite.
class ProportionalTextMeasurer : public TextMeasurer
{
private:
    std::vector<FontMetrics> fonts;

public:
    ProportionalTextMeasurer(std::vector<FontMetrics> fonts) : fonts(std::move(fonts))
    {
    }

    int SizeCount() const override
    {
        return static_cast<int>(fonts.size());
    }

    int Width(const std::string &text, int size) const override
    {
        int width = 0;
        for (uint32_t code : Utf8::CodePoints(text))
            width += fonts[size].Advance(code);
        return width;
    }

    int LineHeight(int size) const override
    {
        return fonts[size].LineHeight();
    }
};

#endif
