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

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}