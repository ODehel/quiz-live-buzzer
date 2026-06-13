#ifndef BUZZER_BUTTON_PRESS_TRANSLATOR_H
#define BUZZER_BUTTON_PRESS_TRANSLATOR_H

#include "BuzzerBehavior.h"
#include "PhysicalButton.h"
#include "ButtonPressTranslator.h"

class BuzzerButtonPressTranslator : public ButtonPressTranslator
{
private:
    BuzzerBehavior &buzzerBehavior;

public:
    BuzzerButtonPressTranslator(BuzzerBehavior &buzzerBehavior) : buzzerBehavior(buzzerBehavior)
    {
    }

    void TranslateButtonPress(PhysicalButton physicalButton) override
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