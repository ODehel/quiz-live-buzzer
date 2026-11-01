#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "LcdRenderer.h"
#include "../mocks/ScreenMocks.h"

TEST(LcdRendererTest, AuthSuccessMessageShowsConnecteOnScreen)
{
    MockScreen mockScreen;
    LcdRenderer lcdRenderer{mockScreen};

    EXPECT_CALL(mockScreen, Show("Connecté")).Times(1);

    lcdRenderer.Apply("auth_success");
}

class MessageWithoutDisplayTest : public ::testing::TestWithParam<std::string>
{
};

TEST_P(MessageWithoutDisplayTest, LeavesTheScreenUnchanged)
{
    MockScreen mockScreen;
    LcdRenderer lcdRenderer{mockScreen};

    EXPECT_CALL(mockScreen, Show(testing::_)).Times(0);

    lcdRenderer.Apply(GetParam());
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