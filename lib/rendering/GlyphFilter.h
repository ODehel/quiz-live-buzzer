#ifndef GLYPH_FILTER_H
#define GLYPH_FILTER_H

#include <cstdint>
#include <string>
#include "Utf8.h"

namespace GlyphFilter
{
    // Les codes au-delà de Latin-1 que les polices dessinent : `EXTRA_CODES` de `scripts/generate-gfx-font.py`
    // (Œ œ – — ’ “ ” … €). À tenir d'accord avec lui.
    inline bool IsDrawnBeyondLatin1(uint32_t code)
    {
        switch (code)
        {
        case 0x0152: // Œ
        case 0x0153: // œ
        case 0x2013: // –
        case 0x2014: // —
        case 0x2019: // ’
        case 0x201C: // “
        case 0x201D: // ”
        case 0x2026: // …
        case 0x20AC: // €
            return true;
        default:
            return false;
        }
    }

    inline bool HasGlyph(uint32_t code)
    {
        return code <= 0xFF || IsDrawnBeyondLatin1(code);
    }

    inline std::string ReplaceCodesWithoutGlyph(const std::string &text)
    {
        std::string filtered;
        size_t i = 0;
        while (i < text.size())
        {
            const size_t length = 1 + Utf8::ContinuationCount(static_cast<unsigned char>(text[i]));
            const std::string character = text.substr(i, length);
            i += character.size();
            const uint32_t code = Utf8::CodePoints(character).front();
            if (code < 0x20 && code != '\n')
                continue;
            if (HasGlyph(code))
                filtered += character;
            else
                filtered += '?';
        }
        return filtered;
    }
}

#endif
