#ifndef BUTTON_PRESS_TRANSLATOR_H
#define BUTTON_PRESS_TRANSLATOR_H

#include "BuzzerBehavior.h"

enum class PhysicalButton
{
    A,
    B,
    C,
    D,
    Buzz,
    Unknown
};

class ButtonPressTranslator
{
private:
    BuzzerBehavior &buzzerBehavior;

public:
    ButtonPressTranslator(BuzzerBehavior &buzzerBehavior) : buzzerBehavior(buzzerBehavior)
    {
    }

    void TranslateButtonPress(PhysicalButton physicalButton)
    {
        switch (physicalButton)
        {
        case PhysicalButton::A:
            buzzerBehavior.OnMcqPressed('A');
            break;
        case PhysicalButton::B:
            buzzerBehavior.OnMcqPressed('B');
            break;
        case PhysicalButton::C:
            buzzerBehavior.OnMcqPressed('C');
            break;
        case PhysicalButton::D:
            buzzerBehavior.OnMcqPressed('D');
            break;
        case PhysicalButton::Buzz:
            buzzerBehavior.OnBuzzPressed();
            break;
        case PhysicalButton::Unknown:
            break;
        }
    }
};

#endif