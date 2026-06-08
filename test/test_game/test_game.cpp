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

    buzzerBehavior.OnButtonPressed('A');
}

TEST_F(BuzzerBehaviorTest, ArmedBuzzerSendsAnswerOnceOnButtonPress)
{
    EXPECT_CALL(mockAnswerSender, SendAnswer('A')).Times(1);

    buzzerBehavior.Arm();
    buzzerBehavior.OnButtonPressed('A');
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}