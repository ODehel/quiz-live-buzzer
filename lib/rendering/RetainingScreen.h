#ifndef RETAINING_SCREEN_H
#define RETAINING_SCREEN_H

#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "LastScreenReader.h"
#include "Overloaded.h"
#include "Screen.h"

// Décorateur de `Screen` partagé par les deux producteurs d'écran (`LcdRenderer` et `ScreenLocalFeedback`) : il retient le dernier écran
// effectivement affiché, quel qu'en soit le producteur, pour que l'écran de coupure l'atténue (US-FW-015 CA-2). Il retient aussi les dernières
// propositions et la zone choisie, et les transmet aux écrans qui s'y redessinent : l'adaptateur n'en garde plus aucun état.
class RetainingScreen : public Screen, public LastScreenReader
{
private:
    Screen &screen;
    std::vector<std::string> zoneTexts;
    std::optional<size_t> chosenZone;
    DimmedScreen dimmed;
    bool connectionLostIsShown = false;
    ScreenModel lastScreen = ScreenUnchanged{};

public:
    RetainingScreen(Screen &screen) : screen(screen)
    {
    }

    const ScreenModel &LastScreen() const override
    {
        return lastScreen;
    }

    // Un `ScreenUnchanged` ne remplace pas l'écran retenu et n'est pas transmis.
    void Display(const TopBar &topBar, const ScreenModel &screenModel) override
    {
        if (std::holds_alternative<ScreenUnchanged>(screenModel))
            return;
        const bool wasConnectionLostShown = connectionLostIsShown;
        connectionLostIsShown = std::holds_alternative<ConnectionLostScreen>(screenModel);
        std::visit(Overloaded{
                       [](const ScreenUnchanged &) {},
                       [this, &topBar](const TextScreen &textScreen)
                       {
                           dimmed = DimmedScreen{textScreen.text, {}, std::nullopt};
                           Forward(topBar, textScreen);
                       },
                       [this, &topBar](const ZonesScreen &zonesScreen)
                       {
                           zoneTexts = zonesScreen.zoneTexts;
                           chosenZone = std::nullopt;
                           dimmed = DimmedScreen{"", zoneTexts, std::nullopt};
                           Forward(topBar, zonesScreen);
                       },
                       [this, &topBar](const HighlightedZoneScreen &highlightedZoneScreen)
                       {
                           dimmed = DimmedScreen{"", zoneTexts, highlightedZoneScreen.zoneIndex};
                           Forward(topBar, HighlightedZoneScreen{highlightedZoneScreen.zoneIndex, highlightedZoneScreen.bannerText, zoneTexts});
                       },
                       [this, &topBar](const ChosenZoneScreen &chosenZoneScreen)
                       {
                           chosenZone = chosenZoneScreen.zoneIndex;
                           dimmed = DimmedScreen{"", zoneTexts, chosenZone};
                           Forward(topBar, ChosenZoneScreen{chosenZoneScreen.zoneIndex, zoneTexts});
                       },
                       [this, &topBar](const BannerScreen &bannerScreen)
                       {
                           dimmed = DimmedScreen{"", zoneTexts, chosenZone};
                           Forward(topBar, BannerScreen{bannerScreen.bannerText, zoneTexts, chosenZone});
                       },
                       [this, &topBar](const OutOfGameScreen &outOfGameScreen) { Forward(topBar, outOfGameScreen); },
                       [this, &topBar](const LoginRefusedScreen &loginRefusedScreen) { Forward(topBar, loginRefusedScreen); },
                       [this, &topBar, wasConnectionLostShown](const ConnectionLostScreen &connectionLostScreen)
                       {
                           ConnectionLostScreen completed = connectionLostScreen;
                           completed.dimmed = dimmed;
                           completed.bannerOnly = wasConnectionLostShown;
                           Forward(topBar, completed);
                       }},
                   screenModel);
    }

private:
    // Tout écran transmis est retenu tel qu'il part, complété (US-FW-002 CA-10 : l'écran d'avant un appui est restitué à l'identique).
    void Forward(const TopBar &topBar, const ScreenModel &screenModel)
    {
        lastScreen = screenModel;
        screen.Display(topBar, screenModel);
    }
};

#endif
