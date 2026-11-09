#ifndef LCD_RENDERER_H
#define LCD_RENDERER_H

#include <variant>
#include "ArmedModeReader.h"
#include "MessageRenderer.h"
#include "Screen.h"
#include "ScreenPresenter.h"

class LcdRenderer : public MessageRenderer
{
private:
    Screen &screen;
    const ArmedModeReader &armedModeReader;
    ScreenPresenter presenter;

public:
    LcdRenderer(Screen &screen, const ArmedModeReader &armedModeReader) : screen(screen), armedModeReader(armedModeReader)
    {
    }

    void Apply(const HubMessage &message) override
    {
        const ScreenModel screenModel = presenter.ScreenFor(message, armedModeReader.IsArmed());
        if (!std::holds_alternative<ScreenUnchanged>(screenModel))
            screen.Display(screenModel);
    }
};

#endif
