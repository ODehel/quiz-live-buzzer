#ifndef BUTTON_PRESS_DETECTOR_H
#define BUTTON_PRESS_DETECTOR_H

#include "ButtonReader.h"
#include "ButtonPressTranslator.h"

class ButtonPressDetector
{
private:
    ButtonInput lastButton = ButtonInput::Unknown;
    ButtonReader &reader;
    ButtonPressTranslator &translator;

public:
    ButtonPressDetector(ButtonReader &reader, ButtonPressTranslator &translator) : reader(reader), translator(translator)
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