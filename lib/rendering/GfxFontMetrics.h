#ifndef GFX_FONT_METRICS_H
#define GFX_FONT_METRICS_H

#include <cstdint>
#include <utility>
#include <vector>
#include "FontMetrics.h"

// Interligne des textes de quiz, en pourcentage du `yAdvance` d'Oswald (64, 50 et 40 px au lieu de 71, 56 et 44).
// 82 % faisait mordre le descendant d'une ligne sur l'accent de la suivante (smoke test du 2026-11-03).
static const int QuizLineSpacingPercent = 90;

// Avance du glyphe que la police dessine pour `code`, ou `notFound` si elle n'en dessine pas.
// Même recherche que `lgfx::GFXfont::getGlyph` : sans plage (`range_num` à 0), le glyphe d'indice i
// dessine le code `first` + i ; avec des plages, le code `start` + k dessine le glyphe d'indice `base` + k.
template <class GfxFont>
int FindAdvance(const GfxFont &font, uint32_t code, int notFound)
{
    if (code < font.first || code > font.last)
        return notFound;
    if (font.range_num == 0)
        return font.glyph[code - font.first].xAdvance;
    for (int i = 0; i < font.range_num; ++i)
    {
        if (code >= font.range[i].start && code <= font.range[i].end)
            return font.glyph[font.range[i].base + (code - font.range[i].start)].xAdvance;
    }
    return notFound;
}

// Lit les dimensions d'une police au format GFXfont (`first`, `last`, `yAdvance`, `glyph[i].xAdvance`,
// `range_num`, `range`) : `lgfx::GFXfont` en production, une fausse police dans les tests. Le type est
// un paramètre de gabarit pour que `lib/` ne dépende pas de LovyanGFX.
//
// Hypothèses : la police contient le glyphe `?` (0x3F) et les glyphes de Latin-1 (0x20 à 0xFF) ; ceux
// qu'elle dessine au-delà de 0xFF (œ, ’, … : les codes de ses plages) gardent leur avance. Un code que
// la police ne dessine pas avance comme `?`, ce que `FontMetrics::Advance` fait déjà. C'est une
// surestimation voulue : LovyanGFX ne dessine rien pour ce code et avance de la largeur d'un
// espace, plus étroite ; mesurer `?` fait couper plus tôt, jamais déborder.
//
// `lineSpacingPercent` règle l'interligne : la hauteur de ligne est le `yAdvance` de la police
// multiplié par ce pourcentage, arrondi au pixel le plus proche. Le `yAdvance` natif d'Oswald
// (environ 1,48 em) est trop grand pour un texte de quiz ; l'interligne retenu est `QuizLineSpacingPercent`.
template <class GfxFont>
FontMetrics ReadFontMetrics(const GfxFont &font, int lineSpacingPercent)
{
    const int notFound = -1;
    const int questionMarkAdvance = FindAdvance(font, '?', notFound);
    std::vector<int> advances;
    for (int code = FontMetrics::FirstCode; code <= FontMetrics::LastCode; ++code)
    {
        const int advance = FindAdvance(font, code, notFound);
        advances.push_back(advance == notFound ? questionMarkAdvance : advance);
    }
    std::vector<std::pair<uint32_t, int>> extraAdvances;
    for (int i = 0; i < font.range_num; ++i)
    {
        for (uint32_t code = font.range[i].start; code <= font.range[i].end; ++code)
        {
            const int advance = FindAdvance(font, code, notFound);
            if (code > FontMetrics::LastCode && advance != notFound)
                extraAdvances.emplace_back(code, advance);
        }
    }
    const int lineHeight = (font.yAdvance * lineSpacingPercent + 50) / 100;
    return FontMetrics(lineHeight, advances, extraAdvances);
}

#endif
