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
        const std::string &type = message.Type();
        if (type == "timer_end")
        {
            if (armedModeReader.IsArmed())
                screen.Show(displayState.LcdTextForTimeUp());
            return;
        }
        if (type == "timer_tick")
        {
            if (message.HasRemainingSeconds())
                screen.Show(displayState.LcdTextForRemainingSeconds(message.RemainingSeconds()));
            return;
        }
        std::string text = displayState.LcdTextFor(type);
        if (!text.empty())
            screen.Show(text);
    }
};

#endif