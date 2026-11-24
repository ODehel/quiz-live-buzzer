#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include <vector>
#include "../mocks/ScreenMocks.h"
#include "RetainingScreen.h"

using ::testing::_;
using ::testing::NiceMock;

namespace
{
const std::vector<std::string> FourZones{"A : un", "B : deux", "C : trois", "D : quatre"};
const TopBar CutTopBar = TopBar::ForPlayer("Camille").WithLinkCut();

ConnectionLostScreen LostScreen(int attempt)
{
    return ConnectionLostScreen{"Connexion perdue", "Tentative " + std::to_string(attempt) + "/3", attempt, 3};
}

ConnectionLostScreen LostScreenDimming(int attempt, DimmedScreen dimmed, bool bannerOnly = false)
{
    ConnectionLostScreen screen = LostScreen(attempt);
    screen.dimmed = dimmed;
    screen.bannerOnly = bannerOnly;
    return screen;
}
} // namespace

class RetainingScreenTest : public ::testing::Test
{
protected:
    NiceMock<MockScreen> inner;
    RetainingScreen screen{inner};
};

TEST_F(RetainingScreenTest, ForwardsAScreenWithItsTopBarAsItIs)
{
    EXPECT_CALL(inner, Display(TopBar::ForPlayer("Camille"), ScreenModel{TextScreen{"Connecté"}})).Times(1);

    screen.Display(TopBar::ForPlayer("Camille"), TextScreen{"Connecté"});
}

TEST_F(RetainingScreenTest, ForwardsTheOutOfGameScreenAsItIs)
{
    const ScreenModel outOfGame = OutOfGameScreen{"Hors jeu", "cause", "sortie"};
    EXPECT_CALL(inner, Display(CutTopBar, outOfGame)).Times(1);

    screen.Display(CutTopBar, outOfGame);
}

TEST_F(RetainingScreenTest, AnUnchangedScreenIsNotForwarded)
{
    EXPECT_CALL(inner, Display(_, _)).Times(0);

    screen.Display(TopBar::Hidden(), ScreenUnchanged{});
}

TEST_F(RetainingScreenTest, TheConnectionLostScreenDimsTheLastTextScreen)
{
    screen.Display(TopBar::Hidden(), TextScreen{"Connecté"});

    EXPECT_CALL(inner, Display(CutTopBar, ScreenModel{LostScreenDimming(1, DimmedScreen{"Connecté", {}, std::nullopt})})).Times(1);

    screen.Display(CutTopBar, LostScreen(1));
}

TEST_F(RetainingScreenTest, TheConnectionLostScreenDimsTheLastScreenWhoeverProducedIt)
{
    screen.Display(TopBar::Hidden(), TextScreen{"Connecté"});
    screen.Display(TopBar::Hidden(), ZonesScreen{FourZones});
    screen.Display(TopBar::Hidden(), ChosenZoneScreen{3});
    screen.Display(TopBar::Hidden(), TextScreen{"Buzz envoyé"});

    EXPECT_CALL(inner, Display(_, ScreenModel{LostScreenDimming(1, DimmedScreen{"Buzz envoyé", {}, std::nullopt})})).Times(1);

    screen.Display(CutTopBar, LostScreen(1));
}

TEST_F(RetainingScreenTest, TheConnectionLostScreenDimsTheQuestionZones)
{
    screen.Display(TopBar::Hidden(), ZonesScreen{FourZones});

    EXPECT_CALL(inner, Display(_, ScreenModel{LostScreenDimming(2, DimmedScreen{"", FourZones, std::nullopt})})).Times(1);

    screen.Display(CutTopBar, LostScreen(2));
}

TEST_F(RetainingScreenTest, AnUnchangedScreenDoesNotReplaceTheRetainedScreen)
{
    screen.Display(TopBar::Hidden(), TextScreen{"Connecté"});
    screen.Display(TopBar::Hidden(), ScreenUnchanged{});

    EXPECT_CALL(inner, Display(_, ScreenModel{LostScreenDimming(1, DimmedScreen{"Connecté", {}, std::nullopt})})).Times(1);

    screen.Display(CutTopBar, LostScreen(1));
}

TEST_F(RetainingScreenTest, ThereIsNothingToDimBeforeAnyScreen)
{
    EXPECT_CALL(inner, Display(CutTopBar, ScreenModel{LostScreenDimming(1, DimmedScreen{})})).Times(1);

    screen.Display(CutTopBar, LostScreen(1));
}

TEST_F(RetainingScreenTest, TheResultOfAQuestionIsDimmedAsTheZonesWithTheRightAnswerHighlightedWithoutItsBanner)
{
    screen.Display(TopBar::Hidden(), ZonesScreen{FourZones});
    screen.Display(TopBar::Hidden(), HighlightedZoneScreen{1, "Bonne réponse : B"});

    EXPECT_CALL(inner, Display(_, ScreenModel{LostScreenDimming(1, DimmedScreen{"", FourZones, 1})})).Times(1);

    screen.Display(CutTopBar, LostScreen(1));
}

TEST_F(RetainingScreenTest, ABannerScreenIsDimmedAsTheZonesWithTheChosenZoneHighlighted)
{
    screen.Display(TopBar::Hidden(), ZonesScreen{FourZones});
    screen.Display(TopBar::Hidden(), ChosenZoneScreen{2});
    screen.Display(TopBar::Hidden(), BannerScreen{"Réponse enregistrée"});

    EXPECT_CALL(inner, Display(_, ScreenModel{LostScreenDimming(1, DimmedScreen{"", FourZones, 2})})).Times(1);

    screen.Display(CutTopBar, LostScreen(1));
}

TEST_F(RetainingScreenTest, AHighlightedZoneScreenIsForwardedWithTheLastZonesDisplayed)
{
    screen.Display(TopBar::Hidden(), ZonesScreen{FourZones});

    EXPECT_CALL(inner, Display(TopBar::Hidden(), ScreenModel{HighlightedZoneScreen{1, "Bonne réponse : B", FourZones}})).Times(1);

    screen.Display(TopBar::Hidden(), HighlightedZoneScreen{1, "Bonne réponse : B"});
}

TEST_F(RetainingScreenTest, AChosenZoneScreenIsForwardedWithTheLastZonesDisplayed)
{
    screen.Display(TopBar::Hidden(), ZonesScreen{FourZones});

    EXPECT_CALL(inner, Display(TopBar::Hidden(), ScreenModel{ChosenZoneScreen{2, FourZones}})).Times(1);

    screen.Display(TopBar::Hidden(), ChosenZoneScreen{2});
}

TEST_F(RetainingScreenTest, ABannerScreenIsForwardedWithTheLastZonesAndTheChosenZone)
{
    screen.Display(TopBar::Hidden(), ZonesScreen{FourZones});
    screen.Display(TopBar::Hidden(), ChosenZoneScreen{0});

    EXPECT_CALL(inner, Display(TopBar::Hidden(), ScreenModel{BannerScreen{"Réponse enregistrée", FourZones, 0}})).Times(1);

    screen.Display(TopBar::Hidden(), BannerScreen{"Réponse enregistrée"});
}

TEST_F(RetainingScreenTest, ABannerScreenWithoutAChosenZoneIsForwardedWithoutOne)
{
    screen.Display(TopBar::Hidden(), ZonesScreen{FourZones});

    EXPECT_CALL(inner, Display(TopBar::Hidden(), ScreenModel{BannerScreen{"Réponse enregistrée", FourZones, std::nullopt}})).Times(1);

    screen.Display(TopBar::Hidden(), BannerScreen{"Réponse enregistrée"});
}

TEST_F(RetainingScreenTest, NewZonesForgetTheChosenZoneOfTheQuestionBefore)
{
    screen.Display(TopBar::Hidden(), ZonesScreen{FourZones});
    screen.Display(TopBar::Hidden(), ChosenZoneScreen{0});
    const std::vector<std::string> nextZones{"A : x", "B : y", "C : z", "D : w"};
    screen.Display(TopBar::Hidden(), ZonesScreen{nextZones});

    EXPECT_CALL(inner, Display(TopBar::Hidden(), ScreenModel{BannerScreen{"Réponse enregistrée", nextZones, std::nullopt}})).Times(1);

    screen.Display(TopBar::Hidden(), BannerScreen{"Réponse enregistrée"});
}

TEST_F(RetainingScreenTest, ATextScreenBetweenTheZonesAndTheResultDoesNotForgetTheZones)
{
    screen.Display(TopBar::Hidden(), ZonesScreen{FourZones});
    screen.Display(TopBar::Hidden(), TextScreen{"Trop tard"});

    EXPECT_CALL(inner, Display(TopBar::Hidden(), ScreenModel{HighlightedZoneScreen{3, "Bonne réponse : D", FourZones}})).Times(1);

    screen.Display(TopBar::Hidden(), HighlightedZoneScreen{3, "Bonne réponse : D"});
}

TEST_F(RetainingScreenTest, AResultWithoutAnyZonesBeforeIsForwardedWithoutZones)
{
    EXPECT_CALL(inner, Display(TopBar::Hidden(), ScreenModel{HighlightedZoneScreen{3, "Bonne réponse : D", {}}})).Times(1);

    screen.Display(TopBar::Hidden(), HighlightedZoneScreen{3, "Bonne réponse : D"});
}

TEST_F(RetainingScreenTest, TheNextConnectionLostScreenOnlyRepaintsTheBannerAndKeepsTheDimmedScreenOfBeforeTheLoss)
{
    screen.Display(TopBar::Hidden(), ZonesScreen{FourZones});
    screen.Display(CutTopBar, LostScreen(1));

    EXPECT_CALL(inner, Display(CutTopBar, ScreenModel{LostScreenDimming(2, DimmedScreen{"", FourZones, std::nullopt}, true)})).Times(1);

    screen.Display(CutTopBar, LostScreen(2));
}

TEST_F(RetainingScreenTest, TheFirstConnectionLostScreenRepaintsEverything)
{
    screen.Display(TopBar::Hidden(), TextScreen{"Connecté"});

    EXPECT_CALL(inner, Display(_, ScreenModel{LostScreenDimming(1, DimmedScreen{"Connecté", {}, std::nullopt}, false)})).Times(1);

    screen.Display(CutTopBar, LostScreen(1));
}

TEST_F(RetainingScreenTest, ALossAfterTheRecoveryRepaintsEverythingAgain)
{
    screen.Display(TopBar::Hidden(), TextScreen{"Connecté"});
    screen.Display(CutTopBar, LostScreen(1));
    screen.Display(CutTopBar, LostScreen(2));
    screen.Display(TopBar::ForPlayer("Camille"), TextScreen{"Connecté"});

    EXPECT_CALL(inner, Display(_, ScreenModel{LostScreenDimming(1, DimmedScreen{"Connecté", {}, std::nullopt}, false)})).Times(1);

    screen.Display(CutTopBar, LostScreen(1));
}

TEST_F(RetainingScreenTest, AnUnchangedScreenBetweenTwoConnectionLostScreensStillRepaintsOnlyTheBanner)
{
    screen.Display(TopBar::Hidden(), TextScreen{"Connecté"});
    screen.Display(CutTopBar, LostScreen(1));
    screen.Display(TopBar::Hidden(), ScreenUnchanged{});

    EXPECT_CALL(inner, Display(_, ScreenModel{LostScreenDimming(2, DimmedScreen{"Connecté", {}, std::nullopt}, true)})).Times(1);

    screen.Display(CutTopBar, LostScreen(2));
}

TEST_F(RetainingScreenTest, TheOutOfGameScreenAfterTheConnectionLostScreenIsDrawnInFull)
{
    screen.Display(TopBar::Hidden(), TextScreen{"Connecté"});
    screen.Display(CutTopBar, LostScreen(1));

    const ScreenModel outOfGame = OutOfGameScreen{"Hors jeu", "cause", "sortie"};
    EXPECT_CALL(inner, Display(CutTopBar, outOfGame)).Times(1);

    screen.Display(CutTopBar, outOfGame);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
