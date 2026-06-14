#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "HubMessageSender.h"
#include "LocalFeedback.h"
#include "BuzzerBehavior.h"
#include "BuzzerButtonPressTranslator.h"
#include "GpioPinReader.h"
#include "PhysicalButton.h"
#include "PhysicalButtonReader.h"
#include "ButtonPressDetector.h"

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

class MockGpioPinReader : public GpioPinReader
{
public:
    MOCK_METHOD(bool, Read, (), (override));
};

class MockButtonPressTranslator : public ButtonPressTranslator
{
public:
    MOCK_METHOD(void, TranslateButtonPress, (PhysicalButton), (override));
};

TEST(ButtonPressDetectorTest, PressedButtonTriggersAnswer)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    BuzzerButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    MockGpioPinReader mockGpioPinAReader;
    std::vector<ButtonPin> buttonPins{{mockGpioPinAReader, PhysicalButton::A}};
    PhysicalButtonReader physicalButtonReader{buttonPins};
    ButtonPressDetector buttonPressDetector{physicalButtonReader, buttonPressTranslator};

    buzzerBehavior.OnQuestionChoices();

    EXPECT_CALL(mockGpioPinAReader, Read()).WillOnce(testing::Return(true));
    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);

    buttonPressDetector.Poll();
}

TEST(ButtonPressDetectorTest, PressedButtonTriggersAnswerOnce)
{
    MockButtonPressTranslator mockButtonPressTranslator;
    MockGpioPinReader mockGpioPinAReader;
    std::vector<ButtonPin> buttonPins{{mockGpioPinAReader, PhysicalButton::A}};
    PhysicalButtonReader physicalButtonReader{buttonPins};
    ButtonPressDetector buttonPressDetector{physicalButtonReader, mockButtonPressTranslator};

    EXPECT_CALL(mockGpioPinAReader, Read()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(PhysicalButton::A)).Times(1);

    buttonPressDetector.Poll();
    buttonPressDetector.Poll();
}

TEST(ButtonPressDetectorTest, PressedThenReleasedDoesNotTriggerOnRelease)
{
    MockButtonPressTranslator mockButtonPressTranslator;
    MockGpioPinReader mockGpioPinAReader;
    std::vector<ButtonPin> buttonPins{{mockGpioPinAReader, PhysicalButton::A}};
    PhysicalButtonReader physicalButtonReader{buttonPins};
    ButtonPressDetector buttonPressDetector{physicalButtonReader, mockButtonPressTranslator};
    
    EXPECT_CALL(mockGpioPinAReader, Read())
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(PhysicalButton::A)).Times(1);
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(PhysicalButton::Unknown)).Times(0);

    buttonPressDetector.Poll();
    buttonPressDetector.Poll();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}