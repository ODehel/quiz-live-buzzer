#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "AnswerSender.h"
#include "BuzzerBehavior.h"

class MockAnswerSender : public AnswerSender 
{
public:
    MOCK_METHOD(void, SendAnswer, (char), (override));
};

class BuzzerBehaviorTest : public ::testing::Test
{
protected:
    MockAnswerSender mockAnswerSender;
    BuzzerBehavior buzzerBehavior{mockAnswerSender};
};

TEST_F(BuzzerBehaviorTest, InertBuzzerDoesNotSendAnswerOnButtonPress)
{
    EXPECT_CALL(mockAnswerSender, SendAnswer(::testing::_)).Times(0);

    buzzerBehavior.OnButtonPressed(ButtonFamily::Mcq, 'A');
}

TEST_F(BuzzerBehaviorTest, ArmedBuzzerSendsAnswerOnceOnDoubleButtonPress)
{
    EXPECT_CALL(mockAnswerSender, SendAnswer('A')).Times(1);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnButtonPressed(ButtonFamily::Mcq, 'A');
    buzzerBehavior.OnButtonPressed(ButtonFamily::Mcq, 'A');
}

TEST_F(BuzzerBehaviorTest, McqArmedBuzzerIgnoresBuzzPress)
{
    EXPECT_CALL(mockAnswerSender, SendAnswer(::testing::_)).Times(0);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnButtonPressed(ButtonFamily::Buzz, '\0');
}

TEST_F(BuzzerBehaviorTest, TimerEndDisarmsMcqArmedBuzzer)
{
    EXPECT_CALL(mockAnswerSender, SendAnswer(::testing::_)).Times(0);

    buzzerBehavior.OnQuestionChoices();
    buzzerBehavior.OnTimerEnd();
    buzzerBehavior.OnButtonPressed(ButtonFamily::Mcq, 'A');
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}