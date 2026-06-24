#ifndef BUTTON_READER_H
#define BUTTON_READER_H

#include "ButtonInput.h"

class ButtonReader
{
public:
    virtual ~ButtonReader() = default;
    virtual ButtonInput DeduceButton() = 0;
};

#endif