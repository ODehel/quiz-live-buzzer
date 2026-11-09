#ifndef TAPPABLE_ZONES_SCREEN_H
#define TAPPABLE_ZONES_SCREEN_H

#include <string>
#include <vector>
#include <variant>
#include "Overloaded.h"
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

    // Seul un `ZonesScreen` rend des zones tactiles (« Zones actives », CDC §8.2) : tout autre affichage les désactive.
    // Un `ScreenUnchanged` ne touche ni l'écran ni les zones.
    void Display(const ScreenModel &screenModel) override
    {
        std::visit(Overloaded{
                       [](const ScreenUnchanged &) {},
                       [this, &screenModel](const ZonesScreen &zonesScreen)
                       {
                           screen.Display(screenModel);
                           ActivateZonesFor(zonesScreen.zoneTexts);
                       },
                       [this, &screenModel](const auto &)
                       {
                           screen.Display(screenModel);
                           zones.clear();
                       }},
                   screenModel);
    }

private:
    // Une zone tactile par texte affiché (quatre au plus, comme `GfxScreen`), sur le même découpage que le dessin.
    void ActivateZonesFor(const std::vector<std::string> &zoneTexts)
    {
        const ButtonInput buttons[] = {ButtonInput::A, ButtonInput::B, ButtonInput::C, ButtonInput::D};
        const size_t zoneCount = sizeof(buttons) / sizeof(buttons[0]);
        zones.clear();
        for (size_t i = 0; i < zoneTexts.size() && i < zoneCount; ++i)
        {
            const ZoneRect rect = grid.At(i);
            zones.push_back({rect.x, rect.y, rect.x + rect.width, rect.y + rect.height, buttons[i]});
        }
    }
};

#endif
