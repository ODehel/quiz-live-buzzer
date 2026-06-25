#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "../mocks/GameMocks.h"
#include "BuzzerBehavior.h"
#include "BuzzerButtonPressTranslator.h"
#include "ButtonInput.h"
#include "TouchPoint.h"
#include "TouchZoneReader.h"
#include "TouchPointReader.h"
#include "TouchButtonReader.h"

class MockTouchPointReader : public TouchPointReader
{
public:
    MOCK_METHOD(TouchPoint, Read, (), (override));
};

TEST(ButtonPressTranslatorTest, MCQButtonPressSendsMatchingAnswerA)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    BuzzerButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    buzzerBehavior.OnQuestionChoices();

    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);

    buttonPressTranslator.TranslateButtonPress(ButtonInput::A);
}

TEST(ButtonPressTranslatorTest, MCQButtonPressSendsMatchingAnswerB)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    BuzzerButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    buzzerBehavior.OnQuestionChoices();

    EXPECT_CALL(mockHubMessageSender, SendAnswer('B')).Times(1);

    buttonPressTranslator.TranslateButtonPress(ButtonInput::B);
}

TEST(ButtonPressTranslatorTest, MCQButtonPressSendsMatchingAnswerC)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    BuzzerButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    buzzerBehavior.OnQuestionChoices();

    EXPECT_CALL(mockHubMessageSender, SendAnswer('C')).Times(1);

    buttonPressTranslator.TranslateButtonPress(ButtonInput::C);
}

TEST(ButtonPressTranslatorTest, MCQButtonPressSendsMatchingAnswerD)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    BuzzerButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    buzzerBehavior.OnQuestionChoices();

    EXPECT_CALL(mockHubMessageSender, SendAnswer('D')).Times(1);

    buttonPressTranslator.TranslateButtonPress(ButtonInput::D);
}

TEST(ButtonPressTranslatorTest, BuzzButtonPressSendsBuzz)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    BuzzerButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    buzzerBehavior.OnQuestionOpen();

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    buttonPressTranslator.TranslateButtonPress(ButtonInput::Buzz);
}

TEST(ButtonPressTranslatorTest, UnknownButtonPressSendsNothing)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    BuzzerButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    buzzerBehavior.OnQuestionChoices();

    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);

    buttonPressTranslator.TranslateButtonPress(ButtonInput::Unknown);
}

TEST(ButtonPressTranslatorTest, TouchPointInsideZoneDeducesItsButton)
{
    MockTouchPointReader mockTouchPointReader;
    std::vector<TouchZone> zones{{0, 0, 100, 100, ButtonInput::A}};
    TouchZoneReader touchZoneReader{zones};
    TouchButtonReader touchButtonReader{mockTouchPointReader, touchZoneReader};

    ON_CALL(mockTouchPointReader, Read()).WillByDefault(::testing::Return(TouchPoint{50, 50}));
    
    ButtonInput buttonInput = touchButtonReader.DeduceButton();

    EXPECT_EQ(buttonInput, ButtonInput::A);
}

TEST(ButtonPressTranslatorTest, TouchPointOutsideZoneDeducesUnknownButton)
{
    MockTouchPointReader mockTouchPointReader;
    std::vector<TouchZone> zones{{0, 0, 100, 100, ButtonInput::A}};
    TouchZoneReader touchZoneReader{zones};
    TouchButtonReader touchButtonReader{mockTouchPointReader, touchZoneReader};

    ON_CALL(mockTouchPointReader, Read()).WillByDefault(::testing::Return(TouchPoint{250, 250}));
    
    ButtonInput buttonInput = touchButtonReader.DeduceButton();

    EXPECT_EQ(buttonInput, ButtonInput::Unknown);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}