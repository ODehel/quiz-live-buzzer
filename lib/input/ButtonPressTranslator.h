#ifndef BUTTON_PRESS_TRANSLATOR_H
#define BUTTON_PRESS_TRANSLATOR_H

#include "PhysicalButton.h"

class ButtonPressTranslator
{
public :
    virtual ~ButtonPressTranslator() = default;
    virtual void TranslateButtonPress(PhysicalButton physicalButton) = 0;
};

#endif