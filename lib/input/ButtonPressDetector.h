#ifndef BUTTON_PRESS_DETECTOR_H
#define BUTTON_PRESS_DETECTOR_H

#include "PhysicalButtonReader.h"
#include "ButtonPressTranslator.h"

class ButtonPressDetector
{
private:
    PhysicalButtonReader &reader;
    ButtonPressTranslator &translator;

public:
    ButtonPressDetector(PhysicalButtonReader &reader, ButtonPressTranslator &translator) : reader(reader), translator(translator)
    {
    }

    void Poll()
    {
        translator.TranslateButtonPress(reader.DeduceButton());
    }
};

#endif