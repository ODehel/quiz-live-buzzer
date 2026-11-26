#ifndef SCREEN_MOCKS_H
#define SCREEN_MOCKS_H

#include <gmock/gmock.h>
#include "LastScreenReader.h"
#include "Screen.h"

class MockScreen : public Screen
{
public:
    MOCK_METHOD(void, Display, (const TopBar &topBar, const ScreenModel &screenModel), (override));
};

// Le dernier écran affiché, que le test pose à la main (en production : `RetainingScreen`).
class FakeLastScreen : public LastScreenReader
{
private:
    ScreenModel last = ScreenUnchanged{};

public:
    void Set(const ScreenModel &screenModel)
    {
        last = screenModel;
    }

    const ScreenModel &LastScreen() const override
    {
        return last;
    }
};

#endif
