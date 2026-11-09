#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "../mocks/ScreenMocks.h"
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

    void ShowTheFourZones()
    {
        screen.Display(TopBar::Hidden(), ZonesScreen{{"A : un", "B : deux", "C : trois", "D : quatre"}});
    }
};

TEST_F(TappableZonesScreenTest, DisplayForwardsATextScreenToTheScreen)
{
    EXPECT_CALL(inner, Display(TopBar::Hidden(), ScreenModel{TextScreen{"Connecté"}}));

    screen.Display(TopBar::Hidden(), TextScreen{"Connecté"});
}

TEST_F(TappableZonesScreenTest, DisplayForwardsTheTopBarToTheScreen)
{
    EXPECT_CALL(inner, Display(TopBar::ForPlayer("Camille"), ScreenModel{TextScreen{"Connecté"}}));

    screen.Display(TopBar::ForPlayer("Camille"), TextScreen{"Connecté"});
}

TEST_F(TappableZonesScreenTest, DisplayForwardsTheTopBarWithAZonesScreenToTheScreen)
{
    const ScreenModel zones = ZonesScreen{{"A : un", "B : deux"}};
    EXPECT_CALL(inner, Display(TopBar::ForPlayer("Camille"), zones));

    screen.Display(TopBar::ForPlayer("Camille"), zones);
}

TEST_F(TappableZonesScreenTest, DisplayForwardsAZonesScreenToTheScreen)
{
    const ScreenModel zones = ZonesScreen{{"A : un", "B : deux"}};
    EXPECT_CALL(inner, Display(TopBar::Hidden(), zones));

    screen.Display(TopBar::Hidden(), zones);
}

TEST_F(TappableZonesScreenTest, DisplayForwardsAHighlightedZoneScreenToTheScreen)
{
    EXPECT_CALL(inner, Display(TopBar::Hidden(), ScreenModel{HighlightedZoneScreen{2, "Bonne réponse : C"}}));

    screen.Display(TopBar::Hidden(), HighlightedZoneScreen{2, "Bonne réponse : C"});
}

TEST_F(TappableZonesScreenTest, DisplayForwardsAChosenZoneScreenToTheScreen)
{
    EXPECT_CALL(inner, Display(TopBar::Hidden(), ScreenModel{ChosenZoneScreen{1}}));

    screen.Display(TopBar::Hidden(), ChosenZoneScreen{1});
}

TEST_F(TappableZonesScreenTest, DisplayForwardsABannerScreenToTheScreen)
{
    EXPECT_CALL(inner, Display(TopBar::Hidden(), ScreenModel{BannerScreen{"Réponse enregistrée"}}));

    screen.Display(TopBar::Hidden(), BannerScreen{"Réponse enregistrée"});
}

TEST_F(TappableZonesScreenTest, NoZoneIsTouchableBeforeAnyZoneIsShown)
{
    EXPECT_EQ(TouchedAt(100, 100), ButtonInput::Unknown);
}

TEST_F(TappableZonesScreenTest, EachQuarterIsTouchableOnceTheFourZonesAreShown)
{
    ShowTheFourZones();

    EXPECT_EQ(TouchedAt(100, 100), ButtonInput::A);
    EXPECT_EQ(TouchedAt(600, 100), ButtonInput::B);
    EXPECT_EQ(TouchedAt(100, 400), ButtonInput::C);
    EXPECT_EQ(TouchedAt(600, 400), ButtonInput::D);
}

TEST_F(TappableZonesScreenTest, OnlyTheShownZonesAreTouchable)
{
    screen.Display(TopBar::Hidden(), ZonesScreen{{"A : un", "B : deux"}});

    EXPECT_EQ(TouchedAt(600, 100), ButtonInput::B);
    EXPECT_EQ(TouchedAt(100, 400), ButtonInput::Unknown);
}

TEST_F(TappableZonesScreenTest, ShowingZonesAgainReplacesThePreviousOnes)
{
    ShowTheFourZones();

    screen.Display(TopBar::Hidden(), ZonesScreen{{"A : un", "B : deux"}});

    EXPECT_EQ(TouchedAt(100, 100), ButtonInput::A);
    EXPECT_EQ(TouchedAt(100, 400), ButtonInput::Unknown);
}

TEST_F(TappableZonesScreenTest, NoMoreThanFourZonesAreTouchable)
{
    screen.Display(TopBar::Hidden(), ZonesScreen{{"A : un", "B : deux", "C : trois", "D : quatre", "E : cinq"}});

    EXPECT_EQ(TouchedAt(100, 500), ButtonInput::Unknown);
}

TEST_F(TappableZonesScreenTest, ATextScreenDeactivatesTheZones)
{
    ShowTheFourZones();

    screen.Display(TopBar::Hidden(), TextScreen{"12 s"});

    EXPECT_EQ(TouchedAt(100, 100), ButtonInput::Unknown);
}

TEST_F(TappableZonesScreenTest, AChosenZoneScreenDeactivatesTheZones)
{
    ShowTheFourZones();

    screen.Display(TopBar::Hidden(), ChosenZoneScreen{1});

    EXPECT_EQ(TouchedAt(600, 100), ButtonInput::Unknown);
}

TEST_F(TappableZonesScreenTest, AHighlightedZoneScreenDeactivatesTheZones)
{
    ShowTheFourZones();

    screen.Display(TopBar::Hidden(), HighlightedZoneScreen{2, "Bonne réponse : C"});

    EXPECT_EQ(TouchedAt(100, 400), ButtonInput::Unknown);
}

TEST_F(TappableZonesScreenTest, ABannerScreenDeactivatesTheZones)
{
    ShowTheFourZones();

    screen.Display(TopBar::Hidden(), BannerScreen{"Réponse enregistrée"});

    EXPECT_EQ(TouchedAt(600, 400), ButtonInput::Unknown);
}

TEST_F(TappableZonesScreenTest, AnUnchangedScreenTouchesNeitherTheScreenNorTheZones)
{
    ShowTheFourZones();
    EXPECT_CALL(inner, Display(testing::_, testing::_)).Times(0);

    screen.Display(TopBar::Hidden(), ScreenUnchanged{});

    EXPECT_EQ(TouchedAt(100, 100), ButtonInput::A);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
