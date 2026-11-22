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
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnMcqPressed('A');
}

TEST_F(BuzzerBehaviorTest, McqArmedBuzzerIgnoresBuzzPress)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, McqArmedBuzzerStaysMcqArmedOnBuzzPress)
{
    const ArmedModeReader &reader = buzzerBehavior;
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer('A')).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnBuzzPressed();

    EXPECT_TRUE(reader.IsArmed());
    buzzerBehavior.OnMcqPressed('A');
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

TEST_F(BuzzerBehaviorTest, SpeedArmedBuzzerIgnoresQuestionChoices)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnMcqPressed('A');
}

TEST_F(BuzzerBehaviorTest, SpeedArmedBuzzerStaysSpeedArmedOnQuestionChoices)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, McqArmedBuzzerIgnoresQuestionOpen)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, McqArmedBuzzerStaysMcqArmedOnQuestionOpen)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnMcqPressed('A');
}

TEST_F(BuzzerBehaviorTest, McqArmedBuzzerIgnoresBuzzUnlocked)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnBuzzUnlocked();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, McqArmedBuzzerStaysMcqArmedOnBuzzUnlocked)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnBuzzUnlocked();
    buzzerBehavior.OnMcqPressed('A');
}

TEST_F(BuzzerBehaviorTest, QuestionChoicesArmsAgainAfterAnAnswer)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendAnswer('B')).Times(1);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnMcqPressed('A');
    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnMcqPressed('B');
}

TEST_F(BuzzerBehaviorTest, BuzzUnlockedArmsAgainAfterABuzz)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(2);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzPressed();
    buzzerBehavior.OnBuzzUnlocked();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, QuestionOpenArmsAgainAfterABuzz)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(2);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzPressed();
    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, QuestionChoicesArmsAgainAfterTimerEnd)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer('D')).Times(1);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnTimerEnd();
    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnMcqPressed('D');
}

TEST_F(BuzzerBehaviorTest, QuestionOpenArmsAgainAfterTimerEnd)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnTimerEnd();
    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, AnswerLockIsSetBeforeTheLocalFeedback)
{
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer('A'))
        .WillOnce([this](char)
                  { buzzerBehavior.OnMcqPressed('B'); });
    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendAnswer('B')).Times(0);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnMcqPressed('A');
}

TEST_F(BuzzerBehaviorTest, ArmedBuzzerAcknowledgesBeforeSendingAnswerB)
{
    testing::InSequence seq;

    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer('B'));
    EXPECT_CALL(mockHubMessageSender, SendAnswer('B'));

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnMcqPressed('B');
}

TEST_F(BuzzerBehaviorTest, ArmedBuzzerAcknowledgesBeforeSendingAnswerD)
{
    testing::InSequence seq;

    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer('D'));
    EXPECT_CALL(mockHubMessageSender, SendAnswer('D'));

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnMcqPressed('D');
}

TEST_F(BuzzerBehaviorTest, SecondPressOnAnotherLetterIsNeitherSentNorAcknowledged)
{
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer('A')).Times(1);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer('B')).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendAnswer('B')).Times(0);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnMcqPressed('A');
    buzzerBehavior.OnMcqPressed('B');
}

TEST_F(BuzzerBehaviorTest, TimerEndWhileInertChangesNothing)
{
    const ArmedModeReader &reader = buzzerBehavior;
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);

    buzzerBehavior.OnTimerEnd();

    EXPECT_FALSE(reader.IsArmed());
    buzzerBehavior.OnMcqPressed('A');
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, FreshInertBuzzerIgnoresEveryPress)
{
    const ArmedModeReader &reader = buzzerBehavior;
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);

    for (char letter : {'A', 'B', 'C', 'D'})
        buzzerBehavior.OnMcqPressed(letter);
    buzzerBehavior.OnBuzzPressed();

    EXPECT_FALSE(reader.IsArmed());
}

TEST_F(BuzzerBehaviorTest, InertBuzzerAfterAnAnswerIgnoresEveryPress)
{
    const ArmedModeReader &reader = buzzerBehavior;
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer('A')).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnMcqPressed('A');
    for (char letter : {'B', 'C', 'D'})
        buzzerBehavior.OnMcqPressed(letter);
    buzzerBehavior.OnBuzzPressed();

    EXPECT_FALSE(reader.IsArmed());
}

TEST_F(BuzzerBehaviorTest, InertBuzzerAfterABuzzIgnoresEveryPress)
{
    const ArmedModeReader &reader = buzzerBehavior;
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer(::testing::_)).Times(0);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzPressed();
    buzzerBehavior.OnBuzzPressed();
    for (char letter : {'A', 'B', 'C', 'D'})
        buzzerBehavior.OnMcqPressed(letter);

    EXPECT_FALSE(reader.IsArmed());
}

TEST_F(BuzzerBehaviorTest, InertBuzzerAfterTimerEndIgnoresEveryPress)
{
    const ArmedModeReader &reader = buzzerBehavior;
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnTimerEnd();
    for (char letter : {'A', 'B', 'C', 'D'})
        buzzerBehavior.OnMcqPressed(letter);
    buzzerBehavior.OnBuzzPressed();

    EXPECT_FALSE(reader.IsArmed());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST_F(BuzzerBehaviorTest, BuzzLockIsSetBeforeTheLocalFeedback)
{
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz())
        .WillOnce([this]()
                  { buzzerBehavior.OnBuzzPressed(); });
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, SecondBuzzPressIsNeitherSentNorAcknowledged)
{
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzPressed();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, SpeedArmedBuzzerStaysSpeedArmedOnMcqPress)
{
    const ArmedModeReader &reader = buzzerBehavior;
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnMcqPressed('A');

    EXPECT_TRUE(reader.IsArmed());
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, BuzzLockedDisarmsSpeedArmedBuzzer)
{
    const ArmedModeReader &reader = buzzerBehavior;
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzLocked();
    buzzerBehavior.OnBuzzPressed();

    EXPECT_FALSE(reader.IsArmed());
}

TEST_F(BuzzerBehaviorTest, BuzzUnlockedArmsAgainAfterBuzzLocked)
{
    const ArmedModeReader &reader = buzzerBehavior;
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzLocked();
    buzzerBehavior.OnBuzzUnlocked();

    EXPECT_TRUE(reader.IsArmed());
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, QuestionOpenArmsAgainAfterBuzzLocked)
{
    const ArmedModeReader &reader = buzzerBehavior;
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    buzzerBehavior.OnQuestionOpen();
    buzzerBehavior.OnBuzzLocked();
    buzzerBehavior.OnQuestionOpen();

    EXPECT_TRUE(reader.IsArmed());
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, InertBuzzerStaysInertOnBuzzLocked)
{
    const ArmedModeReader &reader = buzzerBehavior;
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer(::testing::_)).Times(0);

    buzzerBehavior.OnBuzzLocked();
    buzzerBehavior.OnBuzzPressed();
    for (char letter : {'A', 'B', 'C', 'D'})
        buzzerBehavior.OnMcqPressed(letter);

    EXPECT_FALSE(reader.IsArmed());
}

TEST_F(BuzzerBehaviorTest, InertBuzzerArmsOnBuzzUnlockedAfterBuzzLocked)
{
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    buzzerBehavior.OnBuzzLocked();
    buzzerBehavior.OnBuzzUnlocked();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(BuzzerBehaviorTest, McqArmedBuzzerStaysMcqArmedOnBuzzLocked)
{
    const ArmedModeReader &reader = buzzerBehavior;
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer('C')).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendAnswer('C')).Times(1);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnBuzzLocked();

    EXPECT_TRUE(reader.IsArmed());
    buzzerBehavior.OnMcqPressed('C');
}
