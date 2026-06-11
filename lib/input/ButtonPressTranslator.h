#ifndef BUTTON_PRESS_TRANSLATOR_H
#define BUTTON_PRESS_TRANSLATOR_H

#include "BuzzerBehavior.h"

enum class PhysicalButton
{
    A,
    C,
    Buzz
};

class ButtonPressTranslator
{
private:
    BuzzerBehavior &buzzerBehavior;
    char GetLetterFromPhysicalButton(PhysicalButton physicalButton)
    {
        switch (physicalButton)
        {
        case PhysicalButton::A:
            return 'A';
        case PhysicalButton::C:
            return 'C';
        }
    }
    
    bool IsBuzz(PhysicalButton physicalButton)
    {
        return physicalButton == PhysicalButton::Buzz;
    }

public:
    ButtonPressTranslator(BuzzerBehavior &buzzerBehavior) : buzzerBehavior(buzzerBehavior)
    {
    }

    void TranslateButtonPress(PhysicalButton physicalButton)
    {
        IsBuzz(physicalButton)
            ? buzzerBehavior.OnBuzzPressed()
            : buzzerBehavior.OnMcqPressed(GetLetterFromPhysicalButton(physicalButton));
    }
};

#endif