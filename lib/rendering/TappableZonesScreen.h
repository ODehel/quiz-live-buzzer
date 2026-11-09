#ifndef TAPPABLE_ZONES_SCREEN_H
#define TAPPABLE_ZONES_SCREEN_H

#include <string>
#include <vector>
#include "Screen.h"
#include "TouchZoneReader.h"
#include "ZoneGrid.h"

class TappableZonesScreen : public Screen
{
private:
    Screen &screen;
    ZoneGrid grid;
    std::vector<TouchZone> zones;

public:
    TappableZonesScreen(Screen &screen, int panelWidth, int panelHeight) : screen(screen), grid(panelWidth, 0, panelHeight)
    {
    }

    std::vector<TouchZone> &Zones()
    {
        return zones;
    }

    void Show(const std::string &text) override
    {
        screen.Show(text);
        zones.clear();
    }

    // Seul `ShowZones` rend des zones tactiles (« Zones actives », CDC §8.2) : tout autre affichage les désactive.
    // Une zone tactile par texte affiché (quatre au plus, comme `GfxScreen`), sur le même découpage que le dessin.
    void ShowZones(const std::vector<std::string> &zoneTexts) override
    {
        screen.ShowZones(zoneTexts);
        const ButtonInput buttons[] = {ButtonInput::A, ButtonInput::B, ButtonInput::C, ButtonInput::D};
        const size_t zoneCount = sizeof(buttons) / sizeof(buttons[0]);
        zones.clear();
        for (size_t i = 0; i < zoneTexts.size() && i < zoneCount; ++i)
        {
            const ZoneRect rect = grid.At(i);
            zones.push_back({rect.x, rect.y, rect.x + rect.width, rect.y + rect.height, buttons[i]});
        }
    }

    void HighlightZone(size_t zoneIndex, const std::string &text) override
    {
        screen.HighlightZone(zoneIndex, text);
        zones.clear();
    }

    void HighlightChosenZone(size_t zoneIndex) override
    {
        screen.HighlightChosenZone(zoneIndex);
        zones.clear();
    }

    void ShowBanner(const std::string &text) override
    {
        screen.ShowBanner(text);
        zones.clear();
    }
};

#endif
