#ifndef GFX_SCREEN_H
#define GFX_SCREEN_H

#include "GfxFontMetrics.h"
#include "ProportionalTextMeasurer.h"
#include "Screen.h"
#include "TextLayout.h"
#include "fonts/OswaldRegular30.h"
#include "fonts/OswaldRegular38.h"
#include "fonts/OswaldRegular48.h"
#include <string>
#include <LovyanGFX.hpp>

class GfxScreen : public Screen
{
private:
    static const int margin = 10;            // px laissés libres sur chaque bord de la dalle

    lgfx::LGFX_Device &lcd;
    const lgfx::GFXfont *fonts[3]; // de la plus grande taille (0) à la plus petite
    ProportionalTextMeasurer measurer;

public:
    GfxScreen(lgfx::LGFX_Device &lcd)
        : lcd(lcd),
          fonts{&OswaldRegular48, &OswaldRegular38, &OswaldRegular30},
          measurer({ReadFontMetrics(OswaldRegular48, QuizLineSpacingPercent),
                    ReadFontMetrics(OswaldRegular38, QuizLineSpacingPercent),
                    ReadFontMetrics(OswaldRegular30, QuizLineSpacingPercent)})
    {
    }

    void Show(const std::string &text) override
    {
        const TextLayout layout(measurer, lcd.width() - 2 * margin, lcd.height() - 2 * margin);
        const TextLines placed = layout.Compute(text); // taille et lignes

        lcd.fillScreen(TFT_BLACK); // efface l'écran (pas d'empilement)
        lcd.setFont(fonts[placed.size]);
        lcd.setTextSize(1);
        lcd.setTextDatum(middle_center); // point d'ancrage = centre de la boîte d'encre de la police, sur chaque ligne
        const int lineHeight = measurer.LineHeight(placed.size); // pas entre deux lignes (pas fontHeight(), qui couvre tous les glyphes)
        const int count = static_cast<int>(placed.lines.size());
        const int top = (lcd.height() - lineHeight * count) / 2; // bloc de lignes centré verticalement
        for (int i = 0; i < count; ++i)
            lcd.drawString(placed.lines[i].c_str(),
                           lcd.width() / 2,                       // x = centre horizontal de la dalle
                           top + lineHeight * i + lineHeight / 2); // y = centre de la ligne i
    }
};

#endif
