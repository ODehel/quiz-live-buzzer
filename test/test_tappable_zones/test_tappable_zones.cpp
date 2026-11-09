#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "ScreenMocks.h"
#include "TappableZonesScreen.h"
#include "TouchZoneReader.h"

using ::testing::NiceMock;

class TappableZonesScreenTest : public ::testing::Test
{
protected:
    NiceMock<MockScreen> inner;
    TappableZonesScreen screen{inner, 800, 480};
    TouchZoneReader reader{screen.Zones()};

    ButtonInput TouchedAt(int x, int y)
    {
        return reader.DeduceButton({x, y});
    }
};

TEST_F(TappableZonesScreenTest, ShowForwardsTheTextToTheScreen)
{
    EXPECT_CALL(inner, Show("Connecté"));

    screen.Show("Connecté");
}

TEST_F(TappableZonesScreenTest, ShowZonesForwardsTheTextsToTheScreen)
{
    const std::vector<std::string> texts{"A : un", "B : deux"};
    EXPECT_CALL(inner, ShowZones(texts));

    screen.ShowZones(texts);
}

TEST_F(TappableZonesScreenTest, HighlightZoneForwardsTheZoneAndTheTextToTheScreen)
{
    EXPECT_CALL(inner, HighlightZone(2, "Bonne réponse : C"));

    screen.HighlightZone(2, "Bonne réponse : C");
}

TEST_F(TappableZonesScreenTest, HighlightChosenZoneForwardsTheZoneToTheScreen)
{
    EXPECT_CALL(inner, HighlightChosenZone(1));

    screen.HighlightChosenZone(1);
}

TEST_F(TappableZonesScreenTest, ShowBannerForwardsTheTextToTheScreen)
{
    EXPECT_CALL(inner, ShowBanner("Réponse enregistrée"));

    screen.ShowBanner("Réponse enregistrée");
}

TEST_F(TappableZonesScreenTest, NoZoneIsTouchableBeforeAnyZoneIsShown)
{
    EXPECT_EQ(TouchedAt(100, 100), ButtonInput::Unknown);
}

TEST_F(TappableZonesScreenTest, EachQuarterIsTouchableOnceTheFourZonesAreShown)
{
    screen.ShowZones({"A : un", "B : deux", "C : trois", "D : quatre"});

    EXPECT_EQ(TouchedAt(100, 100), ButtonInput::A);
    EXPECT_EQ(TouchedAt(600, 100), ButtonInput::B);
    EXPECT_EQ(TouchedAt(100, 400), ButtonInput::C);
    EXPECT_EQ(TouchedAt(600, 400), ButtonInput::D);
}

TEST_F(TappableZonesScreenTest, OnlyTheShownZonesAreTouchable)
{
    screen.ShowZones({"A : un", "B : deux"});

    EXPECT_EQ(TouchedAt(600, 100), ButtonInput::B);
    EXPECT_EQ(TouchedAt(100, 400), ButtonInput::Unknown);
}

TEST_F(TappableZonesScreenTest, ShowingZonesAgainReplacesThePreviousOnes)
{
    screen.ShowZones({"A : un", "B : deux", "C : trois", "D : quatre"});

    screen.ShowZones({"A : un", "B : deux"});

    EXPECT_EQ(TouchedAt(100, 100), ButtonInput::A);
    EXPECT_EQ(TouchedAt(100, 400), ButtonInput::Unknown);
}

TEST_F(TappableZonesScreenTest, NoMoreThanFourZonesAreTouchable)
{
    screen.ShowZones({"A : un", "B : deux", "C : trois", "D : quatre", "E : cinq"});

    EXPECT_EQ(TouchedAt(100, 500), ButtonInput::Unknown);
}

TEST_F(TappableZonesScreenTest, ShowDeactivatesTheZones)
{
    screen.ShowZones({"A : un", "B : deux", "C : trois", "D : quatre"});

    screen.Show("12 s");

    EXPECT_EQ(TouchedAt(100, 100), ButtonInput::Unknown);
}

TEST_F(TappableZonesScreenTest, HighlightChosenZoneDeactivatesTheZones)
{
    screen.ShowZones({"A : un", "B : deux", "C : trois", "D : quatre"});

    screen.HighlightChosenZone(1);

    EXPECT_EQ(TouchedAt(600, 100), ButtonInput::Unknown);
}

TEST_F(TappableZonesScreenTest, HighlightZoneDeactivatesTheZones)
{
    screen.ShowZones({"A : un", "B : deux", "C : trois", "D : quatre"});

    screen.HighlightZone(2, "Bonne réponse : C");

    EXPECT_EQ(TouchedAt(100, 400), ButtonInput::Unknown);
}

TEST_F(TappableZonesScreenTest, ShowBannerDeactivatesTheZones)
{
    screen.ShowZones({"A : un", "B : deux", "C : trois", "D : quatre"});

    screen.ShowBanner("Réponse enregistrée");

    EXPECT_EQ(TouchedAt(600, 400), ButtonInput::Unknown);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}