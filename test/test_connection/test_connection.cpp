#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../mocks/GameMocks.h"
#include "../mocks/SessionMocks.h"
#include "../mocks/ConnectionMocks.h"
#include "../mocks/StartupMocks.h"
#include "TokenSessionMessageSender.h"
#include "MessageSerializer.h"
#include "ReconnectionPolicy.h"
#include "ConnectionEventHandler.h"
#include "ConnectionEventListener.h"
#include "SessionMessageSender.h"
#include "BuzzerBehavior.h"
#include "HubMessageDispatcher.h"
#include "BuzzerEventTranslator.h"
#include "BuzzerEventType.h"
#include "NetworkHalt.h"
#include "LoginRefusalHalt.h"
#include "MessageRenderer.h"
#include "LcdRenderer.h"
#include "../mocks/ScreenMocks.h"

class MockMessageRenderer : public MessageRenderer
{
public:
    MOCK_METHOD(void, Apply, (const HubMessage &), (override));
};

class MockConnectionEventListener : public ConnectionEventListener
{
public:
    MOCK_METHOD(void, OnConnectionEstablished, (), (override));
    MOCK_METHOD(void, OnConnectionLost, (), (override));
    MOCK_METHOD(void, OnConnectionAttemptFailed, (), (override));
    MOCK_METHOD(void, OnMessageReceived, (const std::string &), (override));
};

class ConnectionEventHandlerTest : public ::testing::Test
{
protected:
    MockMessageRenderer mockMessageRenderer;
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    MockSessionMessageSender mockSessionMessageSender;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, mockMessageRenderer, networkHalt};

    void EliminateTheBuzzer()
    {
        handler.OnMessageReceived("{\"type\":\"auth_success\"}");
        handler.OnConnectionLost();
        handler.OnConnectionLost();
        handler.OnConnectionLost();
        handler.OnConnectionLost();
    }
};

TEST_F(ConnectionEventHandlerTest, BuzzerEliminatedAtTheFourthDisconnectionAfterAuthSuccess)
{
    policy.OnAuthSuccess();

    handler.OnConnectionLost();
    handler.OnConnectionLost();
    handler.OnConnectionLost();
    EXPECT_FALSE(policy.IsEliminated());

    handler.OnConnectionLost();
    EXPECT_TRUE(policy.IsEliminated());
}

TEST_F(ConnectionEventHandlerTest, BuzzerEliminatedAtTheThirdFailedConnectionAttemptAfterTheLossOfTheEstablishedConnection)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnConnectionLost();

    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
    EXPECT_FALSE(policy.IsEliminated());

    handler.OnConnectionAttemptFailed();
    EXPECT_TRUE(policy.IsEliminated());
}

TEST_F(ConnectionEventHandlerTest, FailedConnectionAttemptsCountEvenWithoutALossOfTheConnectionBefore)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");

    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();

    EXPECT_TRUE(policy.IsEliminated());
}

TEST_F(ConnectionEventHandlerTest, ArmedBuzzerIsDisarmedWhenTheHubStaysUnreachable)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnMessageReceived("{\"type\":\"question_choices\"}");
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
    EXPECT_TRUE(buzzerBehavior.IsArmed());

    handler.OnConnectionAttemptFailed();

    EXPECT_FALSE(buzzerBehavior.IsArmed());
}

TEST_F(ConnectionEventHandlerTest, FailedConnectionAttemptsBeforeTheFirstAuthSuccessNeverEliminate)
{
    for (int attempt = 0; attempt < 10; attempt++)
        handler.OnConnectionAttemptFailed();

    EXPECT_FALSE(policy.IsEliminated());
}

TEST_F(ConnectionEventHandlerTest, AuthSuccessBetweenFailedConnectionAttemptsResetsTheCount)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnConnectionLost();

    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();

    EXPECT_FALSE(policy.IsEliminated());
}

TEST_F(ConnectionEventHandlerTest, ConnectionAttemptFailedEventEliminatesTheBuzzerAfterThreeFailures)
{
    BuzzerEventTranslator translator{handler};
    translator.TranslateBuzzerEvent({BuzzerEventType::TextReceived, "{\"type\":\"auth_success\"}"});
    translator.TranslateBuzzerEvent({BuzzerEventType::Disconnected, ""});

    translator.TranslateBuzzerEvent({BuzzerEventType::ConnectionAttemptFailed, ""});
    translator.TranslateBuzzerEvent({BuzzerEventType::ConnectionAttemptFailed, ""});
    EXPECT_FALSE(policy.IsEliminated());

    translator.TranslateBuzzerEvent({BuzzerEventType::ConnectionAttemptFailed, ""});
    EXPECT_TRUE(policy.IsEliminated());
}

TEST_F(ConnectionEventHandlerTest, McqArmedBuzzerIgnoresAPressOnceEliminated)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer(::testing::_)).Times(0);

    handler.OnMessageReceived("{\"type\":\"question_choices\"}");
    EliminateTheBuzzer();
    buzzerBehavior.OnMcqPressed('A');

    EXPECT_FALSE(buzzerBehavior.IsArmed());
}

TEST_F(ConnectionEventHandlerTest, SpeedArmedBuzzerIgnoresAPressOnceEliminated)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);

    handler.OnMessageReceived("{\"type\":\"question_open\"}");
    EliminateTheBuzzer();
    buzzerBehavior.OnBuzzPressed();

    EXPECT_FALSE(buzzerBehavior.IsArmed());
}

TEST_F(ConnectionEventHandlerTest, ArmedBuzzerStillAnswersAfterThreeDisconnections)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer('B')).Times(1);

    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnMessageReceived("{\"type\":\"question_choices\"}");
    handler.OnConnectionLost();
    handler.OnConnectionLost();
    handler.OnConnectionLost();
    buzzerBehavior.OnMcqPressed('B');
}

TEST_F(ConnectionEventHandlerTest, QuestionChoicesReceivedOnceEliminatedArmsNothing)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);

    EliminateTheBuzzer();
    handler.OnMessageReceived("{\"type\":\"question_choices\"}");
    buzzerBehavior.OnMcqPressed('A');

    EXPECT_FALSE(buzzerBehavior.IsArmed());
}

TEST_F(ConnectionEventHandlerTest, QuestionOpenReceivedOnceEliminatedArmsNothing)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);

    EliminateTheBuzzer();
    handler.OnMessageReceived("{\"type\":\"question_open\"}");
    buzzerBehavior.OnBuzzPressed();

    EXPECT_FALSE(buzzerBehavior.IsArmed());
}

TEST_F(ConnectionEventHandlerTest, BuzzUnlockedReceivedOnceEliminatedArmsNothing)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);

    EliminateTheBuzzer();
    handler.OnMessageReceived("{\"type\":\"buzz_unlocked\"}");
    buzzerBehavior.OnBuzzPressed();

    EXPECT_FALSE(buzzerBehavior.IsArmed());
}

TEST_F(ConnectionEventHandlerTest, MessageReceivedOnceEliminatedIsNotRendered)
{
    EliminateTheBuzzer();

    EXPECT_CALL(mockMessageRenderer, Apply(::testing::_)).Times(0);

    handler.OnMessageReceived("{\"type\":\"question_choices\"}");
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
}

TEST_F(ConnectionEventHandlerTest, TokenExpiringSoonReceivedOnceEliminatedSendsNothing)
{
    EliminateTheBuzzer();

    EXPECT_CALL(mockSessionMessageSender, SendAuthRefresh()).Times(0);

    handler.OnMessageReceived("{\"type\":\"token_expiring_soon\"}");
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
    handler.OnConnectionLost();

    EXPECT_TRUE(policy.IsEliminated());
}

TEST_F(ConnectionEventHandlerTest, TokenExpiringSoonTriggersAuthRefresh)
{
    EXPECT_CALL(mockSessionMessageSender, SendAuthRefresh()).Times(1);

    handler.OnMessageReceived("{\"type\":\"token_expiring_soon\"}");
}

TEST_F(ConnectionEventHandlerTest, DisconnectedEventTriggersConnectionLost)
{
    BuzzerEventTranslator translator{handler};
    policy.OnAuthSuccess();

    translator.TranslateBuzzerEvent({BuzzerEventType::Disconnected, ""});
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

TEST(BuzzerEventTranslatorTest, DisconnectionTriggersConnectionLost)
{
    MockConnectionEventListener mock;
    BuzzerEventTranslator translator{mock};

    EXPECT_CALL(mock, OnConnectionLost()).Times(1);

    translator.TranslateBuzzerEvent({BuzzerEventType::Disconnected, ""});
}

TEST(BuzzerEventTranslatorTest, ConnectionAttemptFailedTriggersConnectionAttemptFailed)
{
    MockConnectionEventListener mock;
    BuzzerEventTranslator translator{mock};

    EXPECT_CALL(mock, OnConnectionAttemptFailed()).Times(1);
    EXPECT_CALL(mock, OnConnectionLost()).Times(0);

    translator.TranslateBuzzerEvent({BuzzerEventType::ConnectionAttemptFailed, ""});
}

TEST(BuzzerEventTranslatorTest, TextReceivedTriggersMessageReceived)
{
    MockConnectionEventListener mock;
    BuzzerEventTranslator translator{mock};

    EXPECT_CALL(mock, OnMessageReceived("{\"type\":\"question_open\"}")).Times(1);

    translator.TranslateBuzzerEvent({BuzzerEventType::TextReceived, "{\"type\":\"question_open\"}"});
}

TEST_F(ConnectionEventHandlerTest, EstablishedConnectionTriggersAuth)
{
    EXPECT_CALL(mockSessionMessageSender, SendAuth()).Times(1);

    handler.OnConnectionEstablished();
}

TEST(BuzzerEventTranslatorTest, ConnectionEstablishedTriggersConnectionHandling)
{
    MockConnectionEventListener mock;
    BuzzerEventTranslator translator{mock};

    EXPECT_CALL(mock, OnConnectionEstablished()).Times(1);

    translator.TranslateBuzzerEvent({BuzzerEventType::Connected, ""});
}

TEST_F(ConnectionEventHandlerTest, ReceivedMessageIsRenderedByType)
{
    EXPECT_CALL(mockMessageRenderer, Apply(::testing::Property(&HubMessage::Type, "auth_success"))).Times(1);

    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
}

TEST_F(ConnectionEventHandlerTest, BuzzerEliminatedByFailedConnectionAttemptsClosesTheConnectionToTheHub)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();

    EXPECT_CALL(mockConnectionCloser, Close()).Times(1);

    handler.OnConnectionAttemptFailed();
}

TEST_F(ConnectionEventHandlerTest, BuzzerEliminatedByDisconnectionsClosesTheConnectionToTheHub)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnConnectionLost();
    handler.OnConnectionLost();
    handler.OnConnectionLost();

    EXPECT_CALL(mockConnectionCloser, Close()).Times(1);

    handler.OnConnectionLost();
}

TEST_F(ConnectionEventHandlerTest, BuzzerNotYetEliminatedKeepsItsConnectionOpen)
{
    EXPECT_CALL(mockConnectionCloser, Close()).Times(0);

    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
}

TEST_F(ConnectionEventHandlerTest, DisconnectionEventOfTheClosedConnectionIsNotCountedOnceTheNetworkIsHalted)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
    // La bibliothèque émet la déconnexion pendant la fermeture du client.
    ON_CALL(mockConnectionCloser, Close()).WillByDefault([this]() { handler.OnConnectionLost(); });

    networkHalt.Halt();

    EXPECT_FALSE(policy.IsEliminated());
}

TEST(ConnectionEventHandlerRenderingTest, TimerEndIsRenderedWithTheModeOfBeforeTheMessage)
{
    ::testing::NiceMock<MockHubMessageSender> mockHubMessageSender;
    ::testing::NiceMock<MockLocalFeedback> mockLocalFeedback;
    ::testing::NiceMock<MockSessionMessageSender> mockSessionMessageSender;
    MockScreen mockScreen;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    LcdRenderer lcdRenderer{mockScreen, buzzerBehavior};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, networkHalt};
    buzzerBehavior.OnQuestionOpen();

    EXPECT_CALL(mockScreen, Display(TopBar::Hidden(), ScreenModel{TextScreen{"Trop tard"}})).Times(1);

    handler.OnMessageReceived("{\"type\":\"timer_end\"}");
}

TEST(ConnectionEventHandlerRenderingTest, TimerEndReceivedByAnArmedBuzzerOnceEliminatedIsNotRendered)
{
    ::testing::NiceMock<MockHubMessageSender> mockHubMessageSender;
    ::testing::NiceMock<MockLocalFeedback> mockLocalFeedback;
    ::testing::NiceMock<MockSessionMessageSender> mockSessionMessageSender;
    ::testing::NiceMock<MockScreen> mockScreen;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    LcdRenderer lcdRenderer{mockScreen, buzzerBehavior};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, networkHalt};
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnMessageReceived("{\"type\":\"question_open\"}");
    handler.OnConnectionLost();
    handler.OnConnectionLost();
    handler.OnConnectionLost();
    handler.OnConnectionLost();

    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(0);

    handler.OnMessageReceived("{\"type\":\"timer_end\"}");
}

TEST(ConnectionEventHandlerRenderingTest, AuthSuccessIsRenderedWithTheUsernameOfTheHubInTheTopBar)
{
    ::testing::NiceMock<MockHubMessageSender> mockHubMessageSender;
    ::testing::NiceMock<MockLocalFeedback> mockLocalFeedback;
    ::testing::NiceMock<MockSessionMessageSender> mockSessionMessageSender;
    MockScreen mockScreen;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    LcdRenderer lcdRenderer{mockScreen, buzzerBehavior};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, networkHalt};

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01"), ScreenModel{TextScreen{"Connecté"}})).Times(1);

    handler.OnMessageReceived("{\"type\":\"auth_success\",\"role\":\"buzzer\",\"username\":\"quiz_buzzer_01\",\"expires_in\":3600}");
}

TEST(ConnectionEventHandlerSessionTest, ReconnectionSendsAFreshTokenAfterTheHubConfirmedTheAuthentication)
{
    ::testing::NiceMock<MockMessageRenderer> mockMessageRenderer;
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    MockPayloadSocketWriter mockPayloadSocketWriter;
    MockTokenFetcher mockTokenFetcher;
    ::testing::StrictMock<MockLoginRefusalListener> mockLoginRefusalListener;
    TokenSessionMessageSender tokenSessionMessageSender{mockPayloadSocketWriter, MessageSerializer{}, mockTokenFetcher, mockLoginRefusalListener};
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ConnectionEventHandler handler{policy, dispatcher, tokenSessionMessageSender, mockMessageRenderer, networkHalt};
    tokenSessionMessageSender.UpdateToken("login-token");

    handler.OnMessageReceived("{\"type\":\"auth_success\",\"role\":\"buzzer\",\"username\":\"quiz_buzzer_01\",\"expires_in\":3600}");

    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(::testing::Return(LoginResult{LoginStatus::Success, "fresh-token"}));
    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth\",\"token\":\"fresh-token\"}")).Times(1);
    handler.OnConnectionEstablished();
}

TEST(ConnectionEventHandlerSessionTest, LoginRefusedDuringAReconnectionClosesTheConnectionWithoutCountingItsDisconnection)
{
    ::testing::NiceMock<MockMessageRenderer> mockMessageRenderer;
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    ::testing::StrictMock<MockPayloadSocketWriter> mockPayloadSocketWriter;
    MockTokenFetcher mockTokenFetcher;
    ::testing::NiceMock<MockScreen> mockScreen;
    LoginOutcomeDisplay loginOutcomeDisplay{mockScreen};
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    LoginRefusalHalt loginRefusalHalt{loginOutcomeDisplay, networkHalt};
    TokenSessionMessageSender tokenSessionMessageSender{mockPayloadSocketWriter, MessageSerializer{}, mockTokenFetcher, loginRefusalHalt};
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ConnectionEventHandler handler{policy, dispatcher, tokenSessionMessageSender, mockMessageRenderer, networkHalt};
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
    // La bibliothèque émet la déconnexion pendant la fermeture du client.
    ON_CALL(mockConnectionCloser, Close()).WillByDefault([&handler]() { handler.OnConnectionLost(); });

    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(::testing::Return(LoginResult{LoginStatus::InvalidCredentials, ""}));
    EXPECT_CALL(mockConnectionCloser, Close()).Times(1);
    handler.OnConnectionEstablished();

    EXPECT_FALSE(policy.IsEliminated());
}

class InvalidHubTextTest : public ConnectionEventHandlerTest, public ::testing::WithParamInterface<std::string>
{
};

TEST_P(InvalidHubTextTest, IsNotRendered)
{
    EXPECT_CALL(mockMessageRenderer, Apply(::testing::_)).Times(0);

    handler.OnMessageReceived(GetParam());
}

TEST_P(InvalidHubTextTest, ChangesNeitherTheModeNorSendsAnything)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockSessionMessageSender, SendAuth()).Times(0);
    EXPECT_CALL(mockSessionMessageSender, SendAuthRefresh()).Times(0);

    handler.OnMessageReceived(GetParam());
    buzzerBehavior.OnBuzzPressed();
    buzzerBehavior.OnMcqPressed('A');
}

INSTANTIATE_TEST_SUITE_P(
    NotACompleteJsonObjectWithATextType,
    InvalidHubTextTest,
    ::testing::Values(
        std::string{""},
        std::string{"hello"},
        std::string{"[]"},
        std::string{"42"},
        std::string{"null"},
        std::string{"{}"},
        std::string{"{\"type\":42}"},
        std::string{"{\"type\":\"question_open\""},
        std::string{"{\"type\":\"question_open\"}garbage"}));

TEST(NetworkHaltTest, ClosesTheConnectionWhenHalted)
{
    MockConnectionCloser mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};

    EXPECT_CALL(mockConnectionCloser, Close()).Times(1);

    networkHalt.Halt();
}

TEST(NetworkHaltTest, IsHaltedOnceHalted)
{
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};

    EXPECT_FALSE(networkHalt.IsHalted());

    networkHalt.Halt();

    EXPECT_TRUE(networkHalt.IsHalted());
}

TEST(NetworkHaltTest, ClosesTheConnectionOnlyOnceWhenHaltedAgain)
{
    MockConnectionCloser mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};

    EXPECT_CALL(mockConnectionCloser, Close()).Times(1);

    networkHalt.Halt();
    networkHalt.Halt();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}