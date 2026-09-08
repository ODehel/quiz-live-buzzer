#ifndef GFX_SCREEN_H
#define GFX_SCREEN_H

#include "Screen.h"
#include <string>
#include <LovyanGFX.hpp>

class GfxScreen : public Screen
{
private:
    lgfx::LGFX_Device &lcd;

public:
    GfxScreen(lgfx::LGFX_Device &lcd) : lcd(lcd)
    {
    }

    void Show(const std::string &text) override
    {
        lcd.fillScreen(TFT_BLACK);        // efface l'écran (pas d'empilement)
        lcd.setTextSize(5);              // définit la taille de police
        lcd.setTextDatum(middle_center);  // point d'ancrage = centre du texte
        lcd.drawString(text.c_str(),      // le texte à afficher
                       lcd.width() / 2,   // x = centre horizontal de la dalle
                       lcd.height() / 2); // y = centre vertical de la dalle
    }
};

#endif