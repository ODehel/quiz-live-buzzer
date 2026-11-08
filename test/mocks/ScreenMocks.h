#ifndef SCREEN_MOCKS_H
#define SCREEN_MOCKS_H

#include <gmock/gmock.h>
#include "Screen.h"

class MockScreen : public Screen
{
public:
    MOCK_METHOD(void, Show, (const std::string &text), (override));
    MOCK_METHOD(void, ShowZones, (const std::vector<std::string> &zoneTexts), (override));
    MOCK_METHOD(void, HighlightZone, (size_t zoneIndex, const std::string &text), (override));
    MOCK_METHOD(void, HighlightChosenZone, (size_t zoneIndex), (override));
    MOCK_METHOD(void, ShowBanner, (const std::string &text), (override));
};

#endif
