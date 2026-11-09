#include <gtest/gtest.h>
#include "ZoneGrid.h"

TEST(ZoneGridTest, ZoneAIsTheTopLeftQuarterOfTheArea)
{
    ZoneGrid grid(800, 0, 480);

    ZoneRect zone = grid.At(0);

    EXPECT_EQ(zone.x, 0);
    EXPECT_EQ(zone.y, 0);
    EXPECT_EQ(zone.width, 400);
    EXPECT_EQ(zone.height, 240);
}

TEST(ZoneGridTest, ZoneBIsTheTopRightQuarterOfTheArea)
{
    ZoneGrid grid(800, 0, 480);

    ZoneRect zone = grid.At(1);

    EXPECT_EQ(zone.x, 400);
    EXPECT_EQ(zone.y, 0);
    EXPECT_EQ(zone.width, 400);
    EXPECT_EQ(zone.height, 240);
}

TEST(ZoneGridTest, ZoneCIsTheBottomLeftQuarterOfTheArea)
{
    ZoneGrid grid(800, 0, 480);

    ZoneRect zone = grid.At(2);

    EXPECT_EQ(zone.x, 0);
    EXPECT_EQ(zone.y, 240);
    EXPECT_EQ(zone.width, 400);
    EXPECT_EQ(zone.height, 240);
}

TEST(ZoneGridTest, ZonesBelowABannerStartAtItsBottomAndShareTheRemainingHeight)
{
    ZoneGrid grid(800, 120, 360);

    ZoneRect zone = grid.At(3);

    EXPECT_EQ(zone.x, 400);
    EXPECT_EQ(zone.y, 300);
    EXPECT_EQ(zone.width, 400);
    EXPECT_EQ(zone.height, 180);
}

TEST(ZoneGridTest, ZoneDIsTheBottomRightQuarterOfTheArea)
{
    ZoneGrid grid(800, 0, 480);

    ZoneRect zone = grid.At(3);

    EXPECT_EQ(zone.x, 400);
    EXPECT_EQ(zone.y, 240);
    EXPECT_EQ(zone.width, 400);
    EXPECT_EQ(zone.height, 240);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}