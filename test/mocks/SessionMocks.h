#ifndef SESSION_MOCKS_H
#define SESSION_MOCKS_H

#include <gmock/gmock.h>
#include "SessionMessageSender.h"
#include "TokenReceiver.h"

class MockSessionMessageSender : public SessionMessageSender
{
public:
    MOCK_METHOD(void, SendAuthRefresh, (), (override));
    MOCK_METHOD(void, SendAuth, (), (override));
};

class MockTokenReceiver : public TokenReceiver
{
public:
    MOCK_METHOD(void, UpdateToken, (const std::string&), (override));
};
#endif