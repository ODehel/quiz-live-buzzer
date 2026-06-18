#ifndef CONNECTION_MOCKS_H
#define CONNECTION_MOCKS_H

#include <gmock/gmock.h>
#include <string>
#include "PayloadSocketWriter.h"

class MockPayloadSocketWriter : public PayloadSocketWriter
{
public:
    MOCK_METHOD(void, Write, (const std::string&), (override));
};

#endif