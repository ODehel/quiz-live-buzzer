#ifndef LCD_RENDERER_H
#define LCD_RENDERER_H

#include "ArmedModeReader.h"
#include "DisplayState.h"
#include "MessageRenderer.h"
#include "Screen.h"

class LcdRenderer : public MessageRenderer
{
private:
    Screen &screen;
    const ArmedModeReader &armedModeReader;
    DisplayState displayState;

public:
    LcdRenderer(Screen &screen, const ArmedModeReader &armedModeReader) : screen(screen), armedModeReader(armedModeReader)
    {
    }

    void Apply(const HubMessage &message) override
    {
        if (message.Type() == "timer_end")
        {
            if (armedModeReader.IsArmed())
                screen.Show(displayState.LcdTextForTimeUp());
            return;
        }
        const std::vector<std::string> zones = displayState.ZoneTextsFor(message);
        if (!zones.empty())
            screen.ShowZones(zones);
        std::string text = displayState.LcdTextFor(message);
        if (!text.empty())
            screen.Show(text);
    }
};

#endif