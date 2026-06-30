#include <gtest/gtest.h>
#include "DisplayState.h"

TEST(DisplayStateTest, AuthSuccessShouldDisplayConnecteOnScreen)
{
    DisplayState displayState;
    displayState.Apply("auth_success");
    EXPECT_EQ(displayState.LcdText(), "Connecté");
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}