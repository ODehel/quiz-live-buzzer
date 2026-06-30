#include <gtest/gtest.h>
#include "DisplayState.h"

TEST(DisplayStateTest, AuthSuccessShouldDisplayConnecteOnScreen)
{
    DisplayState displayState;
    displayState.Apply("auth_success");
    EXPECT_EQ(displayState.LcdText(), "Connecté");
}

TEST(DisplayStateTest, GameResumedShouldDisplayPartieEnCoursOnScreen)
{
    DisplayState displayState;
    displayState.Apply("game_resumed");
    EXPECT_EQ(displayState.LcdText(), "Partie en cours");
}

TEST(DisplayStateTest, QuestionChoicesShouldDisplayQuestionQcmEnCoursOnScreen)
{
    DisplayState displayState;
    displayState.Apply("question_choices");
    EXPECT_EQ(displayState.LcdText(), "Question QCM en cours");
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}