#ifndef SCREEN_MOCKS_H
#define SCREEN_MOCKS_H

#include <gmock/gmock.h>
#include "Screen.h"

class MockScreen : public Screen
{
public:
    MOCK_METHOD(void, Display, (const ScreenModel &screenModel), (override));
};

#endif
