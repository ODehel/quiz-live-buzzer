#include <gtest/gtest.h>
#include <map>
#include <vector>
#include "FontMetrics.h"
#include "ProportionalTextMeasurer.h"

// Polices factices : tous les glyphes de 0x20 à 0xFF avancent de `advance` pixels, sauf ceux
// qu'on surcharge (code -> avance). Trois tailles, de la plus grande à la plus petite.
static FontMetrics Font(int lineHeight, int advance, std::map<char, int> overrides = {})
{
    std::vector<int> advances(FontMetrics::LastCode - FontMetrics::FirstCode + 1, advance);
    for (const auto &entry : overrides)
        advances[entry.first - FontMetrics::FirstCode] = entry.second;
    return FontMetrics(lineHeight, advances);
}

// 'i' est étroit, 'W' est large, '?' a sa propre avance, les autres glyphes avancent de la valeur par défaut de la taille.
static ProportionalTextMeasurer ThreeSizes()
{
    return ProportionalTextMeasurer({Font(60, 30, {{'i', 10}, {'W', 50}, {'?', 22}}),
                                     Font(48, 24, {{'i', 8}, {'W', 40}, {'?', 17}}),
                                     Font(36, 18, {{'i', 6}, {'W', 30}, {'?', 13}})});
}

TEST(ProportionalTextMeasurerTest, ShouldOfferOneSizePerFont)
{
    EXPECT_EQ(ThreeSizes().SizeCount(), 3);
}

TEST(ProportionalTextMeasurerTest, LineHeightShouldBeTheOneOfTheFontOfThatSize)
{
    const ProportionalTextMeasurer measurer = ThreeSizes();
    EXPECT_EQ(measurer.LineHeight(0), 60);
    EXPECT_EQ(measurer.LineHeight(1), 48);
    EXPECT_EQ(measurer.LineHeight(2), 36);
}

TEST(ProportionalTextMeasurerTest, WidthOfOneGlyphShouldBeItsAdvanceInTheFontOfThatSize)
{
    const ProportionalTextMeasurer measurer = ThreeSizes();
    EXPECT_EQ(measurer.Width("i", 0), 10);
    EXPECT_EQ(measurer.Width("W", 1), 40);
    EXPECT_EQ(measurer.Width("a", 2), 18);
}

TEST(ProportionalTextMeasurerTest, WidthOfATextShouldBeTheSumOfItsGlyphAdvances)
{
    const ProportionalTextMeasurer measurer = ThreeSizes();
    EXPECT_EQ(measurer.Width("iWi", 0), 70);
    EXPECT_EQ(measurer.Width("iWa", 1), 72);
}

TEST(ProportionalTextMeasurerTest, EmptyTextShouldHaveNoWidth)
{
    EXPECT_EQ(ThreeSizes().Width("", 0), 0);
}

TEST(ProportionalTextMeasurerTest, GlyphOutsideTheFontShouldCountAsAQuestionMark)
{
    const ProportionalTextMeasurer measurer = ThreeSizes();
    EXPECT_EQ(measurer.Width("€", 0), 22);
    EXPECT_EQ(measurer.Width("😀", 1), 17);
    EXPECT_EQ(measurer.Width("\n", 2), 13);
    EXPECT_EQ(measurer.Width("i€i", 0), 42);
}

TEST(ProportionalTextMeasurerTest, FirstAndLastGlyphsOfTheFontShouldUseTheirOwnAdvance)
{
    const ProportionalTextMeasurer measurer = ThreeSizes();
    EXPECT_EQ(measurer.Width(" ", 0), 30); // 0x20, premier glyphe
    EXPECT_EQ(measurer.Width("ÿ", 0), 30); // 0xFF, dernier glyphe
}

TEST(ProportionalTextMeasurerTest, CodesJustOutsideTheFontShouldCountAsAQuestionMark)
{
    const ProportionalTextMeasurer measurer = ThreeSizes();
    EXPECT_EQ(measurer.Width("\x1F", 0), 22); // juste avant l'espace
    EXPECT_EQ(measurer.Width("Ā", 0), 22);     // 0x100, juste après ÿ
}

// Police dont trois glyphes hors Latin-1 sont connus : œ (U+0153), ’ (U+2019) et € (U+20AC) ont leur avance,
// les autres codes hors de la police comptent toujours comme un '?'.
static FontMetrics FontWithExtraGlyphs()
{
    std::vector<int> advances(FontMetrics::LastCode - FontMetrics::FirstCode + 1, 18);
    advances['?' - FontMetrics::FirstCode] = 13;
    return FontMetrics(36, advances, {{0x153, 21}, {0x2019, 7}, {0x20AC, 19}});
}

TEST(ProportionalTextMeasurerTest, GlyphOutsideLatin1KnownByTheFontShouldUseItsOwnAdvance)
{
    const ProportionalTextMeasurer measurer({FontWithExtraGlyphs()});
    EXPECT_EQ(measurer.Width("œ", 0), 21);
    EXPECT_EQ(measurer.Width("’", 0), 7);
    EXPECT_EQ(measurer.Width("€", 0), 19);
    EXPECT_EQ(measurer.Width("cœur", 0), 18 + 21 + 18 + 18);
}

TEST(ProportionalTextMeasurerTest, GlyphOutsideLatin1UnknownToTheFontShouldStillCountAsAQuestionMark)
{
    const ProportionalTextMeasurer measurer({FontWithExtraGlyphs()});
    EXPECT_EQ(measurer.Width("Œ", 0), 13); // U+0152, juste avant œ
    EXPECT_EQ(measurer.Width("ŕ", 0), 13); // U+0155, juste après œ
    EXPECT_EQ(measurer.Width("…", 0), 13); // U+2026, absent de cette police
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
