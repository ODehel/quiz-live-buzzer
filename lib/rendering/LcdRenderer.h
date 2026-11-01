#ifndef LCD_RENDERER_H
#define LCD_RENDERER_H

#include "DisplayState.h"
#include "MessageRenderer.h"
#include "Screen.h"

class LcdRenderer : public MessageRenderer
{
private:
    Screen &screen;
    DisplayState displayState;

public:
    LcdRenderer(Screen &screen) : screen(screen)
    {
    }

    void Apply(const std::string& type) override 
    {
        std::string text = displayState.LcdTextFor(type);
        if (!text.empty())
            screen.Show(text);
    }
};

#endif