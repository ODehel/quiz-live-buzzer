#ifndef TOP_BAR_NAME_H
#define TOP_BAR_NAME_H

#include <string>
#include "TextMeasurer.h"
#include "Utf8.h"

// Le nom du joueur dans le bandeau du haut, ramené à sa place (US-FW-009 CA-5, décision #83).
// Le nom vit dans la colonne de gauche, de largeur (dalle − titre) ÷ 2 moins la marge : il ne touche jamais « Buzz’O’Live », centré.
// Les largeurs sont celles du `TextMeasurer`, en pixels, à la taille de police du bandeau.
namespace TopBarName
{
    // Le titre au centre du bandeau et la marge à gauche du nom : `GfxScreen` les dessine, `Fit` en mesure la place.
    inline constexpr const char *Title = "Buzz’O’Live";
    inline constexpr const char *Ellipsis = "…";
    inline constexpr int Margin = 10;

    // Largeur de la colonne de gauche : la dalle moins le titre, partagée en deux (division entière), moins la marge.
    inline int ColumnWidth(const TextMeasurer &measurer, int size, int panelWidth)
    {
        return (panelWidth - measurer.Width(Title, size)) / 2 - Margin;
    }

    // Un nom qui tient dans la colonne est rendu entier. Sinon : le plus long début du nom qui tient avec « … », tel quel
    // (l'espace qui précède la coupure est gardée : « Jean-Baptiste … »). Un caractère UTF-8 n'est jamais coupé en deux.
    inline std::string Fit(const TextMeasurer &measurer, int size, const std::string &name, int panelWidth)
    {
        const int column = ColumnWidth(measurer, size, panelWidth);
        if (measurer.Width(name, size) <= column)
            return name;
        const size_t count = Utf8::CharacterCount(name);
        size_t best = 0;
        for (size_t characters = 1; characters <= count; ++characters)
        {
            const size_t offset = Utf8::ByteOffsetOfCharacter(name, characters);
            if (measurer.Width(name.substr(0, offset) + Ellipsis, size) > column)
                break;
            best = offset;
        }
        return name.substr(0, best) + Ellipsis;
    }
}

#endif
