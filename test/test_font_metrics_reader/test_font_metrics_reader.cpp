#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include "GfxFontMetrics.h"

// Fausse police au format GFXfont : mêmes champs que `lgfx::GFXfont` (first, last, yAdvance,
// glyph[i].xAdvance), le glyphe d'indice i dessinant le code first + i.
struct FakeGlyph
{
    uint8_t xAdvance;
};

struct FakeFont
{
    const FakeGlyph *glyph;
    uint16_t first;
    uint16_t last;
    uint8_t yAdvance;
};

// Avance du glyphe d'indice i : une valeur différente pour chaque indice, jamais nulle.
static int AdvanceOfIndex(int i)
{
    return i % 200 + 1;
}

static std::vector<FakeGlyph> GlyphsFor(int first, int last)
{
    std::vector<FakeGlyph> glyphs;
    for (int code = first; code <= last; ++code)
        glyphs.push_back(FakeGlyph{static_cast<uint8_t>(AdvanceOfIndex(code - first))});
    return glyphs;
}

TEST(GfxFontMetricsTest, AdvanceShouldBeTheOneOfTheGlyphOfThatCode)
{
    const std::vector<FakeGlyph> glyphs = GlyphsFor(0x20, 0xFF);
    const FakeFont font{glyphs.data(), 0x20, 0xFF, 44};

    const FontMetrics metrics = ReadFontMetrics(font);

    EXPECT_EQ(metrics.Advance(0x20), AdvanceOfIndex(0x20 - 0x20));
    EXPECT_EQ(metrics.Advance('a'), AdvanceOfIndex('a' - 0x20));
    EXPECT_EQ(metrics.Advance(0xE9), AdvanceOfIndex(0xE9 - 0x20));
    EXPECT_EQ(metrics.Advance(0xFF), AdvanceOfIndex(0xFF - 0x20));
}

TEST(GfxFontMetricsTest, CodeBeforeTheFirstGlyphShouldAdvanceLikeAQuestionMark)
{
    const std::vector<FakeGlyph> glyphs = GlyphsFor(0x30, 0xFF);
    const FakeFont font{glyphs.data(), 0x30, 0xFF, 44};

    const FontMetrics metrics = ReadFontMetrics(font);

    EXPECT_EQ(metrics.Advance(0x20), AdvanceOfIndex('?' - 0x30));
    EXPECT_EQ(metrics.Advance(0x2F), AdvanceOfIndex('?' - 0x30));
    EXPECT_EQ(metrics.Advance(0x30), AdvanceOfIndex(0));
}

TEST(GfxFontMetricsTest, CodeAfterTheLastGlyphShouldAdvanceLikeAQuestionMark)
{
    const std::vector<FakeGlyph> glyphs = GlyphsFor(0x20, 0x7E);
    const FakeFont font{glyphs.data(), 0x20, 0x7E, 44};

    const FontMetrics metrics = ReadFontMetrics(font);

    EXPECT_EQ(metrics.Advance(0x7E), AdvanceOfIndex(0x7E - 0x20));
    EXPECT_EQ(metrics.Advance(0x7F), AdvanceOfIndex('?' - 0x20));
    EXPECT_EQ(metrics.Advance(0xE9), AdvanceOfIndex('?' - 0x20));
}

TEST(GfxFontMetricsTest, LineHeightShouldBeTheNewlineDistanceOfTheFont)
{
    const std::vector<FakeGlyph> glyphs(0xFF - 0x20 + 1, FakeGlyph{10});
    const FakeFont font{glyphs.data(), 0x20, 0xFF, 44};

    const FontMetrics metrics = ReadFontMetrics(font);

    EXPECT_EQ(metrics.LineHeight(), 44);
}
