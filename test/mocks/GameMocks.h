#ifndef GAME_MOCKS_H
#define GAME_MOCKS_H

#include <gmock/gmock.h>
#include "HubMessageSender.h"
#include "LocalFeedback.h"

class MockHubMessageSender : public HubMessageSender
{
public:
    MOCK_METHOD(void, SendAnswer, (char), (override));
    MOCK_METHOD(void, SendBuzz, (), (override));
};

class MockLocalFeedback : public LocalFeedback
{
public:
    MOCK_METHOD(void, Acknowledge, (), (override));
};

#endif