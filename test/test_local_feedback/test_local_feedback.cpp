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
    ScreenLocalFeedback screenLocalFeedback{mockScreen};

    EXPECT_CALL(mockScreen, Display(TopBar::Hidden(), ScreenModel{ChosenZoneScreen{GetParam().second}})).Times(1);

    screenLocalFeedback.AcknowledgeAnswer(GetParam().first);
}

INSTANTIATE_TEST_SUITE_P(
    TheFourChoiceLetters,
    AcknowledgeAnswerTest,
    ::testing::Values(ChosenLetterCase{'A', 0}, ChosenLetterCase{'B', 1}, ChosenLetterCase{'C', 2}, ChosenLetterCase{'D', 3}));

TEST(ScreenLocalFeedbackTest, AcknowledgeAnswerWithAnUnknownLetterLeavesTheScreenUnchanged)
{
    MockScreen mockScreen;
    ScreenLocalFeedback screenLocalFeedback{mockScreen};

    EXPECT_CALL(mockScreen, Display(testing::_, testing::_)).Times(0);

    screenLocalFeedback.AcknowledgeAnswer('Z');
}

TEST(ScreenLocalFeedbackTest, AcknowledgeBuzzShowsBuzzEnvoyeOnScreen)
{
    MockScreen mockScreen;
    ScreenLocalFeedback screenLocalFeedback{mockScreen};

    EXPECT_CALL(mockScreen, Display(TopBar::Hidden(), ScreenModel{TextScreen{"Buzz envoyé"}})).Times(1);

    screenLocalFeedback.AcknowledgeBuzz();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
