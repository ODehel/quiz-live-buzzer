#ifndef BUTTON_PRESS_DETECTOR_H
#define BUTTON_PRESS_DETECTOR_H

#include "PhysicalButtonReader.h"
#include "ButtonPressTranslator.h"

class ButtonPressDetector
{
private:
    PhysicalButton lastButton = PhysicalButton::Unknown;
    PhysicalButtonReader &reader;
    ButtonPressTranslator &translator;

public:
    ButtonPressDetector(PhysicalButtonReader &reader, ButtonPressTranslator &translator) : reader(reader), translator(translator)
    {
    }

    void Poll()
    {
        PhysicalButton currentButton = reader.DeduceButton();

        if (currentButton != lastButton)
            translator.TranslateButtonPress(currentButton);

        lastButton = currentButton;
    }
};

#endif