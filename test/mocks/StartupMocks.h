#ifndef STARTUP_MOCKS_H
#define STARTUP_MOCKS_H

#include <gmock/gmock.h>
#include "TokenFetcher.h"
#include "Sleeper.h"

class MockTokenFetcher : public TokenFetcher
{
public:
    MOCK_METHOD(LoginResult, Fetch, (), (override));
};

class MockSleeper : public Sleeper
{
public:
    MOCK_METHOD(void, Sleep, (unsigned long), (override));
};
#endif
