#ifndef BUTTON_PRESS_TRANSLATOR_H
#define BUTTON_PRESS_TRANSLATOR_H

#include "ButtonInput.h"

class ButtonPressTranslator
{
public :
    virtual ~ButtonPressTranslator() = default;
    virtual void TranslateButtonPress(ButtonInput physicalButton) = 0;
};

#endif