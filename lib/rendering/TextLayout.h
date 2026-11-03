#ifndef TEXT_LAYOUT_H
#define TEXT_LAYOUT_H

#include <string>
#include <vector>

struct TextLines
{
    int scale;
    std::vector<std::string> lines;
};

// Place un texte sur une zone d'écran : choisit la plus grande échelle de police pour laquelle
// le texte, coupé aux espaces, tient en hauteur, puis rend les lignes à dessiner.
// Un titre de question (250 caractères au plus) tient à l'échelle MinScale quels que soient ses mots.
// Au-delà de ce que MinScale peut contenir, le texte garde MinScale et déborde de la zone.
class TextLayout
{
public:
    enum : int
    {
        MaxScale = 5,
        MinScale = 3
    };

private:
    int areaWidth;
    int areaHeight;
    int cellWidth;
    int cellHeight;

public:
    TextLayout(int areaWidth, int areaHeight, int cellWidth, int cellHeight)
        : areaWidth(areaWidth), areaHeight(areaHeight), cellWidth(cellWidth), cellHeight(cellHeight)
    {
    }

    TextLines Compute(const std::string &text) const
    {
        for (int scale = MaxScale; scale > MinScale; --scale)
        {
            std::vector<std::string> lines = Wrap(text, ColumnsAt(scale));
            if (static_cast<int>(lines.size()) <= RowsAt(scale))
                return TextLines{scale, lines};
        }
        return TextLines{MinScale, Wrap(text, ColumnsAt(MinScale))};
    }

    // Coupe aux espaces, ligne par ligne, sans dépasser `columns` caractères ; un mot plus long
    // qu'une ligne est coupé. Un caractère UTF-8 compte pour une colonne et n'est jamais coupé.
    static std::vector<std::string> Wrap(const std::string &text, int columns)
    {
        const size_t width = columns < 1 ? 1 : static_cast<size_t>(columns);
        std::vector<std::string> lines;
        std::string line;
        size_t lineLength = 0;
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

            if (CharacterCount(word) > width)
            {
                if (lineLength > 0)
                    lines.push_back(line);
                while (CharacterCount(word) > width)
                {
                    const size_t cut = ByteOffsetOfCharacter(word, width);
                    lines.push_back(word.substr(0, cut));
                    word = word.substr(cut);
                }
                line = word;
                lineLength = CharacterCount(word);
            }
            else if (lineLength > 0 && lineLength + 1 + CharacterCount(word) > width)
            {
                lines.push_back(line);
                line = word;
                lineLength = CharacterCount(word);
            }
            else
            {
                if (lineLength > 0)
                    line += ' ';
                line += word;
                lineLength = CharacterCount(line);
            }
        }
        if (lineLength > 0)
            lines.push_back(line);
        return lines;
    }

private:
    int ColumnsAt(int scale) const
    {
        return areaWidth / (cellWidth * scale);
    }

    int RowsAt(int scale) const
    {
        return areaHeight / (cellHeight * scale);
    }

    static bool StartsCharacter(char byte)
    {
        return (static_cast<unsigned char>(byte) & 0xC0) != 0x80;
    }

    static size_t CharacterCount(const std::string &text)
    {
        size_t count = 0;
        for (size_t i = 0; i < text.size(); ++i)
            if (StartsCharacter(text[i]))
                ++count;
        return count;
    }

    static size_t ByteOffsetOfCharacter(const std::string &text, size_t characterIndex)
    {
        size_t seen = 0;
        for (size_t i = 0; i < text.size(); ++i)
        {
            if (StartsCharacter(text[i]))
            {
                if (seen == characterIndex)
                    return i;
                ++seen;
            }
        }
        return text.size();
    }
};

#endif
