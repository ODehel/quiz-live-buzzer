#ifndef MOCK_GPIO_PIN_READER_H
#define MOCK_GPIO_PIN_READER_H

#include <gmock/gmock.h>
#include "GpioPinReader.h"

class MockGpioPinReader : public GpioPinReader
{
public:
    MOCK_METHOD(bool, Read, (), (override));
};

#endif