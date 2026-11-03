#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "TextLayout.h"

using Lines = std::vector<std::string>;

// Dalle 800x480 de la CrowPanel avec une marge de 10 px, police de base 6x8 (LovyanGFX).
// À l'échelle 5 : 26 colonnes x 11 lignes ; à 4 : 32 x 14 ; à 3 : 43 x 19.
static const int AreaWidth = 780;
static const int AreaHeight = 460;
static const int CellWidth = 6;
static const int CellHeight = 8;

static std::string Repeat(const std::string &unit, int times)
{
    std::string text;
    for (int i = 0; i < times; ++i)
        text += unit;
    return text;
}

TEST(TextLayoutWrapTest, EmptyTextShouldHaveNoLine)
{
    EXPECT_EQ(TextLayout::Wrap("", 10), Lines{});
}

TEST(TextLayoutWrapTest, TextThatFitsShouldStayOnOneLine)
{
    EXPECT_EQ(TextLayout::Wrap("Partie en cours", 26), Lines{"Partie en cours"});
}

TEST(TextLayoutWrapTest, TextShouldBreakAtTheLastSpaceThatFits)
{
    EXPECT_EQ(TextLayout::Wrap("Question QCM en cours", 10), (Lines{"Question", "QCM en", "cours"}));
}

TEST(TextLayoutWrapTest, WordsFillingTheLineExactlyShouldStayTogether)
{
    EXPECT_EQ(TextLayout::Wrap("ab cd", 5), Lines{"ab cd"});
}

TEST(TextLayoutWrapTest, SpaceBetweenWordsShouldCountAsAColumn)
{
    EXPECT_EQ(TextLayout::Wrap("ab cd", 4), (Lines{"ab", "cd"}));
}

TEST(TextLayoutWrapTest, EverySpaceOfALineShouldCountAsAColumn)
{
    EXPECT_EQ(TextLayout::Wrap("a b c d", 5), (Lines{"a b c", "d"}));
}

TEST(TextLayoutWrapTest, WordFillingTheWholeLineShouldLeaveTheNextWordForTheNextLine)
{
    EXPECT_EQ(TextLayout::Wrap("abcd efgh", 4), (Lines{"abcd", "efgh"}));
}

TEST(TextLayoutWrapTest, WordLongerThanTheLineShouldBeSplit)
{
    EXPECT_EQ(TextLayout::Wrap("abcdefghij", 4), (Lines{"abcd", "efgh", "ij"}));
}

TEST(TextLayoutWrapTest, RestOfASplitWordShouldShareItsLineWithTheNextWord)
{
    EXPECT_EQ(TextLayout::Wrap("abcdefghij k", 4), (Lines{"abcd", "efgh", "ij k"}));
}

TEST(TextLayoutWrapTest, WordLongerThanTheLineShouldStartOnANewLine)
{
    EXPECT_EQ(TextLayout::Wrap("ab cdefghij", 4), (Lines{"ab", "cdef", "ghij"}));
}

TEST(TextLayoutWrapTest, ExtraSpacesShouldBeIgnored)
{
    EXPECT_EQ(TextLayout::Wrap("  ab   cd  ", 10), Lines{"ab cd"});
}

TEST(TextLayoutWrapTest, AccentedCharacterShouldCountAsOneColumn)
{
    EXPECT_EQ(TextLayout::Wrap("éé éé", 5), Lines{"éé éé"});
}

TEST(TextLayoutWrapTest, SplitShouldNotCutInsideAnAccentedCharacter)
{
    EXPECT_EQ(TextLayout::Wrap("ééé", 2), (Lines{"éé", "é"}));
}

TEST(TextLayoutWrapTest, NoColumnShouldBehaveLikeOneColumn)
{
    EXPECT_EQ(TextLayout::Wrap("ab", 0), (Lines{"a", "b"}));
}

TEST(TextLayoutComputeTest, ShortTextShouldUseTheLargestScale)
{
    TextLayout layout{AreaWidth, AreaHeight, CellWidth, CellHeight};
    TextLines result = layout.Compute("Connecté");
    EXPECT_EQ(result.scale, 5);
    EXPECT_EQ(result.lines, Lines{"Connecté"});
}

TEST(TextLayoutComputeTest, TextFillingTheRowsOfTheLargestScaleShouldKeepIt)
{
    TextLayout layout{AreaWidth, AreaHeight, CellWidth, CellHeight};
    TextLines result = layout.Compute(Repeat("x", 26 * 11));
    EXPECT_EQ(result.scale, 5);
    EXPECT_EQ(result.lines.size(), 11u);
}

TEST(TextLayoutComputeTest, TextBeyondTheRowsOfTheLargestScaleShouldUseTheNextScale)
{
    TextLayout layout{AreaWidth, AreaHeight, CellWidth, CellHeight};
    TextLines result = layout.Compute(Repeat("x", 26 * 11 + 1));
    EXPECT_EQ(result.scale, 4);
    EXPECT_EQ(result.lines.size(), 9u);
}

TEST(TextLayoutComputeTest, TextFillingTheRowsOfTheNextScaleShouldKeepIt)
{
    TextLayout layout{AreaWidth, AreaHeight, CellWidth, CellHeight};
    TextLines result = layout.Compute(Repeat("x", 32 * 14));
    EXPECT_EQ(result.scale, 4);
    EXPECT_EQ(result.lines.size(), 14u);
}

TEST(TextLayoutComputeTest, TextBeyondTheRowsOfTheNextScaleShouldUseTheSmallestScale)
{
    TextLayout layout{AreaWidth, AreaHeight, CellWidth, CellHeight};
    TextLines result = layout.Compute(Repeat("x", 32 * 14 + 1));
    EXPECT_EQ(result.scale, 3);
    EXPECT_EQ(result.lines.size(), 11u);
}

TEST(TextLayoutComputeTest, TextBeyondTheSmallestScaleShouldKeepItAndOverflow)
{
    TextLayout layout{AreaWidth, AreaHeight, CellWidth, CellHeight};
    TextLines result = layout.Compute(Repeat("x", 43 * 19 + 1));
    EXPECT_EQ(result.scale, 3);
    EXPECT_EQ(result.lines.size(), 20u);
}

// Contrat de la fonctionnalité : un titre de question (250 caractères au plus, US-005 CA-4)
// tient sur la dalle quelle que soit la longueur de ses mots.
TEST(TextLayoutComputeTest, QuestionTitleOfTheMaximumLengthShouldFitOnTheScreenWhateverItsWords)
{
    TextLayout layout{AreaWidth, AreaHeight, CellWidth, CellHeight};
    std::vector<std::string> titles;
    for (int wordLength = 1; wordLength <= 60; ++wordLength)
        titles.push_back(Repeat(Repeat("x", wordLength) + " ", 250).substr(0, 250));
    for (int wordLength = 1; wordLength <= 60; ++wordLength)
        titles.push_back(Repeat(Repeat("x", wordLength) + " a ", 250).substr(0, 250));
    titles.push_back(Repeat("é", 250));

    for (const std::string &title : titles)
    {
        SCOPED_TRACE(title);
        TextLines result = layout.Compute(title);
        EXPECT_LE(static_cast<int>(result.lines.size()) * CellHeight * result.scale, AreaHeight);
    }
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
