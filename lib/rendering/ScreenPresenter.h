#ifndef SCREEN_PRESENTER_H
#define SCREEN_PRESENTER_H

#include "DisplayState.h"
#include "HubMessage.h"
#include "ScreenModel.h"

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

    ScreenModel ScreenForBuzzSent() const
    {
        return TextScreen{displayState.LcdTextForBuzzSent()};
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
