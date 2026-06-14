#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include "PhysicalButton.h"
#include "PhysicalButtonReader.h"
#include "../mocks/MockGpioPinReader.h"


TEST(PhysicalButtonReaderTest, HighDigitalPinDeducesPhysicalButtonA)
{
    MockGpioPinReader mockGpioPinReader;
    std::vector<ButtonPin> buttonPins{{mockGpioPinReader, PhysicalButton::A}};
    PhysicalButtonReader physicalButtonReader{buttonPins};

    EXPECT_CALL(mockGpioPinReader, Read()).WillOnce(testing::Return(true));

    EXPECT_EQ(physicalButtonReader.DeduceButton(), PhysicalButton::A);
}

TEST(PhysicalButtonReaderTest, LowDigitalPinDeducesPhysicalButtonUnknown)
{
    MockGpioPinReader mockGpioPinReader;
    std::vector<ButtonPin> buttonPins{{mockGpioPinReader, PhysicalButton::B}};
    PhysicalButtonReader physicalButtonReader{buttonPins};

    EXPECT_CALL(mockGpioPinReader, Read()).WillOnce(testing::Return(false));

    EXPECT_EQ(physicalButtonReader.DeduceButton(), PhysicalButton::Unknown);
}

TEST(PhysicalButtonReaderTest, HighDigitalPinDeducesPhysicalButtonB)
{
    MockGpioPinReader mockGpioPinAReader;
    MockGpioPinReader mockGpioPinBReader;
    std::vector<ButtonPin> buttonPins{{mockGpioPinAReader, PhysicalButton::A}, {mockGpioPinBReader, PhysicalButton::B}};
    PhysicalButtonReader physicalButtonReader{buttonPins};

    EXPECT_CALL(mockGpioPinAReader, Read()).WillOnce(testing::Return(false));
    EXPECT_CALL(mockGpioPinBReader, Read()).WillOnce(testing::Return(true));

    EXPECT_EQ(physicalButtonReader.DeduceButton(), PhysicalButton::B);
}

TEST(PhysicalButtonReaderTest, MultiplePressedPinsDeducesPhysicalButtonUnknown)
{
    MockGpioPinReader mockGpioPinAReader;
    MockGpioPinReader mockGpioPinBReader;
    std::vector<ButtonPin> buttonPins{{mockGpioPinAReader, PhysicalButton::A}, {mockGpioPinBReader, PhysicalButton::B}};
    PhysicalButtonReader physicalButtonReader{buttonPins};

    EXPECT_CALL(mockGpioPinAReader, Read()).WillOnce(testing::Return(true));
    EXPECT_CALL(mockGpioPinBReader, Read()).WillOnce(testing::Return(true));

    EXPECT_EQ(physicalButtonReader.DeduceButton(), PhysicalButton::Unknown);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}