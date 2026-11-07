#include <gtest/gtest.h>
#include "DisplayState.h"

TEST(DisplayStateTest, AuthSuccessShouldDisplayConnecteOnScreen)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor(HubMessage{"auth_success"}), "Connecté");
}

TEST(DisplayStateTest, GameResumedShouldDisplayPartieEnCoursOnScreen)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor(HubMessage{"game_resumed"}), "Partie en cours");
}

TEST(DisplayStateTest, QuestionChoicesShouldDisplayTheFourChoicesOnScreen)
{
    DisplayState displayState;
    HubMessage message = HubMessage{"question_choices"}.WithChoices({"Paris", "Lyon", "Nice", "Lille"});
    EXPECT_EQ(displayState.LcdTextFor(message), "A : Paris\nB : Lyon\nC : Nice\nD : Lille");
}

TEST(DisplayStateTest, QuestionChoicesWithoutChoicesShouldLeaveTheScreenUnchanged)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor(HubMessage{"question_choices"}), "");
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

TEST(DisplayStateTest, BuzzLockedShouldDisplayWhoBuzzedOnScreen)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor(HubMessage{"buzz_locked"}.WithBuzzerUsername("Alice")), "Alice a buzzé");
}

TEST(DisplayStateTest, TimerTickShouldDisplayTheRemainingSecondsOnScreen)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor(HubMessage{"timer_tick"}.WithRemainingSeconds(25)), "25 s");
}

TEST(DisplayStateTest, TimerTickWithoutRemainingSecondsShouldHaveNoTextToDisplay)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor(HubMessage{"timer_tick"}), "");
}

TEST(DisplayStateTest, BuzzLockedWithoutBuzzerUsernameShouldHaveNoTextToDisplay)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor(HubMessage{"buzz_locked"}), "");
}

TEST(DisplayStateTest, QuestionOpenShouldDisplayTheQuestionTitleOnScreen)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor(HubMessage{"question_open"}.WithTitle("Quelle est la capitale du Portugal ?")), "Quelle est la capitale du Portugal ?");
}

TEST(DisplayStateTest, QuestionOpenWithoutTitleShouldHaveNoTextToDisplay)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor(HubMessage{"question_open"}), "");
}

TEST(DisplayStateTest, QuestionTitleShouldDisplayTheQuestionNumberAndTitleOnScreen)
{
    DisplayState displayState;
    HubMessage message = HubMessage{"question_title"}.WithQuestionIndex(0).WithTitle("Quelle est la capitale de la France ?");
    EXPECT_EQ(displayState.LcdTextFor(message), "Question 1 : Quelle est la capitale de la France ?");
}

TEST(DisplayStateTest, QuestionTitleShouldNumberTheQuestionFromItsIndexPlusOne)
{
    DisplayState displayState;
    HubMessage message = HubMessage{"question_title"}.WithQuestionIndex(4).WithTitle("Quel fleuve traverse Lisbonne ?");
    EXPECT_EQ(displayState.LcdTextFor(message), "Question 5 : Quel fleuve traverse Lisbonne ?");
}

TEST(DisplayStateTest, QuestionTitleWithoutQuestionIndexShouldHaveNoTextToDisplay)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor(HubMessage{"question_title"}.WithTitle("Quel fleuve traverse Lisbonne ?")), "");
}

TEST(DisplayStateTest, QuestionTitleWithoutTitleShouldHaveNoTextToDisplay)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor(HubMessage{"question_title"}.WithQuestionIndex(0)), "");
}

using FixedTextCase = std::pair<std::string, std::string>;

class MessageWithFixedTextTest : public ::testing::TestWithParam<FixedTextCase>
{
};

TEST_P(MessageWithFixedTextTest, ShouldDisplayItsFixedTextOnScreen)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor(HubMessage{GetParam().first}), GetParam().second);
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
    EXPECT_EQ(displayState.LcdTextFor(HubMessage{GetParam()}), "");
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