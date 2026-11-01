#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ScreenLocalFeedback.h"
#include "../mocks/ScreenMocks.h"

TEST(ScreenLocalFeedbackTest, AcknowledgeAnswerShowsReponseEnvoyeeOnScreen)
{
    MockScreen mockScreen;
    ScreenLocalFeedback screenLocalFeedback{mockScreen};

    EXPECT_CALL(mockScreen, Show("Réponse envoyée")).Times(1);

    screenLocalFeedback.AcknowledgeAnswer();
}

TEST(ScreenLocalFeedbackTest, AcknowledgeBuzzShowsBuzzEnvoyeOnScreen)
{
    MockScreen mockScreen;
    ScreenLocalFeedback screenLocalFeedback{mockScreen};

    EXPECT_CALL(mockScreen, Show("Buzz envoyé")).Times(1);

    screenLocalFeedback.AcknowledgeBuzz();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
