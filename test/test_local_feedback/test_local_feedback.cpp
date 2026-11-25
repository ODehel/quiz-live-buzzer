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
    ScreenLocalFeedback screenLocalFeedback{mockScreen, visibleTopBar};

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
    ScreenLocalFeedback screenLocalFeedback{mockScreen, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(testing::_, testing::_)).Times(0);

    screenLocalFeedback.AcknowledgeAnswer('Z');
}

TEST(ScreenLocalFeedbackTest, AcknowledgeBuzzShowsBuzzEnvoyeOnScreen)
{
    MockScreen mockScreen;
    VisibleTopBar visibleTopBar;
    visibleTopBar.Set(TopBar::ForPlayer("Camille"));
    ScreenLocalFeedback screenLocalFeedback{mockScreen, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("Camille"), ScreenModel{TextScreen{"Buzz envoyé"}})).Times(1);

    screenLocalFeedback.AcknowledgeBuzz();
}

TEST(ScreenLocalFeedbackTest, AcknowledgeBuzzKeepsTheCutLinkOfTheVisibleTopBar)
{
    MockScreen mockScreen;
    VisibleTopBar visibleTopBar;
    visibleTopBar.Set(TopBar::ForPlayer("Camille").WithLinkCut());
    ScreenLocalFeedback screenLocalFeedback{mockScreen, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("Camille").WithLinkCut(), ScreenModel{TextScreen{"Buzz envoyé"}})).Times(1);

    screenLocalFeedback.AcknowledgeBuzz();
}

TEST(ScreenLocalFeedbackTest, TheTopBarBeforeAnyAuthSuccessIsTheInitTopBar)
{
    MockScreen mockScreen;
    VisibleTopBar visibleTopBar;
    ScreenLocalFeedback screenLocalFeedback{mockScreen, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(ScreenPresenter{}.TopBarForInit(), ScreenModel{TextScreen{"Buzz envoyé"}})).Times(1);

    screenLocalFeedback.AcknowledgeBuzz();
}

TEST(ScreenLocalFeedbackTest, ATopBarChangedAfterConstructionIsTheOneShownByTheNextFeedback)
{
    MockScreen mockScreen;
    VisibleTopBar visibleTopBar;
    ScreenLocalFeedback screenLocalFeedback{mockScreen, visibleTopBar};
    visibleTopBar.Set(TopBar::ForPlayer("Camille"));

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("Camille"), ScreenModel{ChosenZoneScreen{0}})).Times(1);

    screenLocalFeedback.AcknowledgeAnswer('A');
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
