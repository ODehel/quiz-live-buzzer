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
#include <vector>
#include <LovyanGFX.hpp>

class GfxScreen : public Screen
{
private:
    static const int margin = 10;            // px laissés libres sur chaque bord de la dalle ou de la zone
    static const size_t zoneCount = 4;       // zones A à D
    static const int zoneBorderInset = 2;    // px entre le bord de la zone et son cadre
    static const int resultBandDivisor = 4;  // le bandeau de résultat occupe 1/4 de la hauteur de la dalle, les zones le reste
    static const size_t noZone = zoneCount;  // indice hors grille : aucune zone en évidence

    lgfx::LGFX_Device &lcd;
    const lgfx::GFXfont *fonts[3]; // de la plus grande taille (0) à la plus petite
    ProportionalTextMeasurer measurer;
    std::vector<std::string> lastZoneTexts; // textes du dernier ShowZones, redessinés par HighlightZone

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
        lcd.fillScreen(TFT_BLACK); // efface l'écran (pas d'empilement)
        DrawTextIn(text, 0, 0, lcd.width(), lcd.height());
    }

    // Une zone par texte, en grille 2x2 : A en haut à gauche, B en haut à droite, C en bas à gauche, D en bas à droite
    // (même découpage que les zones tactiles de main.cpp). Au-delà de quatre textes, les suivants ne sont pas dessinés.
    void ShowZones(const std::vector<std::string> &zoneTexts) override
    {
        lastZoneTexts = zoneTexts;
        lcd.fillScreen(TFT_BLACK);
        DrawZones(0, lcd.height(), noZone);
    }

    // Texte dans un bandeau en haut, les dernières zones affichées dessous (hauteur réduite), la zone zoneIndex en blanc plein.
    void HighlightZone(size_t zoneIndex, const std::string &text) override
    {
        const int bandHeight = lcd.height() / resultBandDivisor;
        lcd.fillScreen(TFT_BLACK);
        DrawTextIn(text, 0, 0, lcd.width(), bandHeight);
        DrawZones(bandHeight, lcd.height() - bandHeight, zoneIndex);
    }

private:
    // Dessine les zones de lastZoneTexts dans la bande (top, areaHeight), sans effacer l'écran.
    void DrawZones(int top, int areaHeight, size_t highlightedZone)
    {
        const int zoneWidth = lcd.width() / 2;
        const int zoneHeight = areaHeight / 2;
        for (size_t i = 0; i < lastZoneTexts.size() && i < zoneCount; ++i)
        {
            const int x = static_cast<int>(i % 2) * zoneWidth;
            const int y = top + static_cast<int>(i / 2) * zoneHeight;
            const bool highlighted = (i == highlightedZone);
            const int frameX = x + zoneBorderInset;
            const int frameY = y + zoneBorderInset;
            const int frameWidth = zoneWidth - 2 * zoneBorderInset;
            const int frameHeight = zoneHeight - 2 * zoneBorderInset;
            if (highlighted)
                lcd.fillRect(frameX, frameY, frameWidth, frameHeight, TFT_WHITE);
            else
                lcd.drawRect(frameX, frameY, frameWidth, frameHeight, TFT_WHITE);
            DrawTextIn(lastZoneTexts[i], x, y, zoneWidth, zoneHeight, highlighted ? TFT_BLACK : TFT_WHITE);
        }
    }

    // Dessine le texte, centré dans le rectangle (x, y, width, height), sans effacer l'écran.
    void DrawTextIn(const std::string &text, int x, int y, int width, int height, int color = TFT_WHITE)
    {
        const TextLayout layout(measurer, width - 2 * margin, height - 2 * margin);
        const TextLines placed = layout.Compute(text); // taille et lignes

        lcd.setFont(fonts[placed.size]);
        lcd.setTextSize(1);
        lcd.setTextColor(color);
        lcd.setTextDatum(middle_center); // point d'ancrage = centre de la boîte d'encre de la police, sur chaque ligne
        const int lineHeight = measurer.LineHeight(placed.size); // pas entre deux lignes (pas fontHeight(), qui couvre tous les glyphes)
        const int count = static_cast<int>(placed.lines.size());
        const int top = y + (height - lineHeight * count) / 2; // bloc de lignes centré verticalement
        for (int i = 0; i < count; ++i)
            lcd.drawString(placed.lines[i].c_str(),
                           x + width / 2,                         // x = centre horizontal du rectangle
                           top + lineHeight * i + lineHeight / 2); // y = centre de la ligne i
    }
};

#endif
