#include <gtest/gtest.h>
#include "ReconnectionPolicy.h"
#include "ConnectionEventHandler.h"

TEST(ConnectionEventHandlerTest, BuzzerEliminatedAfterAThreeTimesDisconnection)
{
    ReconnectionPolicy policy;
    ConnectionEventHandler handler(policy);
    policy.OnAuthSuccess();

    handler.OnConnectionLost();
    handler.OnConnectionLost();
    handler.OnConnectionLost();

    EXPECT_TRUE(policy.IsEliminated());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}