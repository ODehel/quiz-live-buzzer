#ifndef GFX_FONT_METRICS_H
#define GFX_FONT_METRICS_H

#include <vector>
#include "FontMetrics.h"

// Lit les dimensions d'une police au format GFXfont (`first`, `last`, `yAdvance`, `glyph[i].xAdvance`) :
// `lgfx::GFXfont` en production, une fausse police dans les tests. Le type est un paramètre de
// gabarit pour que `lib/` ne dépende pas de LovyanGFX.
//
// Hypothèses : les glyphes se suivent sans trou de `first` à `last` (pas de plage d'encodage de
// LovyanGFX) et la police contient le glyphe `?` (0x3F). Un code que la police ne dessine pas
// avance comme `?`, ce que `FontMetrics::Advance` fait déjà hors de 0x20-0xFF. C'est une
// surestimation voulue : LovyanGFX ne dessine rien pour ce code et avance de la largeur d'un
// espace, plus étroite ; mesurer `?` fait couper plus tôt, jamais déborder.
//
// `lineSpacingPercent` règle l'interligne : la hauteur de ligne est le `yAdvance` de la police
// multiplié par ce pourcentage, arrondi au pixel le plus proche. Le `yAdvance` natif d'Oswald
// (environ 1,48 em) est trop grand pour un texte de quiz : à 82 %, il tombe à environ 1,2 em.
template <class GfxFont>
FontMetrics ReadFontMetrics(const GfxFont &font, int lineSpacingPercent)
{
    const int questionMarkAdvance = font.glyph['?' - font.first].xAdvance;
    std::vector<int> advances;
    for (int code = FontMetrics::FirstCode; code <= FontMetrics::LastCode; ++code)
    {
        if (code < font.first || code > font.last)
            advances.push_back(questionMarkAdvance);
        else
            advances.push_back(font.glyph[code - font.first].xAdvance);
    }
    const int lineHeight = (font.yAdvance * lineSpacingPercent + 50) / 100;
    return FontMetrics(lineHeight, advances);
}

#endif
