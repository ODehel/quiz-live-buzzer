#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "BuzzerBehavior.h"
#include "../mocks/GameMocks.h"

class BuzzerBehaviorTest : public ::testing::Test
{
protected:
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
};

TEST_F(BuzzerBehaviorTest, InertBuzzerDoesNotSendAnswerOnButtonPress)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);

    buzzerBehavior.OnMcqPressed('A');
}

TEST_F(BuzzerBehaviorTest, ArmedBuzzerSendsAnswerOnceOnDoubleButtonPress)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnMcqPressed('A');
    buzzerBehavior.OnMcqPressed('A');
}

TEST_F(BuzzerBehaviorTest, SpeedArmedBuzzerSendsBuzzOnceOnDoubleBuzzPress)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzPressed();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, SpeedArmedBuzzerIgnoresMcqPress)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnMcqPressed('A');
}

TEST_F(BuzzerBehaviorTest, McqArmedBuzzerIgnoresBuzzPress)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, TimerEndDisarmsMcqArmedBuzzer)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnTimerEnd();
    buzzerBehavior.OnMcqPressed('A');
}

TEST_F(BuzzerBehaviorTest, SpeedArmedBuzzerSendsBuzzOnBuzzPress)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, BuzzUnlockedSendsBuzzOnBuzzPress)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    buzzerBehavior.OnBuzzUnlocked();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, TimerEndDisarmsBuzzArmedBuzzer)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnTimerEnd();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, EliminatedBuzzerIgnoresButtonPress)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnEliminated();
    buzzerBehavior.OnMcqPressed('A');
}

TEST_F(BuzzerBehaviorTest, EliminatedBuzzerIgnoresBuzzPress)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnEliminated();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, ArmedBuzzerAcknowledgesAnswerOnButtonPress)
{
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer('C')).Times(1);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnMcqPressed('C');
}

TEST_F(BuzzerBehaviorTest, ArmedBuzzerAcknowledgesBuzzOnBuzzPress)
{
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(1);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer(testing::_)).Times(0);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, ArmedBuzzerAcknowledgesBeforeSendingAnswer)
{
    testing::InSequence seq;

    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer('A'));
    EXPECT_CALL(mockHubMessageSender, SendAnswer('A'));

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnMcqPressed('A');
}

TEST_F(BuzzerBehaviorTest, ArmedBuzzerAcknowledgesBeforeSendingBuzz)
{
    testing::InSequence seq;

    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz());
    EXPECT_CALL(mockHubMessageSender, SendBuzz());

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, InertBuzzerIsNotArmed)
{
    const ArmedModeReader &reader = buzzerBehavior;

    EXPECT_FALSE(reader.IsArmed());
}

TEST_F(BuzzerBehaviorTest, QuestionChoicesArmTheBuzzer)
{
    const ArmedModeReader &reader = buzzerBehavior;

    buzzerBehavior.OnQuestionChoices();

    EXPECT_TRUE(reader.IsArmed());
}

TEST_F(BuzzerBehaviorTest, QuestionOpenArmsTheBuzzer)
{
    const ArmedModeReader &reader = buzzerBehavior;

    buzzerBehavior.OnQuestionOpen();

    EXPECT_TRUE(reader.IsArmed());
}

TEST_F(BuzzerBehaviorTest, BuzzUnlockedArmsTheBuzzer)
{
    const ArmedModeReader &reader = buzzerBehavior;

    buzzerBehavior.OnBuzzUnlocked();

    EXPECT_TRUE(reader.IsArmed());
}

TEST_F(BuzzerBehaviorTest, AnswerPressDisarmsTheBuzzer)
{
    const ArmedModeReader &reader = buzzerBehavior;
    buzzerBehavior.OnQuestionChoices();

    buzzerBehavior.OnMcqPressed('A');

    EXPECT_FALSE(reader.IsArmed());
}

TEST_F(BuzzerBehaviorTest, BuzzPressDisarmsTheBuzzer)
{
    const ArmedModeReader &reader = buzzerBehavior;
    buzzerBehavior.OnQuestionOpen();

    buzzerBehavior.OnBuzzPressed();

    EXPECT_FALSE(reader.IsArmed());
}

TEST_F(BuzzerBehaviorTest, TimerEndDisarmsTheBuzzer)
{
    const ArmedModeReader &reader = buzzerBehavior;
    buzzerBehavior.OnQuestionOpen();

    buzzerBehavior.OnTimerEnd();

    EXPECT_FALSE(reader.IsArmed());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}