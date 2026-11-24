#ifndef GFX_SCREEN_H
#define GFX_SCREEN_H

#include "GfxFontMetrics.h"
#include "Overloaded.h"
#include "ProportionalTextMeasurer.h"
#include "Screen.h"
#include "ScreenModel.h"
#include "TextLayout.h"
#include "ZoneGrid.h"
#include "fonts/OswaldRegular30.h"
#include "fonts/OswaldRegular38.h"
#include "fonts/OswaldRegular48.h"
#include <cmath>
#include <string>
#include <variant>
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
    static const int topBarHeight = 60;      // hauteur du bandeau du haut, trait de séparation compris

    // « Hors jeu » (US-FW-015 CA-12) : une pile centrée de 362 px, dont les mesures viennent de la spec.
    static const int ringDiameter = 128;     // anneau : diamètre extérieur, trait de ringStroke
    static const int ringStroke = 9;
    static const int stackGap = 24;          // écart entre l'anneau, le titre et les deux lignes
    static const int titleBoxHeight = 62;    // boîte d'encre du titre en 48 px
    static const int lineBoxHeight = 50;     // boîte d'encre d'une ligne en 38 px
    static const int outOfGameStackHeight = ringDiameter + stackGap + titleBoxHeight + stackGap + lineBoxHeight + stackGap + lineBoxHeight;

    lgfx::LGFX_Device &lcd;
    const lgfx::GFXfont *fonts[3]; // de la plus grande taille (0) à la plus petite
    ProportionalTextMeasurer measurer;
    std::vector<std::string> lastZoneTexts; // textes du dernier ShowZones, redessinés par HighlightZone
    size_t lastChosenZone = noZone;         // zone touchée par le joueur (HighlightChosenZone), effacée par ShowZones

public:
    GfxScreen(lgfx::LGFX_Device &lcd)
        : lcd(lcd),
          fonts{&OswaldRegular48, &OswaldRegular38, &OswaldRegular30},
          measurer({ReadFontMetrics(OswaldRegular48, QuizLineSpacingPercent),
                    ReadFontMetrics(OswaldRegular38, QuizLineSpacingPercent),
                    ReadFontMetrics(OswaldRegular30, QuizLineSpacingPercent)})
    {
    }

    // Un type d'écran, un dessin : le modèle dit quoi montrer, les méthodes privées savent comment.
    // Le bandeau du haut n'est dessiné qu'avec un `TextScreen` et « Hors jeu » (les seuls écrans qui le portent aujourd'hui) ; les autres écrans l'ignorent.
    void Display(const TopBar &topBar, const ScreenModel &screenModel) override
    {
        std::visit(Overloaded{
                       [](const ScreenUnchanged &) {},
                       [this, &topBar](const TextScreen &textScreen) { Show(topBar, textScreen.text); },
                       [this](const ZonesScreen &zonesScreen) { ShowZones(zonesScreen.zoneTexts); },
                       [this](const HighlightedZoneScreen &highlightedZoneScreen)
                       { HighlightZone(highlightedZoneScreen.zoneIndex, highlightedZoneScreen.bannerText); },
                       [this](const ChosenZoneScreen &chosenZoneScreen) { HighlightChosenZone(chosenZoneScreen.zoneIndex); },
                       [this](const BannerScreen &bannerScreen) { ShowBanner(bannerScreen.bannerText); },
                       [this, &topBar](const OutOfGameScreen &outOfGameScreen) { ShowOutOfGame(topBar, outOfGameScreen); }},
                   screenModel);
    }

private:
    void Show(const TopBar &topBar, const std::string &text)
    {
        lcd.fillScreen(TFT_BLACK); // efface l'écran (pas d'empilement)
        if (!topBar.visible)
        {
            DrawTextIn(text, 0, 0, lcd.width(), lcd.height());
            return;
        }
        DrawTopBar(topBar);
        DrawTextIn(text, 0, topBarHeight, lcd.width(), lcd.height() - topBarHeight);
    }

    // Fond noir, bandeau du haut, puis la pile centrée dans le reste de la dalle : anneau rose `#ff8fa6` avec le symbole marche/arrêt,
    // le titre en 48 px, la cause et la sortie en 38 px, en blanc, une ligne chacun. La liaison barrée du bandeau n'est pas dessinée (US-FW-009 CA-8).
    void ShowOutOfGame(const TopBar &topBar, const OutOfGameScreen &screen)
    {
        lcd.fillScreen(TFT_BLACK);
        int areaTop = 0;
        if (topBar.visible)
        {
            DrawTopBar(topBar);
            areaTop = topBarHeight;
        }
        const int stackTop = areaTop + (lcd.height() - areaTop - outOfGameStackHeight) / 2;
        const int centerX = lcd.width() / 2;
        const int ringCenterY = stackTop + ringDiameter / 2;
        DrawPowerRing(centerX, ringCenterY);

        const int titleTop = stackTop + ringDiameter + stackGap;
        const int causeTop = titleTop + titleBoxHeight + stackGap;
        const int wayOutTop = causeTop + lineBoxHeight + stackGap;
        lcd.setTextSize(1);
        lcd.setTextColor(TFT_WHITE);
        lcd.setTextDatum(middle_center);
        lcd.setFont(fonts[0]);
        lcd.drawString(screen.title.c_str(), centerX, titleTop + titleBoxHeight / 2);
        lcd.setFont(fonts[1]);
        lcd.drawString(screen.cause.c_str(), centerX, causeTop + lineBoxHeight / 2);
        lcd.drawString(screen.wayOut.c_str(), centerX, wayOutTop + lineBoxHeight / 2);
    }

    // L'anneau de 128 px (trait de 9 px) et, dedans, le symbole marche/arrêt de la maquette `fail-02-out.html` (viewBox 36, échelle
    // 128/36) : un arc ouvert en haut (rayon 29 px, centre 3 px sous celui de l'anneau, ouverture de 86°) et une barre verticale de 30 px
    // qui part du haut du centre ; même trait, bouts ronds. Dessinés au pinceau rond (des disques), sans dépendre du sens des angles de `fillArc`.
    void DrawPowerRing(int centerX, int centerY)
    {
        const uint32_t ringColor = lcd.color888(0xff, 0x8f, 0xa6);
        const int outerRadius = ringDiameter / 2;
        lcd.fillCircle(centerX, centerY, outerRadius, ringColor);
        lcd.fillCircle(centerX, centerY, outerRadius - ringStroke, TFT_BLACK);

        const int brushRadius = ringStroke / 2;
        const int arcRadius = 29;
        const int arcCenterY = centerY + 3;
        const float arcStartDegrees = 43.0f; // depuis la verticale, vers la droite, dans le sens des aiguilles d'une montre
        const float arcEndDegrees = 317.0f;
        const float degreesToRadians = 3.14159265f / 180.0f;
        for (float degrees = arcStartDegrees; degrees <= arcEndDegrees; degrees += 1.0f)
        {
            const float radians = degrees * degreesToRadians;
            lcd.fillCircle(centerX + static_cast<int>(lroundf(arcRadius * sinf(radians))),
                           arcCenterY - static_cast<int>(lroundf(arcRadius * cosf(radians))),
                           brushRadius, ringColor);
        }
        const int barTop = centerY - 30;
        lcd.fillRect(centerX - brushRadius, barTop, ringStroke, centerY - barTop, ringColor);
        lcd.fillCircle(centerX, barTop, brushRadius, ringColor);
        lcd.fillCircle(centerX, centerY, brushRadius, ringColor);
    }

    // Le nom du joueur à gauche, « Buzz’O’Live » au centre, un trait dessous ; en petite police, sur une seule ligne.
    void DrawTopBar(const TopBar &topBar)
    {
        lcd.setFont(fonts[2]);
        lcd.setTextSize(1);
        lcd.setTextColor(TFT_WHITE);
        const int middle = (topBarHeight - 1) / 2;
        lcd.setTextDatum(middle_left);
        lcd.drawString(topBar.playerName.c_str(), margin, middle);
        lcd.setTextDatum(middle_center);
        lcd.drawString("Buzz’O’Live", lcd.width() / 2, middle);
        lcd.drawFastHLine(0, topBarHeight - 1, lcd.width(), TFT_WHITE);
    }

    // Une zone par texte, en grille 2x2 : A en haut à gauche, B en haut à droite, C en bas à gauche, D en bas à droite
    // (`ZoneGrid`, le même découpage que les zones tactiles de `TappableZonesScreen`). Au-delà de quatre textes, les suivants ne sont pas dessinés.
    void ShowZones(const std::vector<std::string> &zoneTexts)
    {
        lastZoneTexts = zoneTexts;
        lastChosenZone = noZone;
        lcd.fillScreen(TFT_BLACK);
        DrawZones(0, lcd.height(), noZone);
    }

    // Texte dans un bandeau en haut, les dernières zones affichées dessous (hauteur réduite), la zone zoneIndex en blanc plein.
    void HighlightZone(size_t zoneIndex, const std::string &text)
    {
        const int bandHeight = lcd.height() / resultBandDivisor;
        lcd.fillScreen(TFT_BLACK);
        DrawTextIn(text, 0, 0, lcd.width(), bandHeight);
        DrawZones(bandHeight, lcd.height() - bandHeight, zoneIndex);
    }

    // Les dernières zones affichées en plein écran, la zone zoneIndex (celle que le joueur a touchée) en blanc plein, sans texte ajouté.
    void HighlightChosenZone(size_t zoneIndex)
    {
        lastChosenZone = zoneIndex;
        lcd.fillScreen(TFT_BLACK);
        DrawZones(0, lcd.height(), zoneIndex);
    }

    // Texte dans un bandeau en haut, les dernières zones affichées dessous, la zone choisie par le joueur toujours en évidence.
    void ShowBanner(const std::string &text)
    {
        HighlightZone(lastChosenZone, text);
    }

    // Dessine les zones de lastZoneTexts dans la bande (top, areaHeight), sans effacer l'écran.
    void DrawZones(int top, int areaHeight, size_t highlightedZone)
    {
        const ZoneGrid grid(lcd.width(), top, areaHeight);
        for (size_t i = 0; i < lastZoneTexts.size() && i < zoneCount; ++i)
        {
            const ZoneRect zone = grid.At(i);
            const bool highlighted = (i == highlightedZone);
            const int frameX = zone.x + zoneBorderInset;
            const int frameY = zone.y + zoneBorderInset;
            const int frameWidth = zone.width - 2 * zoneBorderInset;
            const int frameHeight = zone.height - 2 * zoneBorderInset;
            if (highlighted)
                lcd.fillRect(frameX, frameY, frameWidth, frameHeight, TFT_WHITE);
            else
                lcd.drawRect(frameX, frameY, frameWidth, frameHeight, TFT_WHITE);
            DrawTextIn(lastZoneTexts[i], zone.x, zone.y, zone.width, zone.height, highlighted ? TFT_BLACK : TFT_WHITE);
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
