#ifndef BUZZER_BUTTON_PRESS_TRANSLATOR_H
#define BUZZER_BUTTON_PRESS_TRANSLATOR_H

#include "BuzzerBehavior.h"
#include "ButtonInput.h"
#include "ButtonPressTranslator.h"

class BuzzerButtonPressTranslator : public ButtonPressTranslator
{
private:
    BuzzerBehavior &buzzerBehavior;

public:
    BuzzerButtonPressTranslator(BuzzerBehavior &buzzerBehavior) : buzzerBehavior(buzzerBehavior)
    {
    }

    void TranslateButtonPress(ButtonInput physicalButton) override
    {
        switch (physicalButton)
        {
        case ButtonInput::A:
            buzzerBehavior.OnMcqPressed('A');
            break;
        case ButtonInput::B:
            buzzerBehavior.OnMcqPressed('B');
            break;
        case ButtonInput::C:
            buzzerBehavior.OnMcqPressed('C');
            break;
        case ButtonInput::D:
            buzzerBehavior.OnMcqPressed('D');
            break;
        case ButtonInput::Buzz:
            buzzerBehavior.OnBuzzPressed();
            break;
        case ButtonInput::Unknown:
            break;
        }
    }
};

#endif