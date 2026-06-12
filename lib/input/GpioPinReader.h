#ifndef GPIO_PIN_READER_H
#define GPIO_PIN_READER_H

class GpioPinReader
{
public:
    virtual ~GpioPinReader() = default;
    virtual bool Read() = 0;
};

#endif