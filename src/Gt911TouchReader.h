#ifndef GT911_TOUCH_READER_H
#define GT911_TOUCH_READER_H

#include <TAMC_GT911.h>
#include "TouchPointReader.h"
#include "TouchPoint.h"
#include "TouchProjection.h"

class Gt911TouchReader : public TouchPointReader
{
private:
    enum class PinMode { Input, Output };
    enum class PinState { Low, High };
    TAMC_GT911 ts{19, 20, static_cast<uint8_t>(-1), static_cast<uint8_t>(-1), 800, 480};
    TouchProjection projection{{0, 0}, {800, 480}};
    void WriteReg(uint8_t reg, uint8_t value);
    uint8_t ReadReg(uint8_t reg);
    void SetMode(uint8_t pin, PinMode mode);
    void SetState(uint8_t pin, PinState state);
    void ResetTouchSequence();

public:
    TouchPoint Read() override;
    void Begin();
};

#endif