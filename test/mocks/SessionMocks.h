#ifndef SESSION_MOCKS_H
#define SESSION_MOCKS_H

#include <gmock/gmock.h>
#include "SessionMessageSender.h"
#include "LoginRefusalListener.h"

class MockSessionMessageSender : public SessionMessageSender
{
public:
    MOCK_METHOD(void, SendAuthRefresh, (), (override));
    MOCK_METHOD(void, SendAuth, (), (override));
    MOCK_METHOD(bool, PollLogin, (), (override));
    MOCK_METHOD(void, OnConnectionClosed, (), (override));
    MOCK_METHOD(void, Abandon, (), (override));
};

class MockLoginRefusalListener : public LoginRefusalListener
{
public:
    MOCK_METHOD(void, OnLoginRefused, (), (override));
};
#endif
