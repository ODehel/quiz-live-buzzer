#include <gtest/gtest.h>
#include "../mocks/GameMocks.h"
#include "../mocks/SessionMocks.h"
#include "ReconnectionPolicy.h"
#include "ConnectionEventHandler.h"
#include "SessionMessageSender.h"
#include "BuzzerBehavior.h"
#include "HubMessageDispatcher.h"
#include "BuzzerEventTranslator.h"
#include "BuzzerEventType.h"

class ConnectionEventHandlerTest : public ::testing::Test
{
protected:
    MockTokenReceiver mockTokenReceiver;
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    MockSessionMessageSender mockSessionMessageSender;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, mockTokenReceiver};
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

TEST_F(ConnectionEventHandlerTest, AuthSuccessMessageArmsThePolicy)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");

    handler.OnConnectionLost();
    handler.OnConnectionLost();
    handler.OnConnectionLost();

    EXPECT_TRUE(policy.IsEliminated());
}

TEST_F(ConnectionEventHandlerTest, TokenExpiringSoonTriggersAuthRefresh)
{
    EXPECT_CALL(mockSessionMessageSender, SendAuthRefresh()).Times(1);

    handler.OnMessageReceived("{\"type\":\"token_expiring_soon\"}");
}

TEST_F(ConnectionEventHandlerTest, AuthSuccessTriggersUpdateToken)
{
    EXPECT_CALL(mockTokenReceiver, UpdateToken("any-token")).Times(1);

    handler.OnMessageReceived("{\"type\":\"auth_success\",\"token\":\"any-token\"}");
}

TEST_F(ConnectionEventHandlerTest, DisconnectedEventTriggersConnectionLost)
{
    BuzzerEventTranslator translator{handler};
    policy.OnAuthSuccess();

    translator.TranslateBuzzerEvent({BuzzerEventType::Disconnected, ""});
    translator.TranslateBuzzerEvent({BuzzerEventType::Disconnected, ""});
    translator.TranslateBuzzerEvent({BuzzerEventType::Disconnected, ""});

    EXPECT_TRUE(policy.IsEliminated());
}

TEST_F(ConnectionEventHandlerTest, TextReceivedEventTriggersSpeedBuzzArmed)
{
    BuzzerEventTranslator translator{handler};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    translator.TranslateBuzzerEvent({BuzzerEventType::TextReceived, "{\"type\":\"question_open\"}"});
    buzzerBehavior.OnBuzzPressed();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}