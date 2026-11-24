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
#include "RetainingScreen.h"
#include "../mocks/ScreenMocks.h"
#include "ScreenLocalFeedback.h"

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
    ::testing::NiceMock<MockOutOfGameDisplay> mockOutOfGameDisplay;
    ::testing::NiceMock<MockLinkStateDisplay> mockLinkStateDisplay;
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, mockMessageRenderer, mockOutOfGameDisplay, mockLinkStateDisplay, networkHalt};

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

TEST_F(ConnectionEventHandlerTest, OutOfGameScreenIsShownAtTheThirdFailedConnectionAttempt)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();

    EXPECT_CALL(mockOutOfGameDisplay, ShowOutOfGame()).Times(1);

    handler.OnConnectionAttemptFailed();
}

TEST_F(ConnectionEventHandlerTest, OutOfGameScreenIsNotShownBeforeTheThirdFailure)
{
    EXPECT_CALL(mockOutOfGameDisplay, ShowOutOfGame()).Times(0);

    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
}

TEST_F(ConnectionEventHandlerTest, OutOfGameScreenIsShownAtTheFourthDisconnectionAfterAuthSuccess)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnConnectionLost();
    handler.OnConnectionLost();
    handler.OnConnectionLost();

    EXPECT_CALL(mockOutOfGameDisplay, ShowOutOfGame()).Times(1);

    handler.OnConnectionLost();
}

TEST_F(ConnectionEventHandlerTest, OutOfGameScreenIsShownOnlyOnceWhateverComesAfterTheElimination)
{
    EXPECT_CALL(mockOutOfGameDisplay, ShowOutOfGame()).Times(1);

    EliminateTheBuzzer();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionLost();
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
}

TEST_F(ConnectionEventHandlerTest, FailuresBeforeTheFirstAuthSuccessNeverShowTheOutOfGameScreen)
{
    EXPECT_CALL(mockOutOfGameDisplay, ShowOutOfGame()).Times(0);

    for (int i = 0; i < 5; ++i)
    {
        handler.OnConnectionAttemptFailed();
        handler.OnConnectionLost();
    }
}

TEST_F(ConnectionEventHandlerTest, OutOfGameScreenDoesNotReplaceTheScreenOfAHaltedNetwork)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnConnectionLost();
    networkHalt.Halt();

    EXPECT_CALL(mockOutOfGameDisplay, ShowOutOfGame()).Times(0);

    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
}

TEST_F(ConnectionEventHandlerTest, LinkLostIsShownAtTheLossOfTheEstablishedConnectionAsTheFirstAttempt)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");

    EXPECT_CALL(mockLinkStateDisplay, ShowLinkLost(1)).Times(1);

    handler.OnConnectionLost();
}

TEST_F(ConnectionEventHandlerTest, LinkLostIsRedrawnAtEachCountedFailureWithTheNextAttempt)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    ::testing::InSequence inSequence;
    EXPECT_CALL(mockLinkStateDisplay, ShowLinkLost(1)).Times(1);
    EXPECT_CALL(mockLinkStateDisplay, ShowLinkLost(2)).Times(1);
    EXPECT_CALL(mockLinkStateDisplay, ShowLinkLost(3)).Times(1);

    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
}

TEST_F(ConnectionEventHandlerTest, TheThirdCountedFailureShowsTheOutOfGameScreenAndNeverAFourthAttempt)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();

    EXPECT_CALL(mockLinkStateDisplay, ShowLinkLost(::testing::_)).Times(0);
    EXPECT_CALL(mockOutOfGameDisplay, ShowOutOfGame()).Times(1);

    handler.OnConnectionAttemptFailed();
}

TEST_F(ConnectionEventHandlerTest, ADisconnectionCountedAsAFailureIsRedrawnWithTheNextAttempt)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnConnectionLost();

    EXPECT_CALL(mockLinkStateDisplay, ShowLinkLost(2)).Times(1);

    handler.OnConnectionLost();
}

TEST_F(ConnectionEventHandlerTest, NoLinkLostScreenBeforeTheFirstAuthSuccess)
{
    EXPECT_CALL(mockLinkStateDisplay, ShowLinkLost(::testing::_)).Times(0);

    for (int i = 0; i < 5; ++i)
    {
        handler.OnConnectionLost();
        handler.OnConnectionAttemptFailed();
    }
}

TEST_F(ConnectionEventHandlerTest, NoLinkLostScreenOnceTheNetworkIsHalted)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnConnectionLost();
    networkHalt.Halt();

    EXPECT_CALL(mockLinkStateDisplay, ShowLinkLost(::testing::_)).Times(0);

    handler.OnConnectionAttemptFailed();
    handler.OnConnectionLost();
}

TEST_F(ConnectionEventHandlerTest, NoLinkLostScreenOnceTheBuzzerIsEliminated)
{
    EXPECT_CALL(mockLinkStateDisplay, ShowLinkLost(::testing::_)).Times(3);

    EliminateTheBuzzer();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionLost();
}

TEST_F(ConnectionEventHandlerTest, SpeedArmedBuzzerIgnoresAPressWhileTheLinkIsCutAndKeepsItsMode)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);

    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnMessageReceived("{\"type\":\"question_open\"}");
    handler.OnConnectionLost();
    buzzerBehavior.OnBuzzPressed();

    EXPECT_TRUE(buzzerBehavior.IsArmed());
}

TEST_F(ConnectionEventHandlerTest, McqArmedBuzzerIgnoresAPressWhileTheLinkIsCutAndKeepsItsMode)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer(::testing::_)).Times(0);

    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnMessageReceived("{\"type\":\"question_choices\"}");
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    buzzerBehavior.OnMcqPressed('A');

    EXPECT_TRUE(buzzerBehavior.IsArmed());
}

TEST_F(ConnectionEventHandlerTest, PressesStayIgnoredWhenTheConnectionOpensAgainUntilTheAuthSuccess)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);

    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnMessageReceived("{\"type\":\"question_open\"}");
    handler.OnConnectionLost();
    handler.OnConnectionEstablished();
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(ConnectionEventHandlerTest, ThePressGoesOutAgainOnceTheAuthSuccessIsReceivedAndTheModeIsTheOneOfBeforeTheLoss)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(1);

    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnMessageReceived("{\"type\":\"question_open\"}");
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionEstablished();
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");

    EXPECT_TRUE(buzzerBehavior.IsArmed());
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(ConnectionEventHandlerTest, ACutLinkRestoredByTheAuthSuccessRestartsTheAttemptsAtOne)
{
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");

    EXPECT_CALL(mockLinkStateDisplay, ShowLinkLost(1)).Times(1);

    handler.OnConnectionLost();
}

TEST_F(ConnectionEventHandlerTest, AConnectionLostBeforeTheFirstAuthSuccessLeavesThePressesHeard)
{
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnMessageReceived("{\"type\":\"question_open\"}");
    buzzerBehavior.OnBuzzPressed();
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

TEST_F(ConnectionEventHandlerTest, ArmedBuzzerStillAnswersAfterThreeDisconnectionsOnceTheAuthSuccessIsReceived)
{
    EXPECT_CALL(mockHubMessageSender, SendAnswer('B')).Times(1);

    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
    handler.OnMessageReceived("{\"type\":\"question_choices\"}");
    handler.OnConnectionLost();
    handler.OnConnectionLost();
    handler.OnConnectionLost();
    handler.OnMessageReceived("{\"type\":\"auth_success\"}");
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

// La bibliothèque WebSockets 2.7.3 n'émet rien au connect raté lui-même : au tour suivant, elle nettoie le client jamais ouvert et émet alors
// une déconnexion de raison « TCP connection cleanup ». Cette fermeture est l'échec de la tentative, pas une perte de connexion établie.
TEST(BuzzerEventTranslatorTest, DisconnectionOfAnAttemptThatNeverOpenedTriggersConnectionAttemptFailedOnly)
{
    MockConnectionEventListener mock;
    BuzzerEventTranslator translator{mock};

    EXPECT_CALL(mock, OnConnectionAttemptFailed()).Times(1);
    EXPECT_CALL(mock, OnConnectionLost()).Times(0);

    translator.TranslateBuzzerEvent({BuzzerEventType::Disconnected, "TCP connection cleanup"});
}

TEST(BuzzerEventTranslatorTest, DisconnectionWithAnotherReasonStaysAConnectionLost)
{
    MockConnectionEventListener mock;
    BuzzerEventTranslator translator{mock};

    EXPECT_CALL(mock, OnConnectionLost()).Times(1);
    EXPECT_CALL(mock, OnConnectionAttemptFailed()).Times(0);

    translator.TranslateBuzzerEvent({BuzzerEventType::Disconnected, "Connection lost"});
}

// Une tentative ratée ne produit qu'un événement : 2/3 après le premier échec, 3/3 après le deuxième, « Hors jeu » au troisième.
TEST_F(ConnectionEventHandlerTest, EachFailedAttemptReportedByTheLibraryCountsOnceThroughTheTranslator)
{
    BuzzerEventTranslator translator{handler};
    translator.TranslateBuzzerEvent({BuzzerEventType::TextReceived, "{\"type\":\"auth_success\"}"});

    ::testing::InSequence inSequence;
    EXPECT_CALL(mockLinkStateDisplay, ShowLinkLost(1)).Times(1);
    EXPECT_CALL(mockLinkStateDisplay, ShowLinkLost(2)).Times(1);
    EXPECT_CALL(mockLinkStateDisplay, ShowLinkLost(3)).Times(1);
    EXPECT_CALL(mockOutOfGameDisplay, ShowOutOfGame()).Times(1);

    translator.TranslateBuzzerEvent({BuzzerEventType::Disconnected, "Connection lost"});
    translator.TranslateBuzzerEvent({BuzzerEventType::Disconnected, "TCP connection cleanup"});
    translator.TranslateBuzzerEvent({BuzzerEventType::Disconnected, "TCP connection cleanup"});
    translator.TranslateBuzzerEvent({BuzzerEventType::Disconnected, "TCP connection cleanup"});
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
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};
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
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};
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
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01"), ScreenModel{TextScreen{"Connecté"}})).Times(1);

    handler.OnMessageReceived("{\"type\":\"auth_success\",\"role\":\"buzzer\",\"username\":\"quiz_buzzer_01\",\"expires_in\":3600}");
}

TEST(ConnectionEventHandlerRenderingTest, ThirdFailedReconnectionShowsTheOutOfGameScreenWithTheLastKnownNameThenNothingChangesIt)
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
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};
    handler.OnMessageReceived("{\"type\":\"auth_success\",\"role\":\"buzzer\",\"username\":\"quiz_buzzer_01\",\"expires_in\":3600}");
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01").WithLinkCut(), ScreenPresenter{}.ScreenForOutOfGame())).Times(1);
    handler.OnConnectionAttemptFailed();
    ::testing::Mock::VerifyAndClearExpectations(&mockScreen);

    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(0);
    handler.OnMessageReceived("{\"type\":\"auth_success\",\"role\":\"buzzer\",\"username\":\"quiz_buzzer_02\",\"expires_in\":3600}");
    handler.OnMessageReceived("{\"type\":\"question_choices\",\"choices\":[\"a\",\"b\",\"c\",\"d\"]}");
    handler.OnConnectionLost();
}

namespace
{
const char *const AuthSuccessOfQuizBuzzer01 = "{\"type\":\"auth_success\",\"role\":\"buzzer\",\"username\":\"quiz_buzzer_01\",\"expires_in\":3600}";

// Le câblage de `main.cpp` : le rendu et le feedback local passent par `RetainingScreen`, qui retient le dernier écran affiché.
class ConnectionLostRenderingTest : public ::testing::Test
{
protected:
    ::testing::NiceMock<MockHubMessageSender> mockHubMessageSender;
    ::testing::NiceMock<MockLocalFeedback> mockLocalFeedback;
    ::testing::NiceMock<MockSessionMessageSender> mockSessionMessageSender;
    ::testing::NiceMock<MockScreen> mockScreen;
    RetainingScreen retainingScreen{mockScreen};
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    LcdRenderer lcdRenderer{retainingScreen, buzzerBehavior};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};

    static ScreenModel Lost(int attempt, DimmedScreen dimmed, bool bannerOnly = false)
    {
        ConnectionLostScreen screen = std::get<ConnectionLostScreen>(ScreenPresenter{}.ScreenForConnectionLost(attempt));
        screen.dimmed = dimmed;
        screen.bannerOnly = bannerOnly;
        return screen;
    }
};
}

TEST_F(ConnectionLostRenderingTest, TheLossOfTheEstablishedConnectionShowsTheConnectionLostScreenOverTheLastScreenWithoutAnyHubMessage)
{
    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);
    handler.OnMessageReceived("{\"type\":\"question_choices\",\"choices\":[\"un\",\"deux\",\"trois\",\"quatre\"]}");

    const DimmedScreen question{"", {"A : un", "B : deux", "C : trois", "D : quatre"}, std::nullopt};
    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01").WithLinkCut(), Lost(1, question))).Times(1);

    handler.OnConnectionLost();
}

TEST_F(ConnectionLostRenderingTest, EachCountedFailureRepaintsOnlyTheBannerWithTheNextAttemptAndTheThirdOneShowsTheOutOfGameScreen)
{
    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);
    handler.OnConnectionLost();

    const DimmedScreen connected{"Connecté", {}, std::nullopt};
    const TopBar cutTopBar = TopBar::ForPlayer("quiz_buzzer_01").WithLinkCut();
    ::testing::InSequence inSequence;
    EXPECT_CALL(mockScreen, Display(cutTopBar, Lost(2, connected, true))).Times(1);
    EXPECT_CALL(mockScreen, Display(cutTopBar, Lost(3, connected, true))).Times(1);
    EXPECT_CALL(mockScreen, Display(cutTopBar, ScreenModel{ScreenPresenter{}.ScreenForOutOfGame()})).Times(1);

    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
}

TEST_F(ConnectionLostRenderingTest, TheLocalFeedbackOfAPressIsTheLastScreenTheConnectionLostScreenDims)
{
    ScreenLocalFeedback localFeedback{retainingScreen};
    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);
    handler.OnMessageReceived("{\"type\":\"question_choices\",\"choices\":[\"un\",\"deux\",\"trois\",\"quatre\"]}");
    localFeedback.AcknowledgeAnswer('C');

    const DimmedScreen chosen{"", {"A : un", "B : deux", "C : trois", "D : quatre"}, 2};
    EXPECT_CALL(mockScreen, Display(::testing::_, Lost(1, chosen))).Times(1);

    handler.OnConnectionLost();
}

TEST_F(ConnectionLostRenderingTest, AnAuthSuccessDuringTheLossReplacesTheConnectionLostScreenWithTheLinkedTopBar)
{
    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01"), ScreenModel{TextScreen{"Connecté"}})).Times(1);

    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);
}

TEST_F(ConnectionLostRenderingTest, TheQuestionIsNotRestoredByTheAuthSuccessAndTheNextLossDimsTheScreenOfTheAuthSuccess)
{
    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);
    handler.OnMessageReceived("{\"type\":\"question_choices\",\"choices\":[\"un\",\"deux\",\"trois\",\"quatre\"]}");
    handler.OnConnectionLost();
    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);

    const DimmedScreen connected{"Connecté", {}, std::nullopt};
    EXPECT_CALL(mockScreen, Display(::testing::_, Lost(1, connected))).Times(1);

    handler.OnConnectionLost();
}

TEST_F(ConnectionLostRenderingTest, NothingIsDisplayedWhenTheConnectionClosesBeforeTheFirstAuthSuccess)
{
    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(0);

    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionLost();
}

TEST_F(ConnectionLostRenderingTest, NoLinkLostScreenReplacesTheRefusalOfAHaltedNetwork)
{
    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);
    handler.OnConnectionLost();
    networkHalt.Halt();

    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(0);

    handler.OnConnectionAttemptFailed();
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
    ::testing::NiceMock<MockOutOfGameDisplay> mockOutOfGameDisplay;
    ::testing::NiceMock<MockLinkStateDisplay> mockLinkStateDisplay;
    ConnectionEventHandler handler{policy, dispatcher, tokenSessionMessageSender, mockMessageRenderer, mockOutOfGameDisplay, mockLinkStateDisplay, networkHalt};
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
    ::testing::NiceMock<MockOutOfGameDisplay> mockOutOfGameDisplay;
    ::testing::NiceMock<MockLinkStateDisplay> mockLinkStateDisplay;
    ConnectionEventHandler handler{policy, dispatcher, tokenSessionMessageSender, mockMessageRenderer, mockOutOfGameDisplay, mockLinkStateDisplay, networkHalt};
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