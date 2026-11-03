#include <gtest/gtest.h>
#include <algorithm>
#include <iterator>
#include <string>
#include <vector>
#include "FontMetrics.h"
#include "GfxFontMetrics.h"
#include "MonospaceTextMeasurer.h"
#include "ProportionalTextMeasurer.h"
#include "TextLayout.h"
#include "../fixtures/OswaldRegular30Advances.h"

using Lines = std::vector<std::string>;

// Dalle 800x480 de la CrowPanel avec une marge de 10 px, police de base 6x8 (LovyanGFX).
// Tailles 0, 1, 2 = échelles 5, 4, 3 : 26 colonnes x 11 lignes ; 32 x 14 ; 43 x 19.
static const int AreaWidth = 780;
static const int AreaHeight = 460;
static const int CellWidth = 6;
static const int CellHeight = 8;

// Une seule taille, un pixel par caractère : la largeur de la zone se lit en colonnes.
class OnePixelPerCharacter : public TextMeasurer
{
public:
    int SizeCount() const override
    {
        return 1;
    }

    int Width(const std::string &text, int) const override
    {
        return static_cast<int>(Utf8::CharacterCount(text));
    }

    int LineHeight(int) const override
    {
        return 1;
    }
};

static Lines WrapAtColumns(const std::string &text, int columns)
{
    OnePixelPerCharacter measurer;
    TextLayout layout(measurer, columns, 1000);
    return layout.Wrap(text, 0);
}

static std::string Repeat(const std::string &unit, int times)
{
    std::string text;
    for (int i = 0; i < times; ++i)
        text += unit;
    return text;
}

TEST(TextLayoutWrapTest, EmptyTextShouldHaveNoLine)
{
    EXPECT_EQ(WrapAtColumns("", 10), Lines{});
}

TEST(TextLayoutWrapTest, TextThatFitsShouldStayOnOneLine)
{
    EXPECT_EQ(WrapAtColumns("Partie en cours", 26), Lines{"Partie en cours"});
}

TEST(TextLayoutWrapTest, TextShouldBreakAtTheLastSpaceThatFits)
{
    EXPECT_EQ(WrapAtColumns("Question QCM en cours", 10), (Lines{"Question", "QCM en", "cours"}));
}

TEST(TextLayoutWrapTest, WordsFillingTheLineExactlyShouldStayTogether)
{
    EXPECT_EQ(WrapAtColumns("ab cd", 5), Lines{"ab cd"});
}

TEST(TextLayoutWrapTest, SpaceBetweenWordsShouldCountAsAColumn)
{
    EXPECT_EQ(WrapAtColumns("ab cd", 4), (Lines{"ab", "cd"}));
}

TEST(TextLayoutWrapTest, EverySpaceOfALineShouldCountAsAColumn)
{
    EXPECT_EQ(WrapAtColumns("a b c d", 5), (Lines{"a b c", "d"}));
}

TEST(TextLayoutWrapTest, WordFillingTheWholeLineShouldLeaveTheNextWordForTheNextLine)
{
    EXPECT_EQ(WrapAtColumns("abcd efgh", 4), (Lines{"abcd", "efgh"}));
}

TEST(TextLayoutWrapTest, WordLongerThanTheLineShouldBeSplit)
{
    EXPECT_EQ(WrapAtColumns("abcdefghij", 4), (Lines{"abcd", "efgh", "ij"}));
}

TEST(TextLayoutWrapTest, RestOfASplitWordShouldShareItsLineWithTheNextWord)
{
    EXPECT_EQ(WrapAtColumns("abcdefghij k", 4), (Lines{"abcd", "efgh", "ij k"}));
}

TEST(TextLayoutWrapTest, WordLongerThanTheLineShouldStartOnANewLine)
{
    EXPECT_EQ(WrapAtColumns("ab cdefghij", 4), (Lines{"ab", "cdef", "ghij"}));
}

TEST(TextLayoutWrapTest, ExtraSpacesShouldBeIgnored)
{
    EXPECT_EQ(WrapAtColumns("  ab   cd  ", 10), Lines{"ab cd"});
}

TEST(TextLayoutWrapTest, AccentedCharacterShouldCountAsOneColumn)
{
    EXPECT_EQ(WrapAtColumns("éé éé", 5), Lines{"éé éé"});
}

TEST(TextLayoutWrapTest, SplitShouldNotCutInsideAnAccentedCharacter)
{
    EXPECT_EQ(WrapAtColumns("ééé", 2), (Lines{"éé", "é"}));
}

TEST(TextLayoutWrapTest, NoColumnShouldBehaveLikeOneColumn)
{
    EXPECT_EQ(WrapAtColumns("ab", 0), (Lines{"a", "b"}));
}

// Police à chasse variable : 'i' et espace font 1 px, tout autre caractère 3 px.
// Deux tailles : la 0 (la plus grande) mesure 2 px de haut par ligne, la 1 en mesure 1 (largeurs inchangées).
class NarrowLettersMeasurer : public TextMeasurer
{
public:
    int SizeCount() const override
    {
        return 2;
    }

    int Width(const std::string &text, int) const override
    {
        int width = 0;
        for (size_t i = 0; i < text.size(); ++i)
            width += (text[i] == 'i' || text[i] == ' ') ? 1 : 3;
        return width;
    }

    int LineHeight(int size) const override
    {
        return 2 - size;
    }
};

TEST(TextLayoutProportionalTest, NarrowLettersShouldLetMoreCharactersFitOnALine)
{
    NarrowLettersMeasurer measurer;
    TextLayout layout(measurer, 6, 1000);
    EXPECT_EQ(layout.Wrap("iiiiii mm", 0), (Lines{"iiiiii", "mm"}));
}

TEST(TextLayoutProportionalTest, WideLettersShouldBreakBeforeTheyWouldInColumns)
{
    NarrowLettersMeasurer measurer;
    TextLayout layout(measurer, 6, 1000);
    EXPECT_EQ(layout.Wrap("mmm", 0), (Lines{"mm", "m"}));
}

TEST(TextLayoutProportionalTest, SpaceShouldBeMeasuredLikeAnyOtherCharacter)
{
    NarrowLettersMeasurer measurer;
    TextLayout layout(measurer, 7, 1000);
    EXPECT_EQ(layout.Wrap("i mm", 0), (Lines{"i", "mm"}));
}

TEST(TextLayoutProportionalTest, ComputeShouldJudgeTheHeightWithTheLineHeightOfEachSize)
{
    NarrowLettersMeasurer measurer;
    TextLayout layout(measurer, 6, 3);
    // "mm mm" fait 2 lignes : 2 x 2 = 4 px en taille 0 (trop haut), 2 x 1 = 2 px en taille 1 (tient)
    TextLines result = layout.Compute("mm mm");
    EXPECT_EQ(result.size, 1);
    EXPECT_EQ(result.lines, (Lines{"mm", "mm"}));
}

TEST(TextLayoutProportionalTest, TextFillingTheHeightExactlyShouldKeepTheSize)
{
    NarrowLettersMeasurer measurer;
    TextLayout layout(measurer, 6, 4);
    // 2 lignes x 2 px = 4 px = la hauteur de la zone
    TextLines result = layout.Compute("mm mm");
    EXPECT_EQ(result.size, 0);
}

class TextLayoutComputeTest : public ::testing::Test
{
protected:
    MonospaceTextMeasurer measurer{CellWidth, CellHeight};
    TextLayout layout{measurer, AreaWidth, AreaHeight};
};

TEST_F(TextLayoutComputeTest, ShortTextShouldUseTheLargestSize)
{
    TextLines result = layout.Compute("Connecté");
    EXPECT_EQ(measurer.ScaleOf(result.size), 5);
    EXPECT_EQ(result.lines, Lines{"Connecté"});
}

TEST_F(TextLayoutComputeTest, TextFillingTheRowsOfTheLargestScaleShouldKeepIt)
{
    TextLines result = layout.Compute(Repeat("x", 26 * 11));
    EXPECT_EQ(measurer.ScaleOf(result.size), 5);
    EXPECT_EQ(result.lines.size(), 11u);
}

TEST_F(TextLayoutComputeTest, TextBeyondTheRowsOfTheLargestScaleShouldUseTheNextScale)
{
    TextLines result = layout.Compute(Repeat("x", 26 * 11 + 1));
    EXPECT_EQ(measurer.ScaleOf(result.size), 4);
    EXPECT_EQ(result.lines.size(), 9u);
}

TEST_F(TextLayoutComputeTest, TextFillingTheRowsOfTheNextScaleShouldKeepIt)
{
    TextLines result = layout.Compute(Repeat("x", 32 * 14));
    EXPECT_EQ(measurer.ScaleOf(result.size), 4);
    EXPECT_EQ(result.lines.size(), 14u);
}

TEST_F(TextLayoutComputeTest, TextBeyondTheRowsOfTheNextScaleShouldUseTheSmallestScale)
{
    TextLines result = layout.Compute(Repeat("x", 32 * 14 + 1));
    EXPECT_EQ(measurer.ScaleOf(result.size), 3);
    EXPECT_EQ(result.lines.size(), 11u);
}

TEST_F(TextLayoutComputeTest, WordTooWideForALargeSizeShouldStayWholeOnASmallerOne)
{
    // 30 caractères : 900 px en échelle 5 (trop large), 720 px en échelle 4 (tient)
    EXPECT_EQ(layout.Wrap(Repeat("x", 30), 1), Lines{Repeat("x", 30)});
}

TEST_F(TextLayoutComputeTest, WordTooWideForALargeSizeShouldShareItsLineOnASmallerOne)
{
    // "a " + 28 x : 840 px pour le mot seul en échelle 5 (trop large), 720 px la ligne entière en échelle 4
    EXPECT_EQ(layout.Wrap("a " + Repeat("x", 28), 1), Lines{"a " + Repeat("x", 28)});
}

TEST_F(TextLayoutComputeTest, TextBeyondTheSmallestScaleShouldKeepItAndOverflow)
{
    TextLines result = layout.Compute(Repeat("x", 43 * 19 + 1));
    EXPECT_EQ(measurer.ScaleOf(result.size), 3);
    EXPECT_EQ(result.lines.size(), 20u);
}

// Contrat de la fonctionnalité : un titre de question (250 caractères au plus, US-005 CA-4)
// tient sur la dalle quelle que soit la longueur de ses mots.
TEST_F(TextLayoutComputeTest, QuestionTitleOfTheMaximumLengthShouldFitOnTheScreenWhateverItsWords)
{
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
        EXPECT_LE(static_cast<int>(result.lines.size()) * measurer.LineHeight(result.size), AreaHeight);
    }
}

// Police proportionnelle factice : 'i' étroit, 'W' large, tous les autres glyphes (espace compris) à `advance`.
static FontMetrics ProportionalFont(int lineHeight, int advance, int narrow, int wide)
{
    std::vector<int> advances(FontMetrics::LastCode - FontMetrics::FirstCode + 1, advance);
    advances['i' - FontMetrics::FirstCode] = narrow;
    advances['W' - FontMetrics::FirstCode] = wide;
    return FontMetrics(lineHeight, advances);
}

TEST(TextLayoutProportionalTest, SameNumberOfCharactersShouldWrapDifferentlyWhenGlyphsAreWider)
{
    ProportionalTextMeasurer measurer({ProportionalFont(10, 20, 10, 50)});
    TextLayout layout(measurer, 130, 1000);
    EXPECT_EQ(layout.Wrap("ii ii", 0), (Lines{"ii ii"}));
    EXPECT_EQ(layout.Wrap("WW WW", 0), (Lines{"WW", "WW"}));
}

TEST(TextLayoutProportionalTest, SizeShouldBeChosenFromGlyphWidthsNotFromTheNumberOfCharacters)
{
    ProportionalTextMeasurer measurer({ProportionalFont(60, 20, 10, 50), ProportionalFont(30, 10, 5, 25)});
    TextLayout layout(measurer, 120, 100);
    const TextLines wide = layout.Compute("WW WW"); // grande taille : 2 lignes de 60 px = 120 > 100
    EXPECT_EQ(wide.size, 1);
    EXPECT_EQ(wide.lines, (Lines{"WW WW"}));
    const TextLines narrow = layout.Compute("ii ii"); // grande taille : 1 ligne de 60 px
    EXPECT_EQ(narrow.size, 0);
    EXPECT_EQ(narrow.lines, (Lines{"ii ii"}));
}

// Largeurs réelles de la plus petite police (Oswald 30 px, interligne réglé à `QuizLineSpacingPercent`). Si un titre de
// 250 caractères tient à cette taille, il tient à toutes : `Compute` ne garde une plus grande taille
// que si le texte y tient aussi.
static ProportionalTextMeasurer SmallestRealFont()
{
    const std::vector<int> advances(std::begin(OswaldRegular30Advances), std::end(OswaldRegular30Advances));
    const int lineHeight = (OswaldRegular30YAdvance * QuizLineSpacingPercent + 50) / 100;
    return ProportionalTextMeasurer({FontMetrics(lineHeight, advances)});
}

static int HeightOf(const TextLines &placed, const TextMeasurer &measurer)
{
    return static_cast<int>(placed.lines.size()) * measurer.LineHeight(placed.size);
}

// Texte de référence : un titre français réaliste, de 250 caractères au plus (US-005 CA-4), avec
// majuscules, chiffres, ponctuation, accents et mots longs. Latin-1 seulement : l'apostrophe est droite.
static const char *const RealisticTitle =
    "Quel navigateur portugais, né à Sabrosa en 1480, a conduit la première expédition à faire le tour de "
    "la Terre pour le compte de l'Espagne, avant de mourir aux Philippines en 1521, sans jamais revoir "
    "Lisbonne ni les îles Moluques de ses rêves ?";

TEST(TextLayoutRealFontTest, RealisticFrenchTitleShouldFitOnTheScreenWithTheRealGlyphWidths)
{
    const ProportionalTextMeasurer measurer = SmallestRealFont();
    const TextLayout layout(measurer, AreaWidth, AreaHeight);
    const std::string title = RealisticTitle;

    EXPECT_GE(Utf8::CharacterCount(title), 240u);
    EXPECT_LE(Utf8::CharacterCount(title), 250u);
    const TextLines placed = layout.Compute(title);
    EXPECT_LE(HeightOf(placed, measurer), AreaHeight);
    EXPECT_EQ(placed.lines.size(), 4u); // largeurs réelles : fige l'étalonnage de la mesure
}

// Constaté au smoke test du 2026-11-03 : à 82 %, le descendant d'une ligne (`q`, `g`, `y`) mord sur l'accent
// de la ligne du dessous. Le pas entre deux lignes doit laisser au moins un pixel entre les deux.
TEST(TextLayoutRealFontTest, LineHeightShouldLeaveRoomBetweenADescenderAndTheAccentOfTheNextLine)
{
    const ProportionalTextMeasurer measurer = SmallestRealFont();

    const int clearance = measurer.LineHeight(0) - OswaldRegular30DescenderDepth - OswaldRegular30AccentHeight;

    EXPECT_GE(clearance, 1);
}

// Même titre tout en majuscules (les majuscules sont les glyphes les plus larges d'un texte réel).
static const char *const RealisticUppercaseTitle =
    "QUEL NAVIGATEUR PORTUGAIS, NÉ À SABROSA EN 1480, A CONDUIT LA PREMIÈRE EXPÉDITION À FAIRE LE TOUR DE "
    "LA TERRE POUR LE COMPTE DE L'ESPAGNE, AVANT DE MOURIR AUX PHILIPPINES EN 1521, SANS JAMAIS REVOIR "
    "LISBONNE NI LES ÎLES MOLUQUES DE SES RÊVES ?";

TEST(TextLayoutRealFontTest, RealisticUppercaseTitleShouldFitOnTheScreenWithTheRealGlyphWidths)
{
    const ProportionalTextMeasurer measurer = SmallestRealFont();
    const TextLayout layout(measurer, AreaWidth, AreaHeight);
    const std::string title = RealisticUppercaseTitle;

    EXPECT_GE(Utf8::CharacterCount(title), 240u);
    EXPECT_LE(Utf8::CharacterCount(title), 250u);
    const TextLines placed = layout.Compute(title);
    EXPECT_LE(HeightOf(placed, measurer), AreaHeight);
    EXPECT_EQ(placed.lines.size(), 4u);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
