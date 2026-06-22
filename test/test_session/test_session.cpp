#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../mocks/ConnectionMocks.h"
#include "TokenSessionMessageSender.h"
#include "MessageSerializer.h"

TEST(TokenSessionMessageSenderTest, SendsAuthRefreshCarryingTheUpdatedToken)
{
    MockPayloadSocketWriter mockPayloadSocketWriter;
    MessageSerializer messageSerializer;
    TokenSessionMessageSender sender{mockPayloadSocketWriter, messageSerializer};

    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth_refresh\",\"token\":\"any-token\"}")).Times(1);

    sender.UpdateToken("any-token");
    sender.SendAuthRefresh();
}

TEST(TokenSessionMessageSenderTest, SendsAuthCarryingTheUpdatedToken)
{
    MockPayloadSocketWriter mockPayloadSocketWriter;
    MessageSerializer messageSerializer;
    TokenSessionMessageSender sender{mockPayloadSocketWriter, messageSerializer};

    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth\",\"token\":\"any-token\"}")).Times(1);

    sender.UpdateToken("any-token");
    sender.SendAuth();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}