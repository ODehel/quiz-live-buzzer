#ifndef CONNECTION_MOCKS_H
#define CONNECTION_MOCKS_H

#include <gmock/gmock.h>
#include <string>
#include "PayloadSocketWriter.h"
#include "ConnectionCloser.h"

class MockPayloadSocketWriter : public PayloadSocketWriter
{
public:
    MOCK_METHOD(void, Write, (const std::string&), (override));
};

class MockConnectionCloser : public ConnectionCloser
{
public:
    MOCK_METHOD(void, Close, (), (override));
};

#endif