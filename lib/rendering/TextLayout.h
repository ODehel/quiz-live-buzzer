#ifndef TEXT_LAYOUT_H
#define TEXT_LAYOUT_H

#include <string>
#include <vector>
#include "TextMeasurer.h"
#include "Utf8.h"

struct TextLines
{
    int size;
    std::vector<std::string> lines;
};

// Place un texte sur une zone d'écran : choisit la plus grande taille de police pour laquelle
// le texte, coupé aux espaces, tient en hauteur, puis rend les lignes à dessiner.
// Les largeurs et hauteurs sont celles du TextMeasurer, en pixels.
// Au-delà de ce que la plus petite taille peut contenir, le texte garde cette taille et déborde.
class TextLayout
{
private:
    const TextMeasurer &measurer;
    int areaWidth;
    int areaHeight;

public:
    TextLayout(const TextMeasurer &measurer, int areaWidth, int areaHeight)
        : measurer(measurer), areaWidth(areaWidth), areaHeight(areaHeight)
    {
    }

    TextLines Compute(const std::string &text) const
    {
        const int smallest = measurer.SizeCount() - 1;
        for (int size = 0; size < smallest; ++size)
        {
            std::vector<std::string> lines = Wrap(text, size);
            if (static_cast<int>(lines.size()) * measurer.LineHeight(size) <= areaHeight)
                return TextLines{size, lines};
        }
        return TextLines{smallest, Wrap(text, smallest)};
    }

    // Coupe aux espaces, ligne par ligne, sans dépasser la largeur de la zone ; un mot plus large
    // qu'une ligne est coupé (au moins un caractère par ligne). Un caractère UTF-8 n'est jamais coupé en deux.
    std::vector<std::string> Wrap(const std::string &text, int size) const
    {
        std::vector<std::string> lines;
        std::string line;
        size_t position = 0;
        while (position < text.size())
        {
            if (text[position] == ' ')
            {
                ++position;
                continue;
            }
            size_t end = text.find(' ', position);
            if (end == std::string::npos)
                end = text.size();
            std::string word = text.substr(position, end - position);
            position = end;

            if (!Fits(word, size))
            {
                if (!line.empty())
                    lines.push_back(line);
                while (!Fits(word, size))
                {
                    const size_t cut = LongestPrefixThatFits(word, size);
                    lines.push_back(word.substr(0, cut));
                    word = word.substr(cut);
                }
                line = word;
            }
            else if (!line.empty() && !Fits(line + " " + word, size))
            {
                lines.push_back(line);
                line = word;
            }
            else
            {
                line = line.empty() ? word : line + " " + word;
            }
        }
        if (!line.empty())
            lines.push_back(line);
        return lines;
    }

private:
    bool Fits(const std::string &text, int size) const
    {
        return measurer.Width(text, size) <= areaWidth;
    }

    size_t LongestPrefixThatFits(const std::string &word, int size) const
    {
        const size_t count = Utf8::CharacterCount(word);
        size_t best = 0;
        for (size_t characters = 1; characters <= count; ++characters)
        {
            const size_t offset = Utf8::ByteOffsetOfCharacter(word, characters);
            if (!Fits(word.substr(0, offset), size))
                break;
            best = offset;
        }
        return best == 0 ? Utf8::ByteOffsetOfCharacter(word, 1) : best;
    }
};

#endif
