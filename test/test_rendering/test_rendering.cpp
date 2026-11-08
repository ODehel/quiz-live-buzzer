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

TEST(DisplayStateTest, QuestionChoicesShouldGiveOneZoneTextPerChoice)
{
    DisplayState displayState;
    HubMessage message = HubMessage{"question_choices"}.WithChoices({"Paris", "Lyon", "Nice", "Lille"});
    EXPECT_EQ(displayState.ZoneTextsFor(message),
              (std::vector<std::string>{"A : Paris", "B : Lyon", "C : Nice", "D : Lille"}));
}

TEST(DisplayStateTest, QuestionChoicesShouldLabelEachZoneWithItsOwnLetter)
{
    DisplayState displayState;
    HubMessage message = HubMessage{"question_choices"}.WithChoices({"1789", "1792", "1804", "1815"});
    EXPECT_EQ(displayState.ZoneTextsFor(message),
              (std::vector<std::string>{"A : 1789", "B : 1792", "C : 1804", "D : 1815"}));
}

TEST(DisplayStateTest, AMessageOtherThanQuestionChoicesShouldGiveNoZoneText)
{
    DisplayState displayState;
    HubMessage message = HubMessage{"auth_success"}.WithChoices({"Paris", "Lyon", "Nice", "Lille"});
    EXPECT_TRUE(displayState.ZoneTextsFor(message).empty());
}

TEST(DisplayStateTest, QuestionChoicesWithoutChoicesShouldGiveNoZoneText)
{
    DisplayState displayState;
    EXPECT_TRUE(displayState.ZoneTextsFor(HubMessage{"question_choices"}).empty());
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

TEST(DisplayStateTest, McqQuestionResultShouldDisplayTheCorrectAndThePlayerAnswerOnScreen)
{
    DisplayState displayState;
    HubMessage message = HubMessage{"question_result"}.WithCorrectAnswer("B").WithPlayerAnswer("C");
    EXPECT_EQ(displayState.LcdTextFor(message), "Bonne réponse : B\nVotre réponse : C");
}

TEST(DisplayStateTest, McqQuestionResultWithoutPlayerAnswerShouldDisplayOnlyTheCorrectAnswer)
{
    DisplayState displayState;
    HubMessage message = HubMessage{"question_result"}.WithCorrectAnswer("B");
    EXPECT_EQ(displayState.LcdTextFor(message), "Bonne réponse : B");
}

TEST(DisplayStateTest, SpeedQuestionResultThatIsCorrectShouldDisplayValideOnScreen)
{
    DisplayState displayState;
    HubMessage message = HubMessage{"question_result"}.WithCorrectAnswer("Paris").WithCorrect(true);
    EXPECT_EQ(displayState.LcdTextFor(message), "Validé");
}

TEST(DisplayStateTest, SpeedQuestionResultThatIsNotCorrectShouldDisplayRateOnScreen)
{
    DisplayState displayState;
    HubMessage message = HubMessage{"question_result"}.WithCorrectAnswer("Paris").WithCorrect(false);
    EXPECT_EQ(displayState.LcdTextFor(message), "Raté");
}

TEST(DisplayStateTest, QuestionResultWithoutCorrectAnswerShouldLeaveTheScreenUnchanged)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor(HubMessage{"question_result"}.WithCorrect(true).WithPlayerAnswer("C")), "");
}

TEST(DisplayStateTest, SpeedQuestionResultWithoutCorrectFlagShouldLeaveTheScreenUnchanged)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.LcdTextFor(HubMessage{"question_result"}.WithCorrectAnswer("Paris")), "");
}

class McqLetterTest : public ::testing::TestWithParam<std::string>
{
};

TEST_P(McqLetterTest, QuestionResultShouldTreatTheLetterAsAnMcqAnswer)
{
    DisplayState displayState;
    HubMessage message = HubMessage{"question_result"}.WithCorrectAnswer(GetParam());
    EXPECT_EQ(displayState.LcdTextFor(message), "Bonne réponse : " + GetParam());
}

INSTANTIATE_TEST_SUITE_P(
    TheFourChoiceLetters,
    McqLetterTest,
    ::testing::Values("A", "B", "C", "D"));

TEST(DisplayStateTest, SpeedQuestionResultWhoseAnswerStartsWithAChoiceLetterShouldStillBeASpeedResult)
{
    DisplayState displayState;
    HubMessage message = HubMessage{"question_result"}.WithCorrectAnswer("Berlin").WithCorrect(true);
    EXPECT_EQ(displayState.LcdTextFor(message), "Validé");
}

using HighlightCase = std::pair<std::string, size_t>;

class HighlightedZoneTest : public ::testing::TestWithParam<HighlightCase>
{
};

TEST_P(HighlightedZoneTest, McqQuestionResultShouldHighlightTheZoneOfTheCorrectAnswer)
{
    DisplayState displayState;
    HubMessage message = HubMessage{"question_result"}.WithCorrectAnswer(GetParam().first).WithPlayerAnswer("C");
    EXPECT_EQ(displayState.HighlightedZoneFor(message), static_cast<int>(GetParam().second));
}

INSTANTIATE_TEST_SUITE_P(
    TheFourChoiceLetters,
    HighlightedZoneTest,
    ::testing::Values(HighlightCase{"A", 0}, HighlightCase{"B", 1}, HighlightCase{"C", 2}, HighlightCase{"D", 3}));

TEST(DisplayStateTest, SpeedQuestionResultShouldHighlightNoZone)
{
    DisplayState displayState;
    HubMessage message = HubMessage{"question_result"}.WithCorrectAnswer("Paris").WithCorrect(true);
    EXPECT_EQ(displayState.HighlightedZoneFor(message), DisplayState::NoZone);
}

TEST(DisplayStateTest, QuestionResultWithoutCorrectAnswerShouldHighlightNoZone)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.HighlightedZoneFor(HubMessage{"question_result"}.WithPlayerAnswer("C")), DisplayState::NoZone);
}

TEST(DisplayStateTest, AMessageOtherThanQuestionResultShouldHighlightNoZone)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.HighlightedZoneFor(HubMessage{"auth_success"}.WithCorrectAnswer("B")), DisplayState::NoZone);
}

using ChosenLetterCase = std::pair<char, int>;

class ChosenZoneTest : public ::testing::TestWithParam<ChosenLetterCase>
{
};

TEST_P(ChosenZoneTest, AChosenLetterShouldMapToItsZone)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.ZoneIndexForAnswer(GetParam().first), GetParam().second);
}

INSTANTIATE_TEST_SUITE_P(
    TheFourChoiceLetters,
    ChosenZoneTest,
    ::testing::Values(ChosenLetterCase{'A', 0}, ChosenLetterCase{'B', 1}, ChosenLetterCase{'C', 2}, ChosenLetterCase{'D', 3}));

TEST(DisplayStateTest, ACharacterOutsideTheFourLettersShouldChooseNoZone)
{
    DisplayState displayState;
    EXPECT_EQ(displayState.ZoneIndexForAnswer('E'), DisplayState::NoZone);
    EXPECT_EQ(displayState.ZoneIndexForAnswer('@'), DisplayState::NoZone);
    EXPECT_EQ(displayState.ZoneIndexForAnswer('a'), DisplayState::NoZone);
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
TEST(DisplayStateTest, AnswerReceivedShouldKeepTheZonesOnScreen)
{
    DisplayState displayState;
    EXPECT_TRUE(displayState.KeepsZonesFor(HubMessage{"answer_received"}));
}

class MessageThatReplacesTheZonesTest : public ::testing::TestWithParam<std::string>
{
};

TEST_P(MessageThatReplacesTheZonesTest, ShouldNotKeepTheZonesOnScreen)
{
    DisplayState displayState;
    EXPECT_FALSE(displayState.KeepsZonesFor(HubMessage{GetParam()}));
}

INSTANTIATE_TEST_SUITE_P(
    MessagesOtherThanAnswerReceived,
    MessageThatReplacesTheZonesTest,
    ::testing::Values("auth_success", "buzz_accepted", "question_choices", "question_result", "timer_end"));
