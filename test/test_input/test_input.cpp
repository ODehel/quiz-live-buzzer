#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../mocks/GameMocks.h"
#include "BuzzerBehavior.h"
#include "BuzzerButtonPressTranslator.h"
#include "PhysicalButton.h"

TEST(ButtonPressTranslatorTest, MCQButtonPressSendsMatchingAnswerA)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    BuzzerButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    buzzerBehavior.OnQuestionChoices();

    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);

    buttonPressTranslator.TranslateButtonPress(PhysicalButton::A);
}

TEST(ButtonPressTranslatorTest, MCQButtonPressSendsMatchingAnswerB)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    BuzzerButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    buzzerBehavior.OnQuestionChoices();

    EXPECT_CALL(mockHubMessageSender, SendAnswer('B')).Times(1);

    buttonPressTranslator.TranslateButtonPress(PhysicalButton::B);
}

TEST(ButtonPressTranslatorTest, MCQButtonPressSendsMatchingAnswerC)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    BuzzerButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    buzzerBehavior.OnQuestionChoices();

    EXPECT_CALL(mockHubMessageSender, SendAnswer('C')).Times(1);

    buttonPressTranslator.TranslateButtonPress(PhysicalButton::C);
}

TEST(ButtonPressTranslatorTest, MCQButtonPressSendsMatchingAnswerD)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    BuzzerButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    buzzerBehavior.OnQuestionChoices();

    EXPECT_CALL(mockHubMessageSender, SendAnswer('D')).Times(1);

    buttonPressTranslator.TranslateButtonPress(PhysicalButton::D);
}

TEST(ButtonPressTranslatorTest, BuzzButtonPressSendsBuzz)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    BuzzerButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    buzzerBehavior.OnQuestionOpen();

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    buttonPressTranslator.TranslateButtonPress(PhysicalButton::Buzz);
}

TEST(ButtonPressTranslatorTest, UnknownButtonPressSendsNothing)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    BuzzerButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    buzzerBehavior.OnQuestionChoices();

    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);

    buttonPressTranslator.TranslateButtonPress(PhysicalButton::Unknown);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}