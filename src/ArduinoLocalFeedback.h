#ifndef ARDUINO_LOCAL_FEEDBACK_H
#define ARDUINO_LOCAL_FEEDBACK_H

#include <array>
#include "LocalFeedback.h"

class ArduinoLocalFeedback : public LocalFeedback
{
private:
    const std::array<int, 4> pins;

public:
    ArduinoLocalFeedback(std::array<int, 4> pins) : pins(pins)
    {
    }

    void Begin();
    void Acknowledge() override;
};

#endif