#ifndef ARMED_MODE_READER_H
#define ARMED_MODE_READER_H

class ArmedModeReader
{
public:
    virtual ~ArmedModeReader() = default;
    virtual bool IsArmed() const = 0;
};

#endif
