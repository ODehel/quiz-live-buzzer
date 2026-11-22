#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include "../mocks/MockGpioPinReader.h"
#include "ButtonInput.h"
#include "PhysicalButtonReader.h"
#include "ButtonPressDetector.h"
#include "TouchPoint.h"
#include "TouchPointReader.h"
#include "TouchZoneReader.h"
#include "TouchButtonReader.h"

class MockButtonPressTranslator : public ButtonPressTranslator
{
public:
    MOCK_METHOD(void, TranslateButtonPress, (ButtonInput), (override));
};

class MockTouchPointReader : public TouchPointReader
{
public:
    MOCK_METHOD(TouchPoint, Read, (), (override));
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

TEST(ButtonPressDetectorTest, HeldTouchTriggersAnswerOnce)
{
    MockButtonPressTranslator mockButtonPressTranslator;
    MockTouchPointReader mockTouchPointReader;
    std::vector<TouchZone> zones{{0, 0, 100, 100, ButtonInput::A}, {101, 0, 200, 100, ButtonInput::B}};
    TouchZoneReader touchZoneReader{zones};
    TouchButtonReader touchButtonReader{mockTouchPointReader, touchZoneReader};
    ButtonPressDetector buttonPressDetector{touchButtonReader, mockButtonPressTranslator};

    EXPECT_CALL(mockTouchPointReader, Read()).WillRepeatedly(testing::Return(TouchPoint{50, 50}));
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(ButtonInput::A)).Times(1);

    buttonPressDetector.Poll();
    buttonPressDetector.Poll();
    buttonPressDetector.Poll();
}

TEST(ButtonPressDetectorTest, FingerSlidingFromZoneAToZoneBTriggersTwoPresses)
{
    MockButtonPressTranslator mockButtonPressTranslator;
    MockTouchPointReader mockTouchPointReader;
    std::vector<TouchZone> zones{{0, 0, 100, 100, ButtonInput::A}, {101, 0, 200, 100, ButtonInput::B}};
    TouchZoneReader touchZoneReader{zones};
    TouchButtonReader touchButtonReader{mockTouchPointReader, touchZoneReader};
    ButtonPressDetector buttonPressDetector{touchButtonReader, mockButtonPressTranslator};

    EXPECT_CALL(mockTouchPointReader, Read())
        .WillOnce(testing::Return(TouchPoint{50, 50}))
        .WillOnce(testing::Return(TouchPoint{150, 50}));
    {
        testing::InSequence inOrder;
        EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(ButtonInput::A)).Times(1);
        EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(ButtonInput::B)).Times(1);
    }

    buttonPressDetector.Poll();
    buttonPressDetector.Poll();
}

TEST(ButtonPressDetectorTest, FingerLeavingZoneAForNoZoneAndComingBackTriggersTwoPresses)
{
    MockButtonPressTranslator mockButtonPressTranslator;
    MockTouchPointReader mockTouchPointReader;
    std::vector<TouchZone> zones{{0, 0, 100, 100, ButtonInput::A}, {101, 0, 200, 100, ButtonInput::B}};
    TouchZoneReader touchZoneReader{zones};
    TouchButtonReader touchButtonReader{mockTouchPointReader, touchZoneReader};
    ButtonPressDetector buttonPressDetector{touchButtonReader, mockButtonPressTranslator};

    EXPECT_CALL(mockTouchPointReader, Read())
        .WillOnce(testing::Return(TouchPoint{50, 50}))
        .WillOnce(testing::Return(TouchPoint{300, 300}))
        .WillOnce(testing::Return(TouchPoint{50, 50}));
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(ButtonInput::A)).Times(2);
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(ButtonInput::B)).Times(0);
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