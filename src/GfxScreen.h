#ifndef GFX_SCREEN_H
#define GFX_SCREEN_H

#include "Screen.h"
#include "TextLayout.h"
#include <string>
#include <LovyanGFX.hpp>

class GfxScreen : public Screen
{
private:
    static const int margin = 10; // px laissés libres sur chaque bord de la dalle

    lgfx::LGFX_Device &lcd;

public:
    GfxScreen(lgfx::LGFX_Device &lcd) : lcd(lcd)
    {
    }

    void Show(const std::string &text) override
    {
        lcd.setTextSize(1); // cellule de base de la police, pour mesurer
        const TextLayout layout(lcd.width() - 2 * margin, lcd.height() - 2 * margin, lcd.fontWidth(), lcd.fontHeight());
        const TextLines placed = layout.Compute(text); // échelle et lignes

        lcd.fillScreen(TFT_BLACK); // efface l'écran (pas d'empilement)
        lcd.setTextSize(placed.scale);
        lcd.setTextDatum(middle_center); // point d'ancrage = centre de chaque ligne
        const int lineHeight = lcd.fontHeight();
        const int count = static_cast<int>(placed.lines.size());
        const int top = (lcd.height() - lineHeight * count) / 2; // bloc de lignes centré verticalement
        for (int i = 0; i < count; ++i)
            lcd.drawString(placed.lines[i].c_str(),
                           lcd.width() / 2,                       // x = centre horizontal de la dalle
                           top + lineHeight * i + lineHeight / 2); // y = centre de la ligne i
    }
};

#endif
