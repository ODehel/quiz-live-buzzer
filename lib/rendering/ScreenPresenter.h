#ifndef SCREEN_PRESENTER_H
#define SCREEN_PRESENTER_H

#include "DisplayState.h"
#include "HubMessage.h"
#include "ScreenModel.h"
#include "TopBar.h"

class ScreenPresenter
{
private:
    DisplayState displayState;

public:
    ScreenModel ScreenFor(const HubMessage &message, bool questionArmed) const
    {
        if (message.Type() == "timer_end")
            return questionArmed ? ScreenModel{TextScreen{displayState.LcdTextForTimeUp()}} : ScreenModel{ScreenUnchanged{}};
        const std::vector<std::string> zones = displayState.ZoneTextsFor(message);
        if (!zones.empty())
            return ZonesScreen{zones};
        const std::string text = displayState.LcdTextFor(message);
        if (displayState.KeepsZonesFor(message))
            return BannerScreen{text};
        const int highlightedZone = displayState.HighlightedZoneFor(message);
        if (highlightedZone != DisplayState::NoZone)
            return HighlightedZoneScreen{static_cast<size_t>(highlightedZone), text};
        if (text.empty())
            return ScreenUnchanged{};
        return TextScreen{text};
    }

    // Seul l'écran « Connecté » porte le bandeau du haut (CDC §8.2) : le nom vient de l'`username` d'`auth_success`.
    TopBar TopBarFor(const HubMessage &message) const
    {
        if (message.Type() == "auth_success")
            return TopBar::ForPlayer(message.Username());
        return TopBar::Hidden();
    }

    ScreenModel ScreenForBuzzSent() const
    {
        return TextScreen{displayState.LcdTextForBuzzSent()};
    }

    ScreenModel ScreenForLoginRefused() const
    {
        return TextScreen{displayState.LcdTextForLoginRefused()};
    }

    ScreenModel ScreenForTappedAnswer(char letter) const
    {
        const int zone = displayState.ZoneIndexForAnswer(letter);
        if (zone == DisplayState::NoZone)
            return ScreenUnchanged{};
        return ChosenZoneScreen{static_cast<size_t>(zone)};
    }
};

#endif
