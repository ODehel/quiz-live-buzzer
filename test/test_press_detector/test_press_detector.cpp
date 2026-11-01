#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../mocks/MockGpioPinReader.h"
#include "ButtonInput.h"
#include "PhysicalButtonReader.h"
#include "ButtonPressDetector.h"

class MockButtonPressTranslator : public ButtonPressTranslator
{
public:
    MOCK_METHOD(void, TranslateButtonPress, (ButtonInput), (override));
};

TEST(ButtonPressDetectorTest, PressedButtonTriggersAnswerOnce)
{
    MockButtonPressTranslator mockButtonPressTranslator;
    MockGpioPinReader mockGpioPinBuzzReader;
    PhysicalButtonReader physicalButtonReader{mockGpioPinBuzzReader};
    ButtonPressDetector buttonPressDetector{physicalButtonReader, mockButtonPressTranslator};

    EXPECT_CALL(mockGpioPinBuzzReader, Read()).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(ButtonInput::Buzz)).Times(1);

    buttonPressDetector.Poll();
    buttonPressDetector.Poll();
}

TEST(ButtonPressDetectorTest, PressedThenReleasedDoesNotTriggerOnRelease)
{
    MockButtonPressTranslator mockButtonPressTranslator;
    MockGpioPinReader mockGpioPinBuzzReader;
    PhysicalButtonReader physicalButtonReader{mockGpioPinBuzzReader};
    ButtonPressDetector buttonPressDetector{physicalButtonReader, mockButtonPressTranslator};

    EXPECT_CALL(mockGpioPinBuzzReader, Read())
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(ButtonInput::Buzz)).Times(1);
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(ButtonInput::Unknown)).Times(0);

    buttonPressDetector.Poll();
    buttonPressDetector.Poll();
}

TEST(ButtonPressDetectorTest, ReleasedThenPressedAgainTriggersAnswerTwice)
{
    MockButtonPressTranslator mockButtonPressTranslator;
    MockGpioPinReader mockGpioPinBuzzReader;
    PhysicalButtonReader physicalButtonReader{mockGpioPinBuzzReader};
    ButtonPressDetector buttonPressDetector{physicalButtonReader, mockButtonPressTranslator};

    EXPECT_CALL(mockGpioPinBuzzReader, Read())
        .WillOnce(testing::Return(true))
        .WillOnce(testing::Return(false))
        .WillOnce(testing::Return(true));
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(ButtonInput::Buzz)).Times(2);
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