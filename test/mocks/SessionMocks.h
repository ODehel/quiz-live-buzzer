#ifndef SESSION_MOCKS_H
#define SESSION_MOCKS_H

#include <gmock/gmock.h>
#include "SessionMessageSender.h"

class MockSessionMessageSender : public SessionMessageSender
{
public:
    MOCK_METHOD(void, SendAuthRefresh, (), (override));
    MOCK_METHOD(void, SendAuth, (), (override));
};
#endif