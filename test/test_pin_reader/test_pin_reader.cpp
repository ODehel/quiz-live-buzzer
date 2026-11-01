#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "ButtonInput.h"
#include "PhysicalButtonReader.h"
#include "../mocks/MockGpioPinReader.h"


TEST(PhysicalButtonReaderTest, HighDigitalPinDeducesBuzzButton)
{
    MockGpioPinReader mockGpioPinReader;
    PhysicalButtonReader physicalButtonReader{mockGpioPinReader};

    EXPECT_CALL(mockGpioPinReader, Read()).WillOnce(testing::Return(true));

    EXPECT_EQ(physicalButtonReader.DeduceButton(), ButtonInput::Buzz);
}

TEST(PhysicalButtonReaderTest, LowDigitalPinDeducesUnknownButton)
{
    MockGpioPinReader mockGpioPinReader;
    PhysicalButtonReader physicalButtonReader{mockGpioPinReader};

    EXPECT_CALL(mockGpioPinReader, Read()).WillOnce(testing::Return(false));

    EXPECT_EQ(physicalButtonReader.DeduceButton(), ButtonInput::Unknown);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}