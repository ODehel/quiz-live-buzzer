#ifndef UTF8_H
#define UTF8_H

#include <string>

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
