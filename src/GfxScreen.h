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

    // Couleurs en RVB 888 (`uint32_t`) : LovyanGFX lit un `int` (comme `TFT_WHITE`) en RVB 565. Rouge, rose et gris sont des valeurs de maquette
    // (`mockup.css`), hors du monochrome des autres écrans (US-FW-015), jamais vues sur la dalle.
    static constexpr uint32_t colorWhite = 0xFFFFFF;
    static constexpr uint32_t colorBlack = 0x000000;
    static constexpr uint32_t colorDimmed = 0x666666; // le blanc à 40 %, pour le dernier écran atténué
    static constexpr uint32_t colorRed = 0xE8174F;    // `--wrong` : le bandeau de coupure
    static constexpr uint32_t colorPink = 0xFF8FA6;   // `--wrong-soft` : les icônes

    // L'écran de coupure (US-FW-015 CA-11) : le bandeau rouge de 60 px à y 60, à la place de l'en-tête ; le dernier écran atténué dessous.
    static const int lostBannerHeight = 60;
    static const int attemptDotDiameter = 16;
    static const int attemptDotGap = 8;
    static const int attemptLabelGap = 12;   // entre « Tentative N/3 » et les points
    static const int attemptDotOutline = 2;  // épaisseur du contour d'un point non plein

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
    // Le bandeau du haut n'est dessiné qu'avec un `TextScreen`, « Hors jeu » et la coupure (les seuls écrans qui le portent aujourd'hui) ; les autres écrans l'ignorent.
    // `GfxScreen` ne retient aucun écran : les propositions et la zone choisie arrivent dans le modèle (`RetainingScreen`).
    void Display(const TopBar &topBar, const ScreenModel &screenModel) override
    {
        std::visit(Overloaded{
                       [](const ScreenUnchanged &) {},
                       [this, &topBar](const TextScreen &textScreen) { Show(topBar, textScreen.text); },
                       [this](const ZonesScreen &zonesScreen) { ShowZones(zonesScreen.zoneTexts); },
                       [this](const HighlightedZoneScreen &highlightedZoneScreen)
                       { HighlightZone(highlightedZoneScreen.zoneTexts, highlightedZoneScreen.zoneIndex, highlightedZoneScreen.bannerText); },
                       [this](const ChosenZoneScreen &chosenZoneScreen) { HighlightChosenZone(chosenZoneScreen.zoneTexts, chosenZoneScreen.zoneIndex); },
                       [this](const BannerScreen &bannerScreen) { ShowBanner(bannerScreen); },
                       [this, &topBar](const OutOfGameScreen &outOfGameScreen) { ShowOutOfGame(topBar, outOfGameScreen); },
                       [this, &topBar](const ConnectionLostScreen &connectionLostScreen) { ShowConnectionLost(topBar, connectionLostScreen); }},
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
    // le titre en 48 px, la cause et la sortie en 38 px, en blanc, une ligne chacun.
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
        lcd.setTextColor(colorWhite);
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

    // Le nom du joueur à gauche, « Buzz’O’Live » au centre, la liaison à droite, un trait dessous ; en petite police, sur une seule ligne.
    void DrawTopBar(const TopBar &topBar)
    {
        lcd.setFont(fonts[2]);
        lcd.setTextSize(1);
        lcd.setTextColor(colorWhite);
        const int middle = (topBarHeight - 1) / 2;
        lcd.setTextDatum(middle_left);
        lcd.drawString(topBar.playerName.c_str(), margin, middle);
        lcd.setTextDatum(middle_center);
        lcd.drawString("Buzz’O’Live", lcd.width() / 2, middle);
        DrawLinkIcon(topBar.linkCut);
        lcd.drawFastHLine(0, topBarHeight - 1, lcd.width(), colorWhite);
    }

    // Un point du pinceau rond : le trait de l'icône est une suite de disques (comme `DrawPowerRing`), sans dépendre du sens des angles de `fillArc`.
    void Brush(float x, float y, int radius, uint32_t color)
    {
        lcd.fillCircle(static_cast<int>(lroundf(x)), static_cast<int>(lroundf(y)), radius, color);
    }

    // Un arc de la maquette (courbe de Bézier quadratique P0, P1, P2 en coordonnées de l'icône, mises à l'échelle) tracé au pinceau rond.
    void BrushQuadratic(float originX, float originY, float scale, float x0, float y0, float x1, float y1, float x2, float y2, int radius, uint32_t color)
    {
        const int steps = 24;
        for (int i = 0; i <= steps; ++i)
        {
            const float t = static_cast<float>(i) / steps;
            const float u = 1.0f - t;
            Brush(originX + scale * (u * u * x0 + 2 * u * t * x1 + t * t * x2),
                  originY + scale * (u * u * y0 + 2 * u * t * y1 + t * t * y2), radius, color);
        }
    }

    // L'icône de liaison de la maquette (`qcm-02-choices.html`, viewBox 24 × 18, mise à l'échelle 1,5 : 36 × 27 px, à droite du bandeau, marge de 10 px) :
    // trois arcs et un point. « Liée » : blanche. « Coupée » : arcs et point atténués à 55 %, barrée d'un trait plein, en rose `#ff8fa6`
    // (US-FW-009 CA-8 dit « rouge » ; la maquette et la géométrie de US-FW-015 donnent ce rose aux icônes).
    void DrawLinkIcon(bool cut)
    {
        const float scale = 1.5f;
        const float originX = static_cast<float>(lcd.width() - margin) - 24 * scale;
        const float originY = (static_cast<float>(topBarHeight - 1) - 18 * scale) / 2;
        const uint32_t strongColor = cut ? colorPink : colorWhite;
        const uint32_t waveColor = cut ? lcd.color888(0x8C, 0x4E, 0x5B) : colorWhite; // 55 % du rose sur noir
        const int waveRadius = 2;
        BrushQuadratic(originX, originY, scale, 2, 6.5f, 12, -1.5f, 22, 6.5f, waveRadius, waveColor);
        BrushQuadratic(originX, originY, scale, 5.5f, 10.5f, 12, 5, 18.5f, 10.5f, waveRadius, waveColor);
        BrushQuadratic(originX, originY, scale, 9, 14, 12, 11.5f, 15, 14, waveRadius, waveColor);
        Brush(originX + 12 * scale, originY + 16.6f * scale, waveRadius, waveColor);
        if (!cut)
            return;
        const int strikeRadius = 2;
        const int steps = 24;
        for (int i = 0; i <= steps; ++i)
        {
            const float t = static_cast<float>(i) / steps;
            Brush(originX + scale * (3 + 18 * t), originY + scale * (1.5f + 15.5f * t), strikeRadius, strongColor);
        }
    }

    // La coupure : fond noir, bandeau du haut (liaison barrée), le dernier écran atténué sous le bandeau rouge, puis le bandeau.
    // Quand la coupure est déjà à l'écran (`bannerOnly`), seul le bandeau est repeint : ni le bandeau du haut ni le contenu n'ont changé.
    void ShowConnectionLost(const TopBar &topBar, const ConnectionLostScreen &screen)
    {
        if (!screen.bannerOnly)
        {
            lcd.fillScreen(TFT_BLACK);
            DrawTopBar(topBar);
            DrawDimmed(screen.dimmed);
        }
        DrawLostBanner(screen);
    }

    // Le dernier écran, redessiné en gris sous le bandeau du haut et celui de coupure (de y 120 à la fin de la dalle) : le texte plein écran,
    // ou les propositions à leur place (zone en évidence : remplissage gris, texte noir). Le texte d'en-tête d'un résultat n'est pas redessiné :
    // le bandeau rouge prend la place de l'en-tête.
    void DrawDimmed(const DimmedScreen &dimmed)
    {
        const int top = topBarHeight + lostBannerHeight;
        const int height = lcd.height() - top;
        if (!dimmed.zoneTexts.empty())
            DrawZones(dimmed.zoneTexts, top, height, dimmed.highlightedZone.value_or(noZone), colorDimmed);
        else if (!dimmed.text.empty())
            DrawTextIn(dimmed.text, 0, top, lcd.width(), height, colorDimmed);
    }

    // Le bandeau rouge de 800 × 60 px à y 60 : « Connexion perdue » à gauche (x 10), « Tentative N/3 » puis trois points de 16 px espacés de 8 px
    // à droite (N pleins, les autres en contour), le tout en 38 px blanc.
    void DrawLostBanner(const ConnectionLostScreen &screen)
    {
        lcd.fillRect(0, topBarHeight, lcd.width(), lostBannerHeight, colorRed);
        lcd.setFont(fonts[1]);
        lcd.setTextSize(1);
        lcd.setTextColor(colorWhite);
        const int middle = topBarHeight + lostBannerHeight / 2;
        lcd.setTextDatum(middle_left);
        lcd.drawString(screen.title.c_str(), margin, middle);

        const int radius = attemptDotDiameter / 2;
        const int dotsWidth = screen.attemptCount * attemptDotDiameter + (screen.attemptCount - 1) * attemptDotGap;
        const int dotsLeft = lcd.width() - margin - dotsWidth;
        for (int i = 0; i < screen.attemptCount; ++i)
        {
            const int centerX = dotsLeft + i * (attemptDotDiameter + attemptDotGap) + radius;
            lcd.fillCircle(centerX, middle, radius, colorWhite);
            if (i >= screen.attempt)
                lcd.fillCircle(centerX, middle, radius - attemptDotOutline, colorRed);
        }
        lcd.setTextDatum(middle_right);
        lcd.drawString(screen.attemptLabel.c_str(), dotsLeft - attemptLabelGap, middle);
    }

    // Une zone par texte, en grille 2x2 : A en haut à gauche, B en haut à droite, C en bas à gauche, D en bas à droite
    // (`ZoneGrid`, le même découpage que les zones tactiles de `TappableZonesScreen`). Au-delà de quatre textes, les suivants ne sont pas dessinés.
    void ShowZones(const std::vector<std::string> &zoneTexts)
    {
        lcd.fillScreen(TFT_BLACK);
        DrawZones(zoneTexts, 0, lcd.height(), noZone);
    }

    // Texte dans un bandeau en haut, les dernières zones affichées dessous (hauteur réduite), la zone zoneIndex en blanc plein.
    void HighlightZone(const std::vector<std::string> &zoneTexts, size_t zoneIndex, const std::string &text)
    {
        const int bandHeight = lcd.height() / resultBandDivisor;
        lcd.fillScreen(TFT_BLACK);
        DrawTextIn(text, 0, 0, lcd.width(), bandHeight);
        DrawZones(zoneTexts, bandHeight, lcd.height() - bandHeight, zoneIndex);
    }

    // Les dernières zones affichées en plein écran, la zone zoneIndex (celle que le joueur a touchée) en blanc plein, sans texte ajouté.
    void HighlightChosenZone(const std::vector<std::string> &zoneTexts, size_t zoneIndex)
    {
        lcd.fillScreen(TFT_BLACK);
        DrawZones(zoneTexts, 0, lcd.height(), zoneIndex);
    }

    // Texte dans un bandeau en haut, les dernières zones affichées dessous, la zone choisie par le joueur toujours en évidence.
    void ShowBanner(const BannerScreen &screen)
    {
        HighlightZone(screen.zoneTexts, screen.chosenZone.value_or(noZone), screen.bannerText);
    }

    // Dessine les zones dans la bande (top, areaHeight), sans effacer l'écran, de la couleur `color` (blanc, ou gris pour la coupure).
    void DrawZones(const std::vector<std::string> &zoneTexts, int top, int areaHeight, size_t highlightedZone, uint32_t color = colorWhite)
    {
        const ZoneGrid grid(lcd.width(), top, areaHeight);
        for (size_t i = 0; i < zoneTexts.size() && i < zoneCount; ++i)
        {
            const ZoneRect zone = grid.At(i);
            const bool highlighted = (i == highlightedZone);
            const int frameX = zone.x + zoneBorderInset;
            const int frameY = zone.y + zoneBorderInset;
            const int frameWidth = zone.width - 2 * zoneBorderInset;
            const int frameHeight = zone.height - 2 * zoneBorderInset;
            if (highlighted)
                lcd.fillRect(frameX, frameY, frameWidth, frameHeight, color);
            else
                lcd.drawRect(frameX, frameY, frameWidth, frameHeight, color);
            DrawTextIn(zoneTexts[i], zone.x, zone.y, zone.width, zone.height, highlighted ? colorBlack : color);
        }
    }

    // Dessine le texte, centré dans le rectangle (x, y, width, height), sans effacer l'écran.
    void DrawTextIn(const std::string &text, int x, int y, int width, int height, uint32_t color = colorWhite)
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
