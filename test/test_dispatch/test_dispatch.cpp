#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../mocks/GameMocks.h"
#include "BuzzerBehavior.h"
#include "HubMessageDispatcher.h"

TEST(HubMessageDispatcherTest, QuestionOpenArmsSpeed)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    dispatcher.Dispatch("question_open");
    buzzerBehavior.OnBuzzPressed();
}

TEST(HubMessageDispatcherTest, QuestionChoicesArmsMcq)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);

    dispatcher.Dispatch("question_choices");
    buzzerBehavior.OnMcqPressed('A');
}

TEST(HubMessageDispatcherTest, TimerEndDisarmsArmedBuzzer)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    
    dispatcher.Dispatch("question_open");
    dispatcher.Dispatch("timer_end");
    buzzerBehavior.OnBuzzPressed();
}

TEST(HubMessageDispatcherTest, BuzzUnlockedArmsSpeed)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    dispatcher.Dispatch("buzz_unlocked");
    buzzerBehavior.OnBuzzPressed();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(HubMessageDispatcherTest, BuzzAcceptedKeepsSpeedArmed)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    dispatcher.Dispatch("question_open");
    dispatcher.Dispatch("buzz_accepted");

    EXPECT_TRUE(buzzerBehavior.IsArmed());
    buzzerBehavior.OnBuzzPressed();
}

TEST(HubMessageDispatcherTest, BuzzInvalidatedKeepsSpeedArmed)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    dispatcher.Dispatch("question_open");
    dispatcher.Dispatch("buzz_invalidated");

    EXPECT_TRUE(buzzerBehavior.IsArmed());
    buzzerBehavior.OnBuzzPressed();
}

TEST(HubMessageDispatcherTest, BuzzAcceptedKeepsInertBuzzerInert)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);

    dispatcher.Dispatch("buzz_accepted");

    EXPECT_FALSE(buzzerBehavior.IsArmed());
    buzzerBehavior.OnBuzzPressed();
}

TEST(HubMessageDispatcherTest, BuzzInvalidatedKeepsInertBuzzerInert)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);

    dispatcher.Dispatch("buzz_invalidated");

    EXPECT_FALSE(buzzerBehavior.IsArmed());
    buzzerBehavior.OnBuzzPressed();
}