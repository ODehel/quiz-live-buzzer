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

TEST_F(TappableZonesScreenTest, AnOutOfGameScreenDeactivatesTheZonesAndForwardsItsTopBar)
{
    ShowTheFourZones();
    const ScreenModel outOfGame = OutOfGameScreen{"Hors jeu", "cause", "sortie"};

    EXPECT_CALL(inner, Display(TopBar::ForPlayer("Camille"), outOfGame));
    screen.Display(TopBar::ForPlayer("Camille"), outOfGame);

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


TEST_F(TappableZonesScreenTest, AConnectionLostScreenCarryingTheQuestionDeactivatesTheZonesAndForwardsItself)
{
    ShowTheFourZones();
    ConnectionLostScreen lost{"Connexion perdue", "Tentative 1/3", 1, 3};
    lost.dimmed.zoneTexts = {"A : un", "B : deux", "C : trois", "D : quatre"};
    const ScreenModel connectionLost = lost;
    const TopBar cutTopBar = TopBar::ForPlayer("Camille").WithLinkCut();
    EXPECT_CALL(inner, Display(cutTopBar, connectionLost)).Times(1);

    screen.Display(cutTopBar, connectionLost);

    EXPECT_EQ(TouchedAt(100, 100), ButtonInput::Unknown);
}

// Sous le bandeau du haut (60 px), les zones se découpent dans la dalle moins 60 px, comme leur dessin (US-FW-009 CA-7) : 800 × 420 px, soit 400 × 210 px par zone.
// `TouchZoneReader` compte les bords droit et bas dans la zone : les essais aux limites prennent un point de part et d'autre, pas le bord.
TEST_F(TappableZonesScreenTest, NoPointOfAVisibleTopBarIsTouchable)
{
    screen.Display(TopBar::ForPlayer("Camille"), ZonesScreen{{"A : un", "B : deux", "C : trois", "D : quatre"}});

    EXPECT_EQ(TouchedAt(100, 0), ButtonInput::Unknown);
    EXPECT_EQ(TouchedAt(100, 59), ButtonInput::Unknown);
    EXPECT_EQ(TouchedAt(600, 59), ButtonInput::Unknown);
}

TEST_F(TappableZonesScreenTest, TheZonesStartRightUnderAVisibleTopBar)
{
    screen.Display(TopBar::ForPlayer("Camille"), ZonesScreen{{"A : un", "B : deux", "C : trois", "D : quatre"}});

    EXPECT_EQ(TouchedAt(0, 60), ButtonInput::A);
    EXPECT_EQ(TouchedAt(799, 60), ButtonInput::B);
}

TEST_F(TappableZonesScreenTest, TheZonesShareTheHeightUnderAVisibleTopBarInTwoRowsOf210Pixels)
{
    screen.Display(TopBar::ForPlayer("Camille"), ZonesScreen{{"A : un", "B : deux", "C : trois", "D : quatre"}});

    EXPECT_EQ(TouchedAt(100, 269), ButtonInput::A);
    EXPECT_EQ(TouchedAt(600, 269), ButtonInput::B);
    EXPECT_EQ(TouchedAt(100, 271), ButtonInput::C);
    EXPECT_EQ(TouchedAt(600, 271), ButtonInput::D);
    EXPECT_EQ(TouchedAt(100, 479), ButtonInput::C);
}

TEST_F(TappableZonesScreenTest, TheZonesShareTheWidthUnderAVisibleTopBarInTwoColumnsOf400Pixels)
{
    screen.Display(TopBar::ForPlayer("Camille"), ZonesScreen{{"A : un", "B : deux", "C : trois", "D : quatre"}});

    EXPECT_EQ(TouchedAt(399, 100), ButtonInput::A);
    EXPECT_EQ(TouchedAt(401, 100), ButtonInput::B);
}

TEST_F(TappableZonesScreenTest, ACutLinkTopBarCoversTheSameHeightAsALinkedOne)
{
    screen.Display(TopBar::ForPlayer("Camille").WithLinkCut(), ZonesScreen{{"A : un", "B : deux", "C : trois", "D : quatre"}});

    EXPECT_EQ(TouchedAt(100, 59), ButtonInput::Unknown);
    EXPECT_EQ(TouchedAt(100, 60), ButtonInput::A);
}

TEST_F(TappableZonesScreenTest, ShowingZonesWithoutATopBarAfterOneWithATopBarTakesBackTheFullPanel)
{
    screen.Display(TopBar::ForPlayer("Camille"), ZonesScreen{{"A : un", "B : deux", "C : trois", "D : quatre"}});

    ShowTheFourZones();

    EXPECT_EQ(TouchedAt(100, 10), ButtonInput::A);
    EXPECT_EQ(TouchedAt(100, 250), ButtonInput::C);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
