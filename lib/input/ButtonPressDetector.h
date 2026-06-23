#ifndef BUTTON_PRESS_DETECTOR_H
#define BUTTON_PRESS_DETECTOR_H

#include "PhysicalButtonReader.h"
#include "ButtonPressTranslator.h"

class ButtonPressDetector
{
private:
    ButtonInput lastButton = ButtonInput::Unknown;
    PhysicalButtonReader &reader;
    ButtonPressTranslator &translator;

public:
    ButtonPressDetector(PhysicalButtonReader &reader, ButtonPressTranslator &translator) : reader(reader), translator(translator)
    {
    }

    void Poll()
    {
        ButtonInput currentButton = reader.DeduceButton();

        if (currentButton != lastButton && currentButton != ButtonInput::Unknown)
            translator.TranslateButtonPress(currentButton);

        lastButton = currentButton;
    }
};

#endif