#include <gtest/gtest.h>
#include "DisplayState.h"

TEST(DisplayStateTest, AuthSuccessShouldDisplayConnecteOnScreen)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor("auth_success"), "Connecté");
}

TEST(DisplayStateTest, GameResumedShouldDisplayPartieEnCoursOnScreen)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor("game_resumed"), "Partie en cours");
}

TEST(DisplayStateTest, QuestionChoicesShouldDisplayQuestionQcmEnCoursOnScreen)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor("question_choices"), "Question QCM en cours");
}

TEST(DisplayStateTest, AnswerSentShouldDisplayReponseEnvoyeeOnScreen)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextForAnswerSent(), "Réponse envoyée");
}

TEST(DisplayStateTest, BuzzSentShouldDisplayBuzzEnvoyeOnScreen)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextForBuzzSent(), "Buzz envoyé");
}

using FixedTextCase = std::pair<std::string, std::string>;

class MessageWithFixedTextTest : public ::testing::TestWithParam<FixedTextCase>
{
};

TEST_P(MessageWithFixedTextTest, ShouldDisplayItsFixedTextOnScreen)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor(GetParam().first), GetParam().second);
}

INSTANTIATE_TEST_SUITE_P(
    MessagesWithAFixedText,
    MessageWithFixedTextTest,
    ::testing::Values(
        FixedTextCase{"answer_received", "Réponse enregistrée"},
        FixedTextCase{"buzz_accepted", "À toi !"},
        FixedTextCase{"buzz_invalidated", "Raté"},
        FixedTextCase{"buzz_unlocked", "À nouveau possible de buzzer"}));

class MessageWithoutDisplayTest : public ::testing::TestWithParam<std::string>
{
};

TEST_P(MessageWithoutDisplayTest, ShouldHaveNoTextToDisplay)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor(GetParam()), "");
}

INSTANTIATE_TEST_SUITE_P(
    MessagesLeavingTheScreenUnchanged,
    MessageWithoutDisplayTest,
    ::testing::Values("timer_tick", "play_sound_url", "play_system_sound", "token_expiring_soon", "unknown_type"));

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}