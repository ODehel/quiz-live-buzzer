#include <gtest/gtest.h>
#include <string>
#include "GlyphFilter.h"

TEST(GlyphFilterTest, EmojiShouldBeReplacedByAQuestionMark)
{
    EXPECT_EQ(GlyphFilter::ReplaceCodesWithoutGlyph("Un drapeau \U0001F600 ici"), "Un drapeau ? ici");
}

TEST(GlyphFilterTest, EachCodeOfAnEmojiMadeOfSeveralCodesShouldBeReplacedByItsOwnQuestionMark)
{
    // Drapeau du Brésil : deux indicateurs régionaux.
    EXPECT_EQ(GlyphFilter::ReplaceCodesWithoutGlyph("\U0001F1E7\U0001F1F7"), "??");
}

TEST(GlyphFilterTest, CombiningAccentShouldBeReplacedByAQuestionMarkAfterItsLetter)
{
    EXPECT_EQ(GlyphFilter::ReplaceCodesWithoutGlyph("é"), "e?");
}

TEST(GlyphFilterTest, NarrowNoBreakSpaceShouldBeReplacedByAQuestionMark)
{
    EXPECT_EQ(GlyphFilter::ReplaceCodesWithoutGlyph("10 000"), "10?000");
}

TEST(GlyphFilterTest, SymbolsOutsideLatin1ThatTheFontDrawsShouldBeKept)
{
    const std::string symbols = "Œ œ ’ … € – — “ ”";
    EXPECT_EQ(GlyphFilter::ReplaceCodesWithoutGlyph(symbols), symbols);
}

TEST(GlyphFilterTest, ControlCodesShouldBeRemovedExceptTheLineFeed)
{
    EXPECT_EQ(GlyphFilter::ReplaceCodesWithoutGlyph("a\rb\tc\nd"), "abc\nd");
}

TEST(GlyphFilterTest, EveryControlCodeBelowTheSpaceShouldBeRemovedExceptTheLineFeed)
{
    std::string controls;
    for (char code = 0x01; code < 0x20; ++code)
        controls += code;
    EXPECT_EQ(GlyphFilter::ReplaceCodesWithoutGlyph("<" + controls + ">"), "<\n>");
}

TEST(GlyphFilterTest, LatinOneShouldBeKept)
{
    const std::string latinOne = " ~¡«»¿ÀÉÇéçÿ"; // de l'espace (0x20) à ÿ (0xFF)
    EXPECT_EQ(GlyphFilter::ReplaceCodesWithoutGlyph(latinOne), latinOne);
}

TEST(GlyphFilterTest, FirstCodeBeyondLatin1ShouldBeReplacedByAQuestionMark)
{
    EXPECT_EQ(GlyphFilter::ReplaceCodesWithoutGlyph("Ā"), "?");
}

TEST(GlyphFilterTest, CodesNextToTheKeptSymbolsShouldBeReplacedByAQuestionMark)
{
    // Œ œ ’ … € – — “ ” sont gardés ; leurs voisins, non : ‘ (U+2018), ‚ (U+201A), ‹ (U+2039), ™ (U+2122), Ÿ (U+0178).
    EXPECT_EQ(GlyphFilter::ReplaceCodesWithoutGlyph("‘‚‹™Ÿő―„‧₭"), "??????????");
}

TEST(GlyphFilterTest, CodesInsideLatin1WithoutAUsefulGlyphShouldBeKeptAsTheyAre)
{
    // Le CDC fixe la plage Latin-1 comme dessinée : U+007F, U+0080 à U+009F et U+00AD ne sont ni remplacés ni retirés.
    const std::string kept = "\x7F\xC2\x80\xC2\x90\xC2\x9F\xC2\xAD"; // U+007F, U+0080, U+0090, U+009F, U+00AD
    EXPECT_EQ(GlyphFilter::ReplaceCodesWithoutGlyph(kept), kept);
}

TEST(GlyphFilterTest, TextWithoutAnyCodeToReplaceShouldBeUnchanged)
{
    EXPECT_EQ(GlyphFilter::ReplaceCodesWithoutGlyph(""), "");
    EXPECT_EQ(GlyphFilter::ReplaceCodesWithoutGlyph("Quelle est la capitale de la France ?"), "Quelle est la capitale de la France ?");
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
