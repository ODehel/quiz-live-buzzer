#ifndef GAME_MOCKS_H
#define GAME_MOCKS_H

#include <gmock/gmock.h>
#include "HubMessageSender.h"
#include "LocalFeedback.h"
#include "ArmedModeReader.h"

class MockHubMessageSender : public HubMessageSender
{
public:
    // Par défaut l'envoi réussit (gmock rendrait faux) : un test d'échec le dit avec `ON_CALL` ou `WillOnce`.
    MockHubMessageSender()
    {
        ON_CALL(*this, SendAnswer(::testing::_)).WillByDefault(::testing::Return(true));
        ON_CALL(*this, SendBuzz()).WillByDefault(::testing::Return(true));
    }

    MOCK_METHOD(bool, SendAnswer, (char), (override));
    MOCK_METHOD(bool, SendBuzz, (), (override));
};

class MockLocalFeedback : public LocalFeedback
{
public:
    MOCK_METHOD(void, AcknowledgeAnswer, (char), (override));
    MOCK_METHOD(void, AcknowledgeBuzz, (), (override));
    MOCK_METHOD(void, RestoreScreenBeforePress, (), (override));
};

class MockArmedModeReader : public ArmedModeReader
{
public:
    MOCK_METHOD(bool, IsArmed, (), (const, override));
};

#endif