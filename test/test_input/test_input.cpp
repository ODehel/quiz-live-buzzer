#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "HubMessageSender.h"
#include "BuzzerBehavior.h"
#include "LocalFeedback.h"
#include "ButtonPressTranslator.h"

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

TEST(ButtonPressTranslatorTest, MCQButtonPressSendsMatchingAnswerA)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    ButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    buzzerBehavior.OnQuestionChoices();

    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);

    buttonPressTranslator.TranslateButtonPress(PhysicalButton::A);
}

TEST(ButtonPressTranslatorTest, MCQButtonPressSendsMatchingAnswerC)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    ButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    buzzerBehavior.OnQuestionChoices();

    EXPECT_CALL(mockHubMessageSender, SendAnswer('C')).Times(1);

    buttonPressTranslator.TranslateButtonPress(PhysicalButton::C);
}

TEST(ButtonPressTranslatorTest, BuzzButtonPressSendsBuzz)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    ButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    buzzerBehavior.OnQuestionOpen();

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    buttonPressTranslator.TranslateButtonPress(PhysicalButton::Buzz);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}