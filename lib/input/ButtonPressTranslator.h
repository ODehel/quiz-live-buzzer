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
    ButtonFamily GetButtonFamilyFromPhysicalButton(PhysicalButton physicalButton)
    {
        switch (physicalButton)
        {
        case PhysicalButton::Buzz :
            return ButtonFamily::Buzz;
        
        case PhysicalButton::A :
        case PhysicalButton::C :
            return ButtonFamily::Mcq;
        }
    }

public:
    ButtonPressTranslator(BuzzerBehavior &buzzerBehavior) : buzzerBehavior(buzzerBehavior)
    {
    }

    void TranslateButtonPress(PhysicalButton physicalButton)
    {
        if (GetButtonFamilyFromPhysicalButton(physicalButton) == ButtonFamily::Buzz)
        {
            buzzerBehavior.OnBuzzPressed();
        }
        else
        {
            buzzerBehavior.OnMcqPressed(GetLetterFromPhysicalButton(physicalButton));
        }
    }
};

#endif