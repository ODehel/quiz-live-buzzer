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