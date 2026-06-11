#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "HubMessageSender.h"
#include "BuzzerBehavior.h"
#include "LocalFeedback.h"

class MockHubMessageSender : public HubMessageSender 
{
public:
    MOCK_METHOD(void, SendAnswer, (char), (override));
    MOCK_METHOD(void, SendBuzz, (), (override));
};

class MockLocalFeedback : public LocalFeedback
{
public:
    MOCK_METHOD(void, Acknowledge, (), (override));
};

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

TEST_F(BuzzerBehaviorTest, ArmedBuzzerAcknowledgesOnButtonPress)
{
    EXPECT_CALL(mockLocalFeedback, Acknowledge()).Times(1);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnMcqPressed('A');
}

TEST_F(BuzzerBehaviorTest, ArmedBuzzerAcknowledgesOnBuzzPress)
{
    EXPECT_CALL(mockLocalFeedback, Acknowledge()).Times(1);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, ArmedBuzzerAcknowledgesBeforeSendingAnswer)
{
    testing::InSequence seq;

    EXPECT_CALL(mockLocalFeedback, Acknowledge());
    EXPECT_CALL(mockHubMessageSender, SendAnswer('A'));

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnMcqPressed('A');
} 

TEST_F(BuzzerBehaviorTest, ArmedBuzzerAcknowledgesBeforeSendingBuzz)
{
    testing::InSequence seq;

    EXPECT_CALL(mockLocalFeedback, Acknowledge());
    EXPECT_CALL(mockHubMessageSender, SendBuzz());

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzPressed();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}