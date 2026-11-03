#ifndef TEXT_MEASURER_H
#define TEXT_MEASURER_H

#include <string>

// Mesure du texte pour la mise en page : plusieurs tailles de police candidates, de la plus
// grande (0) à la plus petite (SizeCount() - 1).
class TextMeasurer
{
public:
    virtual ~TextMeasurer() = default;
    virtual int SizeCount() const = 0;
    virtual int Width(const std::string &text, int size) const = 0;
    virtual int LineHeight(int size) const = 0;
};

#endif
