#ifndef LAST_SCREEN_READER_H
#define LAST_SCREEN_READER_H

#include "ScreenModel.h"

// Le dernier écran effectivement affiché, quel qu'en soit le producteur, tel qu'il a été transmis à l'écran (propositions et zone choisie comprises).
// `ScreenUnchanged` tant qu'aucun écran n'a été affiché.
class LastScreenReader
{
public:
    virtual ~LastScreenReader() = default;
    virtual const ScreenModel &LastScreen() const = 0;
};

#endif
