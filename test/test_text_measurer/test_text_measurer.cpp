#include <gtest/gtest.h>
#include "MonospaceTextMeasurer.h"

TEST(MonospaceTextMeasurerTest, ShouldOfferThreeSizes)
{
    MonospaceTextMeasurer measurer(6, 8);
    EXPECT_EQ(measurer.SizeCount(), 3);
}

TEST(MonospaceTextMeasurerTest, WidthOfATextAtTheLargestSizeShouldBeItsCharactersTimesTheCellWidthTimesFive)
{
    MonospaceTextMeasurer measurer(6, 8);
    EXPECT_EQ(measurer.Width("abc", 0), 90);
}

TEST(MonospaceTextMeasurerTest, AccentedCharacterShouldTakeOneCell)
{
    MonospaceTextMeasurer measurer(6, 8);
    EXPECT_EQ(measurer.Width("éé", 0), 60);
}

TEST(MonospaceTextMeasurerTest, WidthShouldShrinkWithTheSize)
{
    MonospaceTextMeasurer measurer(6, 8);
    EXPECT_EQ(measurer.Width("abc", 1), 72);
    EXPECT_EQ(measurer.Width("abc", 2), 54);
}

TEST(MonospaceTextMeasurerTest, LineHeightAtTheLargestSizeShouldBeTheCellHeightTimesFive)
{
    MonospaceTextMeasurer measurer(6, 8);
    EXPECT_EQ(measurer.LineHeight(0), 40);
}

TEST(MonospaceTextMeasurerTest, LineHeightShouldShrinkWithTheSize)
{
    MonospaceTextMeasurer measurer(6, 8);
    EXPECT_EQ(measurer.LineHeight(1), 32);
    EXPECT_EQ(measurer.LineHeight(2), 24);
}

TEST(MonospaceTextMeasurerTest, SizesShouldDrawAtScalesFiveFourAndThree)
{
    MonospaceTextMeasurer measurer(6, 8);
    EXPECT_EQ(measurer.ScaleOf(0), 5);
    EXPECT_EQ(measurer.ScaleOf(1), 4);
    EXPECT_EQ(measurer.ScaleOf(2), 3);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
