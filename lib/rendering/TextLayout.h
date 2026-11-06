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
    // Un signe de ponctuation haute (« ? ! : ; » ) ») ne commence pas une ligne : il emmène avec lui le mot qui
    // le précède, sauf si ce mot est déjà seul sur sa ligne ou si le mot et le signe ne tiendraient pas ensemble.
    // Symétriquement, un guillemet ouvrant ne finit pas une ligne : il emmène avec lui le mot qui le suit.
    std::vector<std::string> Wrap(const std::string &text, int size) const
    {
        std::vector<std::string> lines;
        std::string line;
        size_t anchor = 0; // début, dans `line`, du dernier mot qui n'est pas un signe (0 : aucun mot avant lui)
        size_t position = 0;
        while (position < text.size())
        {
            if (text[position] == ' ')
            {
                ++position;
                continue;
            }
            const size_t end = EndOfWord(text, position);
            std::string word = text.substr(position, end - position);
            position = end;

            if (IsOpeningGuillemet(word))
                JoinWithTheNextWord(text, size, word, position);

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
                anchor = 0;
            }
            else if (SignShouldTakeItsWordToTheNextLine(line, anchor, word, size))
            {
                lines.push_back(line.substr(0, anchor - 1));
                line = line.substr(anchor) + " " + word;
                anchor = 0;
            }
            else if (!line.empty() && !Fits(line + " " + word, size))
            {
                lines.push_back(line);
                line = word;
                anchor = 0;
            }
            else
            {
                if (!line.empty())
                    line += " ";
                if (!IsClosingPunctuation(word))
                    anchor = line.size();
                line += word;
            }
        }
        if (!line.empty())
            lines.push_back(line);
        return lines;
    }

private:
    static size_t EndOfWord(const std::string &text, size_t from)
    {
        const size_t space = text.find(' ', from);
        return space == std::string::npos ? text.size() : space;
    }

    static bool IsOpeningGuillemet(const std::string &word)
    {
        return word == "«";
    }

    // Le guillemet ouvrant, que le français sépare du mot suivant par une espace, ne finit pas une ligne : il forme
    // un seul mot avec le mot qu'il ouvre, sauf s'il n'y a pas de mot après lui ou si les deux ne tiendraient pas
    // ensemble sur une ligne (il garde alors la place d'un mot ordinaire).
    void JoinWithTheNextWord(const std::string &text, int size, std::string &word, size_t &position) const
    {
        size_t start = position;
        while (start < text.size() && text[start] == ' ')
            ++start;
        if (start == text.size())
            return;
        const size_t end = EndOfWord(text, start);
        const std::string joined = word + " " + text.substr(start, end - start);
        if (!Fits(joined, size))
            return;
        word = joined;
        position = end;
    }

    // Le signe ne tient plus sur la ligne, mais il tiendrait sur la suivante avec le mot (et les signes) qui le précèdent.
    // Quand ce mot est seul sur la ligne (`anchor` vaut 0), les deux mesures sont identiques : la condition est fausse.
    bool SignShouldTakeItsWordToTheNextLine(const std::string &line, size_t anchor, const std::string &word, int size) const
    {
        return IsClosingPunctuation(word) && !Fits(line + " " + word, size) &&
               Fits(line.substr(anchor) + " " + word, size);
    }

    // Un mot composé uniquement de signes que la typographie française sépare du mot précédent par une
    // espace (« Quoi ? », « oui », « Quoi ?! ») : il reste sur la ligne de ce mot.
    static bool IsClosingPunctuation(const std::string &word)
    {
        static const char *const signs[] = {"?", "!", ":", ";", "»", ")"};
        size_t position = 0;
        while (position < word.size())
        {
            bool found = false;
            for (const char *sign : signs)
            {
                const size_t length = std::char_traits<char>::length(sign);
                if (word.compare(position, length, sign) == 0)
                {
                    position += length;
                    found = true;
                    break;
                }
            }
            if (!found)
                return false;
        }
        return !word.empty();
    }

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
