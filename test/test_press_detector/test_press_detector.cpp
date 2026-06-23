#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "../mocks/GameMocks.h"
#include "../mocks/MockGpioPinReader.h"
#include "BuzzerBehavior.h"
#include "BuzzerButtonPressTranslator.h"
#include "ButtonInput.h"
#include "PhysicalButtonReader.h"
#include "ButtonPressDetector.h"

class MockButtonPressTranslator : public ButtonPressTranslator
{
public:
    MOCK_METHOD(void, TranslateButtonPress, (ButtonInput), (override));
};

TEST(ButtonPressDetectorTest, PressedButtonTriggersAnswer)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    BuzzerButtonPressTranslator buttonPressTranslator{buzzerBehavior};
    MockGpioPinReader mockGpioPinAReader;
    std::vector<ButtonPin> buttonPins{{mockGpioPinAReader, ButtonInput::A}};
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
    std::vector<ButtonPin> buttonPins{{mockGpioPinAReader, ButtonInput::A}};
    PhysicalButtonReader physicalButtonReader{buttonPins};
    ButtonPressDetector buttonPressDetector{physicalButtonReader, mockButtonPressTranslator};

    EXPECT_CALL(mockGpioPinAReader, Read()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(ButtonInput::A)).Times(1);

    buttonPressDetector.Poll();
    buttonPressDetector.Poll();
}

TEST(ButtonPressDetectorTest, PressedThenReleasedDoesNotTriggerOnRelease)
{
    MockButtonPressTranslator mockButtonPressTranslator;
    MockGpioPinReader mockGpioPinAReader;
    std::vector<ButtonPin> buttonPins{{mockGpioPinAReader, ButtonInput::A}};
    PhysicalButtonReader physicalButtonReader{buttonPins};
    ButtonPressDetector buttonPressDetector{physicalButtonReader, mockButtonPressTranslator};

    EXPECT_CALL(mockGpioPinAReader, Read())
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(ButtonInput::A)).Times(1);
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(ButtonInput::Unknown)).Times(0);

    buttonPressDetector.Poll();
    buttonPressDetector.Poll();
}

TEST(ButtonPressDetectorTest, ReleasedThenPressedAgainTriggersAnswerTwice)
{
    MockButtonPressTranslator mockButtonPressTranslator;
    MockGpioPinReader mockGpioPinAReader;
    std::vector<ButtonPin> buttonPins{{mockGpioPinAReader, ButtonInput::A}};
    PhysicalButtonReader physicalButtonReader{buttonPins};
    ButtonPressDetector buttonPressDetector{physicalButtonReader, mockButtonPressTranslator};

    EXPECT_CALL(mockGpioPinAReader, Read())
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(ButtonInput::A)).Times(2);
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(ButtonInput::Unknown)).Times(0);

    buttonPressDetector.Poll();
    buttonPressDetector.Poll();
    buttonPressDetector.Poll();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}