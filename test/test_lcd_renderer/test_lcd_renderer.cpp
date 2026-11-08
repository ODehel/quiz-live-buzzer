#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LcdRenderer.h"
#include "../mocks/ScreenMocks.h"
#include "../mocks/GameMocks.h"

TEST(LcdRendererTest, AuthSuccessMessageShowsConnecteOnScreen)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader};

    EXPECT_CALL(mockScreen, Show("Connecté")).Times(1);

    lcdRenderer.Apply(HubMessage{"auth_success"});
}

using FixedTextCase = std::pair<std::string, std::string>;

class MessageWithFixedTextTest : public ::testing::TestWithParam<FixedTextCase>
{
};

TEST_P(MessageWithFixedTextTest, ShowsItsFixedTextOnScreen)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader};

    EXPECT_CALL(mockScreen, Show(GetParam().second)).Times(1);

    lcdRenderer.Apply(HubMessage{GetParam().first});
}

INSTANTIATE_TEST_SUITE_P(
    MessagesWithAFixedText,
    MessageWithFixedTextTest,
    ::testing::Values(
        FixedTextCase{"answer_received", "Réponse enregistrée"},
        FixedTextCase{"buzz_accepted", "À toi !"},
        FixedTextCase{"buzz_invalidated", "Raté"},
        FixedTextCase{"buzz_unlocked", "À nouveau possible de buzzer"}));

TEST(LcdRendererTimerEndTest, ShowsTooLateWhenAQuestionIsArmed)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    ON_CALL(armedModeReader, IsArmed()).WillByDefault(testing::Return(true));
    LcdRenderer lcdRenderer{mockScreen, armedModeReader};

    EXPECT_CALL(mockScreen, Show("Trop tard")).Times(1);

    lcdRenderer.Apply(HubMessage{"timer_end"});
}

TEST(LcdRendererTimerEndTest, LeavesTheScreenUnchangedWhenNoQuestionIsArmed)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    ON_CALL(armedModeReader, IsArmed()).WillByDefault(testing::Return(false));
    LcdRenderer lcdRenderer{mockScreen, armedModeReader};

    EXPECT_CALL(mockScreen, Show(testing::_)).Times(0);

    lcdRenderer.Apply(HubMessage{"timer_end"});
}

using TimerTickCase = std::pair<int, std::string>;

class TimerTickTest : public ::testing::TestWithParam<TimerTickCase>
{
};

TEST_P(TimerTickTest, ShowsTheRemainingSecondsOnScreen)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader};

    EXPECT_CALL(mockScreen, Show(GetParam().second)).Times(1);

    lcdRenderer.Apply(HubMessage{"timer_tick"}.WithRemainingSeconds(GetParam().first));
}

INSTANTIATE_TEST_SUITE_P(
    RemainingSeconds,
    TimerTickTest,
    ::testing::Values(
        TimerTickCase{25, "25 s"},
        TimerTickCase{5, "5 s"},
        TimerTickCase{0, "0 s"}));

TEST(LcdRendererTimerTickTest, LeavesTheScreenUnchangedWithoutRemainingSeconds)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader};

    EXPECT_CALL(mockScreen, Show(testing::_)).Times(0);

    lcdRenderer.Apply(HubMessage{"timer_tick"});
}

using BuzzLockedCase = std::pair<std::string, std::string>;

class BuzzLockedTest : public ::testing::TestWithParam<BuzzLockedCase>
{
};

TEST_P(BuzzLockedTest, ShowsWhoBuzzedFirstOnScreen)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader};

    EXPECT_CALL(mockScreen, Show(GetParam().second)).Times(1);

    lcdRenderer.Apply(HubMessage{"buzz_locked"}.WithBuzzerUsername(GetParam().first));
}

INSTANTIATE_TEST_SUITE_P(
    BuzzerUsernames,
    BuzzLockedTest,
    ::testing::Values(
        BuzzLockedCase{"Alice", "Alice a buzzé"},
        BuzzLockedCase{"Bob", "Bob a buzzé"}));

TEST(LcdRendererBuzzLockedTest, LeavesTheScreenUnchangedWithoutBuzzerUsername)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader};

    EXPECT_CALL(mockScreen, Show(testing::_)).Times(0);

    lcdRenderer.Apply(HubMessage{"buzz_locked"});
}

using QuestionOpenCase = std::pair<std::string, std::string>;

class QuestionOpenTest : public ::testing::TestWithParam<QuestionOpenCase>
{
};

TEST_P(QuestionOpenTest, ShowsTheQuestionTitleOnScreen)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader};

    EXPECT_CALL(mockScreen, Show(GetParam().second)).Times(1);

    lcdRenderer.Apply(HubMessage{"question_open"}.WithTitle(GetParam().first));
}

INSTANTIATE_TEST_SUITE_P(
    QuestionTitles,
    QuestionOpenTest,
    ::testing::Values(
        QuestionOpenCase{"Quelle est la capitale du Portugal ?", "Quelle est la capitale du Portugal ?"},
        QuestionOpenCase{"Qui a buzzé le premier hier soir ?", "Qui a buzzé le premier hier soir ?"}));

TEST(LcdRendererQuestionOpenTest, LeavesTheScreenUnchangedWithoutTitle)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader};

    EXPECT_CALL(mockScreen, Show(testing::_)).Times(0);

    lcdRenderer.Apply(HubMessage{"question_open"});
}

TEST(LcdRendererQuestionTitleTest, ShowsTheQuestionNumberAndTitleOnScreen)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader};

    EXPECT_CALL(mockScreen, Show("Question 3 : Quel fleuve traverse Lisbonne ?")).Times(1);

    lcdRenderer.Apply(HubMessage{"question_title"}.WithQuestionIndex(2).WithTitle("Quel fleuve traverse Lisbonne ?"));
}

TEST(LcdRendererQuestionTitleTest, LeavesTheScreenUnchangedWithoutQuestionIndex)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader};

    EXPECT_CALL(mockScreen, Show(testing::_)).Times(0);

    lcdRenderer.Apply(HubMessage{"question_title"}.WithTitle("Quel fleuve traverse Lisbonne ?"));
}

class MessageWithoutDisplayTest : public ::testing::TestWithParam<std::string>
{
};

TEST_P(MessageWithoutDisplayTest, LeavesTheScreenUnchanged)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader};

    EXPECT_CALL(mockScreen, Show(testing::_)).Times(0);

    lcdRenderer.Apply(HubMessage{GetParam()});
}

INSTANTIATE_TEST_SUITE_P(
    MessagesLeavingTheScreenUnchanged,
    MessageWithoutDisplayTest,
    ::testing::Values("play_sound_url", "play_system_sound", "token_expiring_soon", "unknown_type"));

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
TEST(LcdRendererTest, QuestionChoicesShowsTheFourChoicesAsZonesOnScreen)
{
    testing::StrictMock<MockScreen> mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader};

    EXPECT_CALL(mockScreen, ShowZones(std::vector<std::string>{"A : Paris", "B : Lyon", "C : Nice", "D : Lille"})).Times(1);

    lcdRenderer.Apply(HubMessage{"question_choices"}.WithChoices({"Paris", "Lyon", "Nice", "Lille"}));
}

TEST(LcdRendererTest, QuestionChoicesWithoutChoicesLeavesTheScreenUnchanged)
{
    testing::StrictMock<MockScreen> mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader};

    lcdRenderer.Apply(HubMessage{"question_choices"});
}
