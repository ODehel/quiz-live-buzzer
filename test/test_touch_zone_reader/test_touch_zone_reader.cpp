#include <gtest/gtest.h>
#include <vector>
#include "TouchZoneReader.h"
#include "ButtonInput.h"

TEST(TouchZoneReaderTest, PointInsideZoneDeducesItsButton)
{
    std::vector<TouchZone> zones{{0, 0, 100, 100, ButtonInput::A}};
    TouchZoneReader reader{zones};

    EXPECT_EQ(reader.DeduceButton({50, 50}), ButtonInput::A);
}

TEST(TouchZoneReaderTest, PointOutsideZoneDeducesUnknown)
{
    std::vector<TouchZone> zones{{0, 0, 100, 100, ButtonInput::A}};
    TouchZoneReader reader{zones};

    EXPECT_EQ(reader.DeduceButton({200, 200}), ButtonInput::Unknown);
}

TEST(TouchZoneReaderTest, PointRightOfZoneDeducesUnknown)
{
    std::vector<TouchZone> zones{{0, 0, 100, 100, ButtonInput::A}};
    TouchZoneReader reader{zones};

    EXPECT_EQ(reader.DeduceButton({150, 50}), ButtonInput::Unknown);
}

TEST(TouchZoneReaderTest, PointLeftOfZoneDeducesUnknown)
{
    std::vector<TouchZone> zones{{0, 0, 100, 100, ButtonInput::A}};
    TouchZoneReader reader{zones};

    EXPECT_EQ(reader.DeduceButton({-2, 50}), ButtonInput::Unknown);
}

TEST(TouchZoneReaderTest, PointBelowOfZoneDeducesUnknown)
{
    std::vector<TouchZone> zones{{0, 0, 100, 100, ButtonInput::A}};
    TouchZoneReader reader{zones};

    EXPECT_EQ(reader.DeduceButton({50, 250}), ButtonInput::Unknown);
}

TEST(TouchZoneReaderTest, PointAboveOfZoneDeducesUnknown)
{
    std::vector<TouchZone> zones{{0, 0, 100, 100, ButtonInput::A}};
    TouchZoneReader reader{zones};

    EXPECT_EQ(reader.DeduceButton({50, -2}), ButtonInput::Unknown);
}

TEST(TouchZoneReaderTest, NoContactPointDeducesUnknown)
{
    std::vector<TouchZone> zones{{0, 0, 100, 100, ButtonInput::A}};
    TouchZoneReader reader{zones};

    EXPECT_EQ(reader.DeduceButton(TouchPoint::NoContact()), ButtonInput::Unknown);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}