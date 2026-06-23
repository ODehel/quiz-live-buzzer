#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include "ButtonInput.h"
#include "PhysicalButtonReader.h"
#include "../mocks/MockGpioPinReader.h"


TEST(PhysicalButtonReaderTest, HighDigitalPinDeducesPhysicalButtonA)
{
    MockGpioPinReader mockGpioPinReader;
    std::vector<ButtonPin> buttonPins{{mockGpioPinReader, ButtonInput::A}};
    PhysicalButtonReader physicalButtonReader{buttonPins};

    EXPECT_CALL(mockGpioPinReader, Read()).WillOnce(testing::Return(true));

    EXPECT_EQ(physicalButtonReader.DeduceButton(), ButtonInput::A);
}

TEST(PhysicalButtonReaderTest, LowDigitalPinDeducesPhysicalButtonUnknown)
{
    MockGpioPinReader mockGpioPinReader;
    std::vector<ButtonPin> buttonPins{{mockGpioPinReader, ButtonInput::B}};
    PhysicalButtonReader physicalButtonReader{buttonPins};

    EXPECT_CALL(mockGpioPinReader, Read()).WillOnce(testing::Return(false));

    EXPECT_EQ(physicalButtonReader.DeduceButton(), ButtonInput::Unknown);
}

TEST(PhysicalButtonReaderTest, HighDigitalPinDeducesPhysicalButtonB)
{
    MockGpioPinReader mockGpioPinAReader;
    MockGpioPinReader mockGpioPinBReader;
    std::vector<ButtonPin> buttonPins{{mockGpioPinAReader, ButtonInput::A}, {mockGpioPinBReader, ButtonInput::B}};
    PhysicalButtonReader physicalButtonReader{buttonPins};

    EXPECT_CALL(mockGpioPinAReader, Read()).WillOnce(testing::Return(false));
    EXPECT_CALL(mockGpioPinBReader, Read()).WillOnce(testing::Return(true));

    EXPECT_EQ(physicalButtonReader.DeduceButton(), ButtonInput::B);
}

TEST(PhysicalButtonReaderTest, MultiplePressedPinsDeducesPhysicalButtonUnknown)
{
    MockGpioPinReader mockGpioPinAReader;
    MockGpioPinReader mockGpioPinBReader;
    std::vector<ButtonPin> buttonPins{{mockGpioPinAReader, ButtonInput::A}, {mockGpioPinBReader, ButtonInput::B}};
    PhysicalButtonReader physicalButtonReader{buttonPins};

    EXPECT_CALL(mockGpioPinAReader, Read()).WillOnce(testing::Return(true));
    EXPECT_CALL(mockGpioPinBReader, Read()).WillOnce(testing::Return(true));

    EXPECT_EQ(physicalButtonReader.DeduceButton(), ButtonInput::Unknown);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}