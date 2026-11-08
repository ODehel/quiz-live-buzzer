#include <gtest/gtest.h>
#include <cstdint>
#include <vector>
#include "GfxFontMetrics.h"

// Fausse police au format GFXfont : mêmes champs que `lgfx::GFXfont` (first, last, yAdvance,
// glyph[i].xAdvance, range_num, range). Sans plage (`range_num` à 0), le glyphe d'indice i dessine
// le code first + i. Avec des plages, le code `start` + k dessine le glyphe d'indice `base` + k.
struct FakeGlyph
{
    uint8_t xAdvance;
};

struct FakeEncodeRange
{
    uint16_t start;
    uint16_t end;
    uint16_t base;
};

struct FakeFont
{
    const FakeGlyph *glyph;
    uint16_t first;
    uint16_t last;
    uint8_t yAdvance;
    uint16_t range_num;
    const FakeEncodeRange *range;
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

    const FontMetrics metrics = ReadFontMetrics(font, 100);

    EXPECT_EQ(metrics.Advance(0x20), AdvanceOfIndex(0x20 - 0x20));
    EXPECT_EQ(metrics.Advance('a'), AdvanceOfIndex('a' - 0x20));
    EXPECT_EQ(metrics.Advance(0xE9), AdvanceOfIndex(0xE9 - 0x20));
    EXPECT_EQ(metrics.Advance(0xFF), AdvanceOfIndex(0xFF - 0x20));
}

TEST(GfxFontMetricsTest, CodeBeforeTheFirstGlyphShouldAdvanceLikeAQuestionMark)
{
    const std::vector<FakeGlyph> glyphs = GlyphsFor(0x30, 0xFF);
    const FakeFont font{glyphs.data(), 0x30, 0xFF, 44};

    const FontMetrics metrics = ReadFontMetrics(font, 100);

    EXPECT_EQ(metrics.Advance(0x20), AdvanceOfIndex('?' - 0x30));
    EXPECT_EQ(metrics.Advance(0x2F), AdvanceOfIndex('?' - 0x30));
    EXPECT_EQ(metrics.Advance(0x30), AdvanceOfIndex(0));
}

TEST(GfxFontMetricsTest, CodeAfterTheLastGlyphShouldAdvanceLikeAQuestionMark)
{
    const std::vector<FakeGlyph> glyphs = GlyphsFor(0x20, 0x7E);
    const FakeFont font{glyphs.data(), 0x20, 0x7E, 44};

    const FontMetrics metrics = ReadFontMetrics(font, 100);

    EXPECT_EQ(metrics.Advance(0x7E), AdvanceOfIndex(0x7E - 0x20));
    EXPECT_EQ(metrics.Advance(0x7F), AdvanceOfIndex('?' - 0x20));
    EXPECT_EQ(metrics.Advance(0xE9), AdvanceOfIndex('?' - 0x20));
}

TEST(GfxFontMetricsTest, LineHeightShouldBeTheNewlineDistanceOfTheFont)
{
    const std::vector<FakeGlyph> glyphs(0xFF - 0x20 + 1, FakeGlyph{10});
    const FakeFont font{glyphs.data(), 0x20, 0xFF, 44};

    const FontMetrics metrics = ReadFontMetrics(font, 100);

    EXPECT_EQ(metrics.LineHeight(), 44);
}

TEST(GfxFontMetricsTest, LineHeightShouldBeTheNewlineDistanceScaledByTheLineSpacing)
{
    const std::vector<FakeGlyph> glyphs(0xFF - 0x20 + 1, FakeGlyph{10});
    const FakeFont font{glyphs.data(), 0x20, 0xFF, 100};

    const FontMetrics metrics = ReadFontMetrics(font, 82);

    EXPECT_EQ(metrics.LineHeight(), 82);
}

TEST(GfxFontMetricsTest, ScaledLineHeightShouldBeRoundedToTheNearestPixel)
{
    const std::vector<FakeGlyph> glyphs(0xFF - 0x20 + 1, FakeGlyph{10});
    const FakeFont below{glyphs.data(), 0x20, 0xFF, 56}; // 56 * 82 % = 45,92
    const FakeFont above{glyphs.data(), 0x20, 0xFF, 44}; // 44 * 82 % = 36,08

    EXPECT_EQ(ReadFontMetrics(below, 82).LineHeight(), 46);
    EXPECT_EQ(ReadFontMetrics(above, 82).LineHeight(), 36);
}

// Police comme la production : Latin-1 (indices 0 à 223), puis Œ, œ (indices 224 et 225), ’ (226), … (227) et € (228).
// Les avances des 5 derniers glyphes (91 à 95) ne se confondent avec aucune de celles de Latin-1 (1 à 200).
static std::vector<FakeGlyph> LatinOneThenFiveSymbols()
{
    std::vector<FakeGlyph> glyphs = GlyphsFor(0x20, 0xFF);
    for (uint8_t advance = 91; advance <= 95; ++advance)
        glyphs.push_back(FakeGlyph{advance});
    return glyphs;
}

static const FakeEncodeRange SymbolRanges[] = {{0x20, 0xFF, 0}, {0x152, 0x153, 224}, {0x2019, 0x2019, 226}, {0x2026, 0x2026, 227}, {0x20AC, 0x20AC, 228}};

TEST(GfxFontMetricsTest, AdvanceOfACodeInAnEncodeRangeShouldBeTheOneOfItsGlyph)
{
    const std::vector<FakeGlyph> glyphs = LatinOneThenFiveSymbols();
    const FakeFont font{glyphs.data(), 0x20, 0x20AC, 44, 5, SymbolRanges};

    const FontMetrics metrics = ReadFontMetrics(font, 100);

    EXPECT_EQ(metrics.Advance(0x152), 91);  // Œ
    EXPECT_EQ(metrics.Advance(0x153), 92);  // œ
    EXPECT_EQ(metrics.Advance(0x2019), 93); // ’
    EXPECT_EQ(metrics.Advance(0x2026), 94); // …
    EXPECT_EQ(metrics.Advance(0x20AC), 95); // €
}

TEST(GfxFontMetricsTest, LatinOneShouldKeepItsAdvancesWhenTheFontHasEncodeRanges)
{
    const std::vector<FakeGlyph> glyphs = LatinOneThenFiveSymbols();
    const FakeFont font{glyphs.data(), 0x20, 0x20AC, 44, 5, SymbolRanges};

    const FontMetrics metrics = ReadFontMetrics(font, 100);

    EXPECT_EQ(metrics.Advance(0x20), AdvanceOfIndex(0));
    EXPECT_EQ(metrics.Advance('a'), AdvanceOfIndex('a' - 0x20));
    EXPECT_EQ(metrics.Advance(0xFF), AdvanceOfIndex(0xFF - 0x20));
}

TEST(GfxFontMetricsTest, CodeBetweenFirstAndLastButInNoEncodeRangeShouldAdvanceLikeAQuestionMark)
{
    const std::vector<FakeGlyph> glyphs = LatinOneThenFiveSymbols();
    const FakeFont font{glyphs.data(), 0x20, 0x20AC, 44, 5, SymbolRanges};

    const FontMetrics metrics = ReadFontMetrics(font, 100);

    EXPECT_EQ(metrics.Advance(0x100), AdvanceOfIndex('?' - 0x20));  // juste après ÿ
    EXPECT_EQ(metrics.Advance(0x151), AdvanceOfIndex('?' - 0x20));  // juste avant Œ
    EXPECT_EQ(metrics.Advance(0x154), AdvanceOfIndex('?' - 0x20));  // juste après œ
    EXPECT_EQ(metrics.Advance(0x2018), AdvanceOfIndex('?' - 0x20)); // juste avant ’
    EXPECT_EQ(metrics.Advance(0x20AD), AdvanceOfIndex('?' - 0x20)); // après la dernière plage
}

TEST(GfxFontMetricsTest, CodeOfAnEncodeRangeBeyondTheLastGlyphShouldAdvanceLikeAQuestionMark)
{
    const std::vector<FakeGlyph> glyphs = LatinOneThenFiveSymbols();
    const FakeFont font{glyphs.data(), 0x20, 0x2019, 44, 5, SymbolRanges}; // `last` s'arrête à ’ : … et € sont hors de portée

    const FontMetrics metrics = ReadFontMetrics(font, 100);

    EXPECT_EQ(metrics.Advance(0x2019), 93);                        // ’ reste dessiné
    EXPECT_EQ(metrics.Advance(0x2026), AdvanceOfIndex('?' - 0x20)); // …
    EXPECT_EQ(metrics.Advance(0x20AC), AdvanceOfIndex('?' - 0x20)); // €
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
