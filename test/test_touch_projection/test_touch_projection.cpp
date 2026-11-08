#include <gtest/gtest.h>
#include "TouchProjection.h"
#include "TouchPoint.h"

TEST(TouchProjectionTest, SensorOriginMapsToScreenOrigin)
{
    TouchProjection touchProjection(TouchPoint{0, 0}, TouchPoint{800, 480});

    EXPECT_EQ(touchProjection.Project(TouchPoint{0, 0}), (TouchPoint{0, 0}));
}

TEST(TouchProjectionTest, SensorFarCornerMapsToScreenFarCorner)
{
    TouchProjection touchProjection(TouchPoint{0, 0}, TouchPoint{800, 480});

    EXPECT_EQ(touchProjection.Project(TouchPoint{800, 480}), (TouchPoint{799, 479}));
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}