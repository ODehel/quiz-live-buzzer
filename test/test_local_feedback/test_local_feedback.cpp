#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ScreenLocalFeedback.h"
#include "../mocks/ScreenMocks.h"

using ChosenLetterCase = std::pair<char, size_t>;

class AcknowledgeAnswerTest : public ::testing::TestWithParam<ChosenLetterCase>
{
};

TEST_P(AcknowledgeAnswerTest, AcknowledgeAnswerHighlightsTheChosenZoneWithoutText)
{
    MockScreen mockScreen;
    VisibleTopBar visibleTopBar;
    visibleTopBar.Set(TopBar::ForPlayer("Camille"));
    FakeLastScreen lastScreen;
    ScreenLocalFeedback screenLocalFeedback{mockScreen, visibleTopBar, lastScreen};

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("Camille"), ScreenModel{ChosenZoneScreen{GetParam().second}})).Times(1);

    screenLocalFeedback.AcknowledgeAnswer(GetParam().first);
}

INSTANTIATE_TEST_SUITE_P(
    TheFourChoiceLetters,
    AcknowledgeAnswerTest,
    ::testing::Values(ChosenLetterCase{'A', 0}, ChosenLetterCase{'B', 1}, ChosenLetterCase{'C', 2}, ChosenLetterCase{'D', 3}));

TEST(ScreenLocalFeedbackTest, AcknowledgeAnswerWithAnUnknownLetterLeavesTheScreenUnchanged)
{
    MockScreen mockScreen;
    VisibleTopBar visibleTopBar;
    FakeLastScreen lastScreen;
    ScreenLocalFeedback screenLocalFeedback{mockScreen, visibleTopBar, lastScreen};

    EXPECT_CALL(mockScreen, Display(testing::_, testing::_)).Times(0);

    screenLocalFeedback.AcknowledgeAnswer('Z');
}

TEST(ScreenLocalFeedbackTest, AcknowledgeBuzzShowsBuzzEnvoyeOnScreen)
{
    MockScreen mockScreen;
    VisibleTopBar visibleTopBar;
    visibleTopBar.Set(TopBar::ForPlayer("Camille"));
    FakeLastScreen lastScreen;
    ScreenLocalFeedback screenLocalFeedback{mockScreen, visibleTopBar, lastScreen};

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("Camille"), ScreenModel{TextScreen{"Buzz envoyé"}})).Times(1);

    screenLocalFeedback.AcknowledgeBuzz();
}

TEST(ScreenLocalFeedbackTest, AcknowledgeBuzzKeepsTheCutLinkOfTheVisibleTopBar)
{
    MockScreen mockScreen;
    VisibleTopBar visibleTopBar;
    visibleTopBar.Set(TopBar::ForPlayer("Camille").WithLinkCut());
    FakeLastScreen lastScreen;
    ScreenLocalFeedback screenLocalFeedback{mockScreen, visibleTopBar, lastScreen};

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("Camille").WithLinkCut(), ScreenModel{TextScreen{"Buzz envoyé"}})).Times(1);

    screenLocalFeedback.AcknowledgeBuzz();
}

TEST(ScreenLocalFeedbackTest, TheTopBarBeforeAnyAuthSuccessIsTheInitTopBar)
{
    MockScreen mockScreen;
    VisibleTopBar visibleTopBar;
    FakeLastScreen lastScreen;
    ScreenLocalFeedback screenLocalFeedback{mockScreen, visibleTopBar, lastScreen};

    EXPECT_CALL(mockScreen, Display(ScreenPresenter{}.TopBarForInit(), ScreenModel{TextScreen{"Buzz envoyé"}})).Times(1);

    screenLocalFeedback.AcknowledgeBuzz();
}

TEST(ScreenLocalFeedbackTest, ATopBarChangedAfterConstructionIsTheOneShownByTheNextFeedback)
{
    MockScreen mockScreen;
    VisibleTopBar visibleTopBar;
    FakeLastScreen lastScreen;
    ScreenLocalFeedback screenLocalFeedback{mockScreen, visibleTopBar, lastScreen};
    visibleTopBar.Set(TopBar::ForPlayer("Camille"));

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("Camille"), ScreenModel{ChosenZoneScreen{0}})).Times(1);

    screenLocalFeedback.AcknowledgeAnswer('A');
}

TEST(ScreenLocalFeedbackTest, RestoreScreenBeforePressShowsTheQuestionZonesAgainAfterAnAnswerPress)
{
    MockScreen mockScreen;
    VisibleTopBar visibleTopBar;
    visibleTopBar.Set(TopBar::ForPlayer("Camille"));
    const ScreenModel zones = ZonesScreen{{"A : un", "B : deux", "C : trois", "D : quatre"}};
    FakeLastScreen lastScreen;
    ScreenLocalFeedback screenLocalFeedback{mockScreen, visibleTopBar, lastScreen};
    lastScreen.Set(zones);

    screenLocalFeedback.AcknowledgeAnswer('C');
    lastScreen.Set(ChosenZoneScreen{2, {"A : un", "B : deux", "C : trois", "D : quatre"}});

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("Camille"), zones)).Times(1);

    screenLocalFeedback.RestoreScreenBeforePress();
}

TEST(ScreenLocalFeedbackTest, RestoreScreenBeforePressShowsTheOpenQuestionAgainAfterABuzzPress)
{
    MockScreen mockScreen;
    VisibleTopBar visibleTopBar;
    visibleTopBar.Set(TopBar::ForPlayer("Camille"));
    FakeLastScreen lastScreen;
    ScreenLocalFeedback screenLocalFeedback{mockScreen, visibleTopBar, lastScreen};
    lastScreen.Set(TextScreen{"Quel est le plus long fleuve ?"});

    screenLocalFeedback.AcknowledgeBuzz();
    lastScreen.Set(TextScreen{"Buzz envoyé"});

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("Camille"), ScreenModel{TextScreen{"Quel est le plus long fleuve ?"}})).Times(1);

    screenLocalFeedback.RestoreScreenBeforePress();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
