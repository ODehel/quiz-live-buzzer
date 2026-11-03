#ifndef UTF8_H
#define UTF8_H

#include <cstdint>
#include <string>
#include <vector>

namespace Utf8
{
    inline bool StartsCharacter(char byte)
    {
        return (static_cast<unsigned char>(byte) & 0xC0) != 0x80;
    }

    inline size_t CharacterCount(const std::string &text)
    {
        size_t count = 0;
        for (size_t i = 0; i < text.size(); ++i)
            if (StartsCharacter(text[i]))
                ++count;
        return count;
    }

    // Nombre d'octets de continuation annoncés par l'octet de tête (0 pour l'ASCII ou un octet invalide).
    inline size_t ContinuationCount(unsigned char lead)
    {
        if ((lead & 0xE0) == 0xC0)
            return 1;
        if ((lead & 0xF0) == 0xE0)
            return 2;
        if ((lead & 0xF8) == 0xF0)
            return 3;
        return 0;
    }

    // Points de code du texte. Un octet invalide ou une séquence tronquée est rendu tel quel, octet par octet.
    inline std::vector<uint32_t> CodePoints(const std::string &text)
    {
        std::vector<uint32_t> codes;
        for (size_t i = 0; i < text.size(); ++i)
        {
            const unsigned char lead = static_cast<unsigned char>(text[i]);
            const size_t continuation = ContinuationCount(lead);
            if (continuation == 0 || i + continuation >= text.size())
            {
                codes.push_back(lead);
                continue;
            }
            uint32_t code = lead & (0x3Fu >> continuation);
            for (size_t k = 1; k <= continuation; ++k)
                code = (code << 6) | (static_cast<unsigned char>(text[i + k]) & 0x3Fu);
            codes.push_back(code);
            i += continuation;
        }
        return codes;
    }

    inline size_t ByteOffsetOfCharacter(const std::string &text, size_t characterIndex)
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
}

#endif
