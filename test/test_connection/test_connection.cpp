#include <gtest/gtest.h>
#include "../mocks/GameMocks.h"
#include "ReconnectionPolicy.h"
#include "ConnectionEventHandler.h"
#include "BuzzerBehavior.h"
#include "HubMessageDispatcher.h"

class ConnectionEventHandlerTest : public ::testing::Test
{
protected:
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ConnectionEventHandler handler{policy, dispatcher};
};

TEST_F(ConnectionEventHandlerTest, BuzzerEliminatedAfterAThreeTimesDisconnection)
{    
    policy.OnAuthSuccess();

    handler.OnConnectionLost();
    handler.OnConnectionLost();
    handler.OnConnectionLost();

    EXPECT_TRUE(policy.IsEliminated());
}

TEST_F(ConnectionEventHandlerTest, SpeedBuzzArmedOnQuestionOpenMessageReceived)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    handler.OnMessageReceived("{\"type\":\"question_open\"}");
    buzzerBehavior.OnBuzzPressed();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}