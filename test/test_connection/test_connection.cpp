#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../mocks/GameMocks.h"
#include "../mocks/SessionMocks.h"
#include "../mocks/ConnectionMocks.h"
#include "../mocks/StartupMocks.h"
#include "../mocks/LoginFakes.h"
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
#include "TappableZonesScreen.h"
#include "LoginOutcomeDisplay.h"
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
    ::testing::NiceMock<MockSessionMessageSender> mockSessionMessageSender;
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
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, buzzerBehavior, visibleTopBar};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};
    buzzerBehavior.OnQuestionOpen();

    EXPECT_CALL(mockScreen, Display(ScreenPresenter{}.TopBarForInit(), ScreenModel{TextScreen{"Trop tard"}})).Times(1);

    handler.OnMessageReceived("{\"type\":\"timer_end\"}");
}

TEST(ConnectionEventHandlerRenderingTest, TimerEndReceivedByAnArmedBuzzerOnceEliminatedIsNotRendered)
{
    ::testing::NiceMock<MockHubMessageSender> mockHubMessageSender;
    ::testing::NiceMock<MockLocalFeedback> mockLocalFeedback;
    ::testing::NiceMock<MockSessionMessageSender> mockSessionMessageSender;
    ::testing::NiceMock<MockScreen> mockScreen;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, buzzerBehavior, visibleTopBar};
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

TEST(ConnectionEventHandlerRenderingTest, QuestionChoicesReceivedByAnArmedMcqBuzzerIsNotRenderedAgain)
{
    ::testing::NiceMock<MockHubMessageSender> mockHubMessageSender;
    ::testing::NiceMock<MockLocalFeedback> mockLocalFeedback;
    ::testing::NiceMock<MockSessionMessageSender> mockSessionMessageSender;
    MockScreen mockScreen;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, buzzerBehavior, visibleTopBar};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};

    EXPECT_CALL(mockScreen, Display(ScreenPresenter{}.TopBarForInit(), ScreenModel{ZonesScreen{std::vector<std::string>{"A : Paris", "B : Lyon", "C : Nice", "D : Lille"}}})).Times(1);

    handler.OnMessageReceived("{\"type\":\"question_choices\",\"choices\":[\"Paris\",\"Lyon\",\"Nice\",\"Lille\"]}");
    handler.OnMessageReceived("{\"type\":\"question_choices\",\"choices\":[\"Rome\",\"Pise\",\"Turin\",\"Gênes\"]}");
}

TEST(ConnectionEventHandlerRenderingTest, QuestionChoicesReceivedByAnArmedSpeedBuzzerIsNotRenderedAndKeepsItsMode)
{
    ::testing::NiceMock<MockHubMessageSender> mockHubMessageSender;
    ::testing::NiceMock<MockLocalFeedback> mockLocalFeedback;
    ::testing::NiceMock<MockSessionMessageSender> mockSessionMessageSender;
    MockScreen mockScreen;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, buzzerBehavior, visibleTopBar};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};
    buzzerBehavior.OnQuestionOpen();

    EXPECT_CALL(mockScreen, Display(ScreenPresenter{}.TopBarForInit(), ScreenModel{TextScreen{"Trop tard"}})).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);

    handler.OnMessageReceived("{\"type\":\"question_choices\",\"choices\":[\"Paris\",\"Lyon\",\"Nice\",\"Lille\"]}");
    buzzerBehavior.OnMcqPressed('A');
    handler.OnMessageReceived("{\"type\":\"timer_end\"}");
}

class ArmingOpeningMessageRenderingTest : public ::testing::TestWithParam<std::string>
{
protected:
    ::testing::NiceMock<MockHubMessageSender> mockHubMessageSender;
    ::testing::NiceMock<MockLocalFeedback> mockLocalFeedback;
    ::testing::NiceMock<MockSessionMessageSender> mockSessionMessageSender;
    MockScreen mockScreen;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, buzzerBehavior, visibleTopBar};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};
};

TEST_P(ArmingOpeningMessageRenderingTest, ReceivedByAnArmedSpeedBuzzerIsNotRenderedAndKeepsItsMode)
{
    buzzerBehavior.OnQuestionOpen();

    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    handler.OnMessageReceived(GetParam());
    buzzerBehavior.OnMcqPressed('A');
    buzzerBehavior.OnBuzzPressed();
}

TEST_P(ArmingOpeningMessageRenderingTest, ReceivedByAnArmedMcqBuzzerIsNotRenderedAndKeepsItsMode)
{
    buzzerBehavior.OnQuestionChoices();

    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);

    handler.OnMessageReceived(GetParam());
    buzzerBehavior.OnBuzzPressed();
    buzzerBehavior.OnMcqPressed('A');
}

INSTANTIATE_TEST_SUITE_P(
    QuestionOpenAndBuzzUnlocked,
    ArmingOpeningMessageRenderingTest,
    ::testing::Values(
        std::string{"{\"type\":\"question_open\",\"title\":\"Quelle est la capitale du Portugal ?\"}"},
        std::string{"{\"type\":\"buzz_unlocked\"}"}));

// Un `timer_tick` ne remplace jamais l'écran (US-FW-011 CA-8, US-FW-012 CA-4) et ne change pas le mode (US-FW-004 CA-5) : le vrai `BuzzerBehavior` le prouve par l'appui qui suit.
class TimerTickRenderingTest : public ::testing::Test
{
protected:
    ::testing::NiceMock<MockHubMessageSender> mockHubMessageSender;
    ::testing::NiceMock<MockLocalFeedback> mockLocalFeedback;
    ::testing::NiceMock<MockSessionMessageSender> mockSessionMessageSender;
    MockScreen mockScreen;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, buzzerBehavior, visibleTopBar};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};
};

TEST_F(TimerTickRenderingTest, ReceivedByAnArmedMcqBuzzerKeepsTheZonesAndTheMode)
{
    EXPECT_CALL(mockScreen, Display(ScreenPresenter{}.TopBarForInit(), ScreenModel{ZonesScreen{std::vector<std::string>{"A : Paris", "B : Lyon", "C : Nice", "D : Lille"}}})).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);

    handler.OnMessageReceived("{\"type\":\"question_choices\",\"choices\":[\"Paris\",\"Lyon\",\"Nice\",\"Lille\"]}");
    handler.OnMessageReceived("{\"type\":\"timer_tick\",\"remaining_seconds\":25}");
    buzzerBehavior.OnBuzzPressed();
    buzzerBehavior.OnMcqPressed('A');
}

TEST_F(TimerTickRenderingTest, ReceivedByAnArmedSpeedBuzzerKeepsTheTitleAndTheMode)
{
    EXPECT_CALL(mockScreen, Display(ScreenPresenter{}.TopBarForInit(), ScreenModel{TextScreen{"Quelle est la capitale du Portugal ?"}})).Times(1);
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    handler.OnMessageReceived("{\"type\":\"question_open\",\"title\":\"Quelle est la capitale du Portugal ?\"}");
    handler.OnMessageReceived("{\"type\":\"timer_tick\",\"remaining_seconds\":25}");
    buzzerBehavior.OnMcqPressed('A');
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(TimerTickRenderingTest, ReceivedByAnInertBuzzerIsNotRenderedAndArmsNothing)
{
    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);

    handler.OnMessageReceived("{\"type\":\"timer_tick\",\"remaining_seconds\":25}");
    buzzerBehavior.OnBuzzPressed();
    buzzerBehavior.OnMcqPressed('A');
}

class TimerTickWithoutUsableRemainingSecondsTest : public ::testing::TestWithParam<std::string>
{
protected:
    ::testing::NiceMock<MockHubMessageSender> mockHubMessageSender;
    ::testing::NiceMock<MockLocalFeedback> mockLocalFeedback;
    ::testing::NiceMock<MockSessionMessageSender> mockSessionMessageSender;
    MockScreen mockScreen;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, buzzerBehavior, visibleTopBar};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};
};

TEST_P(TimerTickWithoutUsableRemainingSecondsTest, LeavesTheScreenUnchangedForAnArmedBuzzer)
{
    buzzerBehavior.OnQuestionOpen();

    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(0);
    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    handler.OnMessageReceived(GetParam());
    buzzerBehavior.OnBuzzPressed();
}

INSTANTIATE_TEST_SUITE_P(
    AbsentOrNotAnInteger,
    TimerTickWithoutUsableRemainingSecondsTest,
    ::testing::Values(
        std::string{"{\"type\":\"timer_tick\"}"},
        std::string{"{\"type\":\"timer_tick\",\"remaining_seconds\":\"25\"}"},
        std::string{"{\"type\":\"timer_tick\",\"remaining_seconds\":2.5}"},
        std::string{"{\"type\":\"timer_tick\",\"remaining_seconds\":null}"}));

TEST(ConnectionEventHandlerRenderingTest, AuthSuccessIsRenderedWithTheUsernameOfTheHubInTheTopBar)
{
    ::testing::NiceMock<MockHubMessageSender> mockHubMessageSender;
    ::testing::NiceMock<MockLocalFeedback> mockLocalFeedback;
    ::testing::NiceMock<MockSessionMessageSender> mockSessionMessageSender;
    MockScreen mockScreen;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, buzzerBehavior, visibleTopBar};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01"), ScreenModel{TextScreen{"En attente du maître du jeu\nLa partie va bientôt commencer"}})).Times(1);

    handler.OnMessageReceived("{\"type\":\"auth_success\",\"role\":\"buzzer\",\"username\":\"quiz_buzzer_01\",\"expires_in\":3600}");
}

TEST(ConnectionEventHandlerRenderingTest, AuthSuccessThenGameResumedAreRenderedAsTwoDisplaysInOrderWithTheSameTopBar)
{
    ::testing::NiceMock<MockHubMessageSender> mockHubMessageSender;
    ::testing::NiceMock<MockLocalFeedback> mockLocalFeedback;
    ::testing::NiceMock<MockSessionMessageSender> mockSessionMessageSender;
    MockScreen mockScreen;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, buzzerBehavior, visibleTopBar};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};

    ::testing::InSequence inSequence;
    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01"), ScreenModel{TextScreen{"En attente du maître du jeu\nLa partie va bientôt commencer"}})).Times(1);
    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01"), ScreenModel{TextScreen{"Partie en cours\nEn attente d’une question"}})).Times(1);

    handler.OnMessageReceived("{\"type\":\"auth_success\",\"role\":\"buzzer\",\"username\":\"quiz_buzzer_01\",\"expires_in\":3600}");
    handler.OnMessageReceived("{\"type\":\"game_resumed\"}");
}

TEST(ConnectionEventHandlerRenderingTest, ThirdFailedReconnectionShowsTheOutOfGameScreenWithTheLastKnownNameThenNothingChangesIt)
{
    ::testing::NiceMock<MockHubMessageSender> mockHubMessageSender;
    ::testing::NiceMock<MockLocalFeedback> mockLocalFeedback;
    ::testing::NiceMock<MockSessionMessageSender> mockSessionMessageSender;
    ::testing::NiceMock<MockScreen> mockScreen;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, buzzerBehavior, visibleTopBar};
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
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{retainingScreen, buzzerBehavior, visibleTopBar};
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

    const DimmedScreen connected{"En attente du maître du jeu\nLa partie va bientôt commencer", {}, std::nullopt};
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
    ScreenLocalFeedback localFeedback{retainingScreen, visibleTopBar, retainingScreen};
    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);
    handler.OnMessageReceived("{\"type\":\"question_choices\",\"choices\":[\"un\",\"deux\",\"trois\",\"quatre\"]}");
    localFeedback.AcknowledgeAnswer('C');

    const DimmedScreen chosen{"", {"A : un", "B : deux", "C : trois", "D : quatre"}, 2};
    EXPECT_CALL(mockScreen, Display(::testing::_, Lost(1, chosen))).Times(1);

    handler.OnConnectionLost();
}

// Les deux producteurs d'écran du câblage de `main.cpp` partagent le même bandeau visible (US-FW-009 CA-3) : le nom de l'`auth_success`
// est celui du feedback local d'un appui, qui ne passe pas par `LcdRenderer`.
class SharedTopBarRenderingTest : public ConnectionLostRenderingTest
{
protected:
    ScreenLocalFeedback localFeedback{retainingScreen, visibleTopBar, retainingScreen};
};

TEST_F(SharedTopBarRenderingTest, TheLocalFeedbackOfAnMcqPressKeepsTheNameOfTheAuthSuccess)
{
    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);
    handler.OnMessageReceived("{\"type\":\"question_choices\",\"choices\":[\"un\",\"deux\",\"trois\",\"quatre\"]}");

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01"),
                                    ScreenModel{ChosenZoneScreen{2, std::vector<std::string>{"A : un", "B : deux", "C : trois", "D : quatre"}}})).Times(1);

    localFeedback.AcknowledgeAnswer('C');
}

TEST_F(SharedTopBarRenderingTest, TheLocalFeedbackOfASpeedPressKeepsTheNameOfTheAuthSuccess)
{
    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01"), ScreenModel{TextScreen{"Buzz envoyé"}})).Times(1);

    localFeedback.AcknowledgeBuzz();
}

TEST_F(SharedTopBarRenderingTest, EveryScreenOfTheGameKeepsTheTopBarFromTheAuthSuccessToTheEnd)
{
    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);
    const TopBar topBar = TopBar::ForPlayer("quiz_buzzer_01");

    ::testing::InSequence inSequence;
    EXPECT_CALL(mockScreen, Display(topBar, ScreenModel{ZonesScreen{std::vector<std::string>{"A : un", "B : deux", "C : trois", "D : quatre"}}})).Times(1);
    EXPECT_CALL(mockScreen, Display(topBar, ::testing::VariantWith<ChosenZoneScreen>(::testing::_))).Times(1);
    EXPECT_CALL(mockScreen, Display(topBar, ::testing::VariantWith<BannerScreen>(::testing::_))).Times(1);
    EXPECT_CALL(mockScreen, Display(topBar, ScreenModel{TextScreen{"Trop tard"}})).Times(1);

    handler.OnMessageReceived("{\"type\":\"question_choices\",\"choices\":[\"un\",\"deux\",\"trois\",\"quatre\"]}");
    localFeedback.AcknowledgeAnswer('A');
    handler.OnMessageReceived("{\"type\":\"answer_received\"}");
    handler.OnMessageReceived("{\"type\":\"timer_end\"}");
}

TEST_F(ConnectionLostRenderingTest, AnAuthSuccessDuringTheLossReplacesTheConnectionLostScreenWithTheLinkedTopBar)
{
    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01"), ScreenModel{TextScreen{"En attente du maître du jeu\nLa partie va bientôt commencer"}})).Times(1);

    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);
}

TEST_F(ConnectionLostRenderingTest, TheQuestionIsNotRestoredByTheAuthSuccessAndTheNextLossDimsTheScreenOfTheAuthSuccess)
{
    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);
    handler.OnMessageReceived("{\"type\":\"question_choices\",\"choices\":[\"un\",\"deux\",\"trois\",\"quatre\"]}");
    handler.OnConnectionLost();
    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);

    const DimmedScreen connected{"En attente du maître du jeu\nLa partie va bientôt commencer", {}, std::nullopt};
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

namespace
{
// Le câblage de `main.cpp` de bout en bout pour le refus du login d'une reconnexion : le rendu passe par `RetainingScreen` puis `TappableZonesScreen`,
// le refus par `LoginOutcomeDisplay` sur `TappableZonesScreen` (hors `RetainingScreen`), et le `401` arrive par le vrai `TokenSessionMessageSender`.
// Seuls l'écran physique, le login asynchrone, l'horloge, la socket et la fermeture du client sont des doublures.
class LoginRefusalOverTheConnectionLostScreenTest : public ::testing::Test
{
protected:
    ::testing::NiceMock<MockHubMessageSender> mockHubMessageSender;
    ::testing::NiceMock<MockLocalFeedback> mockLocalFeedback;
    ::testing::NiceMock<MockPayloadSocketWriter> mockPayloadSocketWriter;
    ::testing::NiceMock<MockScreen> mockScreen;
    TappableZonesScreen tappableZonesScreen{mockScreen, 800, 480};
    RetainingScreen retainingScreen{tappableZonesScreen};
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{retainingScreen, buzzerBehavior, visibleTopBar};
    LoginOutcomeDisplay loginOutcomeDisplay{tappableZonesScreen};
    FakeAsyncLogin login;
    FakeClock clock;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    LoginRefusalHalt loginRefusalHalt{loginOutcomeDisplay, networkHalt};
    TokenSessionMessageSender tokenSessionMessageSender{mockPayloadSocketWriter, MessageSerializer{}, login, clock, 5000, loginRefusalHalt};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ConnectionEventHandler handler{policy, dispatcher, tokenSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};

    // Une question QCM à l'écran, puis la perte de la connexion établie : l'écran de coupure est affiché, sa tentative est la 1ʳᵉ.
    void LoseTheConnectionWhileTheQuestionIsOnScreen()
    {
        handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);
        handler.OnMessageReceived("{\"type\":\"question_choices\",\"choices\":[\"un\",\"deux\",\"trois\",\"quatre\"]}");
        handler.OnConnectionLost();
    }

    // La connexion se rouvre, le login redemandé est refusé (`401`) et `loop()` en recueille le résultat.
    void ReconnectAndGetTheLoginRefused()
    {
        handler.OnConnectionEstablished();
        login.Finish(LoginResult{LoginStatus::InvalidCredentials, ""});
        handler.PollSession();
    }
};
}

TEST_F(LoginRefusalOverTheConnectionLostScreenTest, TheRefusalOfTheLoginOfAReconnectionReplacesTheConnectionLostScreenWithTheFullRefusalScreenUnderTheInitTopBar)
{
    LoseTheConnectionWhileTheQuestionIsOnScreen();

    ::testing::InSequence inSequence;
    EXPECT_CALL(mockScreen, Display(ScreenPresenter{}.TopBarForInit(), ScreenModel{ScreenPresenter{}.ScreenForLoginRefused()})).Times(1);
    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(0);

    ReconnectAndGetTheLoginRefused();

    EXPECT_TRUE(tappableZonesScreen.Zones().empty());
}

TEST_F(LoginRefusalOverTheConnectionLostScreenTest, TheRefusalReplacingTheConnectionLostScreenIsTerminalAndNeitherCountedNorOutOfGame)
{
    LoseTheConnectionWhileTheQuestionIsOnScreen();
    // La bibliothèque émet la déconnexion pendant la fermeture du client.
    ON_CALL(mockConnectionCloser, Close()).WillByDefault([this]() { handler.OnConnectionLost(); });
    ReconnectAndGetTheLoginRefused();
    ASSERT_TRUE(networkHalt.IsHalted());

    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(0);

    handler.OnConnectionLost();
    handler.OnConnectionLost();

    EXPECT_FALSE(policy.IsEliminated());
    EXPECT_EQ(policy.CurrentAttempt(), 1);
}

TEST(ConnectionEventHandlerSessionTest, ReconnectionSendsAFreshTokenAfterTheHubConfirmedTheAuthentication)
{
    ::testing::NiceMock<MockMessageRenderer> mockMessageRenderer;
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    MockPayloadSocketWriter mockPayloadSocketWriter;
    FakeAsyncLogin login;
    FakeClock clock;
    ::testing::StrictMock<MockLoginRefusalListener> mockLoginRefusalListener;
    TokenSessionMessageSender tokenSessionMessageSender{mockPayloadSocketWriter, MessageSerializer{}, login, clock, 5000, mockLoginRefusalListener};
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

    handler.OnConnectionLost();
    handler.OnConnectionEstablished();
    login.Finish(LoginResult{LoginStatus::Success, "fresh-token"});

    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth\",\"token\":\"fresh-token\"}")).Times(1);
    handler.PollSession();
}

TEST(ConnectionEventHandlerSessionTest, LoginRefusedDuringAReconnectionClosesTheConnectionWithoutCountingItsDisconnection)
{
    ::testing::NiceMock<MockMessageRenderer> mockMessageRenderer;
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    ::testing::StrictMock<MockPayloadSocketWriter> mockPayloadSocketWriter;
    FakeAsyncLogin login;
    FakeClock clock;
    ::testing::NiceMock<MockScreen> mockScreen;
    LoginOutcomeDisplay loginOutcomeDisplay{mockScreen};
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    LoginRefusalHalt loginRefusalHalt{loginOutcomeDisplay, networkHalt};
    TokenSessionMessageSender tokenSessionMessageSender{mockPayloadSocketWriter, MessageSerializer{}, login, clock, 5000, loginRefusalHalt};
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

    handler.OnConnectionEstablished();
    login.Finish(LoginResult{LoginStatus::InvalidCredentials, ""});

    EXPECT_CALL(mockConnectionCloser, Close()).Times(1);
    handler.PollSession();

    EXPECT_FALSE(policy.IsEliminated());
}

// Renouvellement de la session (US-FW-007) : un renouvellement par `token_expiring_soon` (CA-5), `token_expired` sans effet (CA-7),
// et l'`auth_success` qui répond à un `auth_refresh` sans effet sur l'écran ni le mode (CA-6).
static const std::string TokenExpiringSoon = "{\"type\":\"token_expiring_soon\"}";
static const std::string AuthSuccess = "{\"type\":\"auth_success\"}";

static auto AnAuthSuccess()
{
    return ::testing::Property(&HubMessage::Type, std::string{"auth_success"});
}

TEST_F(ConnectionEventHandlerTest, TheLossOfTheConnectionTellsTheSessionItsConnectionIsClosed)
{
    EXPECT_CALL(mockSessionMessageSender, OnConnectionClosed()).Times(1);

    handler.OnConnectionLost();
}

TEST_F(ConnectionEventHandlerTest, TheEliminationAbandonsTheLoginInFlight)
{
    EXPECT_CALL(mockSessionMessageSender, Abandon()).Times(1);

    EliminateTheBuzzer();
}

TEST_F(ConnectionEventHandlerTest, ABuzzerNotYetEliminatedAbandonsNothing)
{
    policy.OnAuthSuccess();

    EXPECT_CALL(mockSessionMessageSender, Abandon()).Times(0);

    handler.OnConnectionLost();
    handler.OnConnectionLost();
    handler.OnConnectionLost();
}

TEST_F(ConnectionEventHandlerTest, TokenExpiredSendsNothingAndAsksForNoLogin)
{
    EXPECT_CALL(mockSessionMessageSender, SendAuthRefresh()).Times(0);
    EXPECT_CALL(mockSessionMessageSender, SendAuth()).Times(0);

    handler.OnMessageReceived("{\"type\":\"token_expired\"}");
}

TEST_F(ConnectionEventHandlerTest, TokenExpiredLeavesTheArmedModeUntouched)
{
    handler.OnMessageReceived("{\"type\":\"question_open\"}");

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    handler.OnMessageReceived("{\"type\":\"token_expired\"}");
    buzzerBehavior.OnBuzzPressed();
}

TEST_F(ConnectionEventHandlerTest, AuthSuccessAnsweringAnAuthRefreshIsNotRendered)
{
    ON_CALL(mockSessionMessageSender, PollLogin()).WillByDefault(::testing::Return(true));
    handler.OnMessageReceived(TokenExpiringSoon);
    handler.PollSession();

    EXPECT_CALL(mockMessageRenderer, Apply(AnAuthSuccess())).Times(0);

    handler.OnMessageReceived(AuthSuccess);
}

TEST_F(ConnectionEventHandlerTest, AuthSuccessAfterTheOpeningOfAConnectionIsRenderedEvenAfterARenewal)
{
    ON_CALL(mockSessionMessageSender, PollLogin()).WillByDefault(::testing::Return(true));
    handler.OnMessageReceived(TokenExpiringSoon);
    handler.PollSession();
    handler.OnMessageReceived(AuthSuccess);
    handler.OnConnectionLost();
    handler.OnConnectionEstablished();

    EXPECT_CALL(mockMessageRenderer, Apply(AnAuthSuccess())).Times(1);

    handler.OnMessageReceived(AuthSuccess);
}

TEST_F(ConnectionEventHandlerTest, AuthSuccessIsRenderedWhenNoAuthRefreshWasSent)
{
    ON_CALL(mockSessionMessageSender, PollLogin()).WillByDefault(::testing::Return(false));
    handler.OnMessageReceived(TokenExpiringSoon);
    handler.PollSession();

    EXPECT_CALL(mockMessageRenderer, Apply(AnAuthSuccess())).Times(1);

    handler.OnMessageReceived(AuthSuccess);
}

TEST_F(ConnectionEventHandlerTest, EveryAuthSuccessAnsweringTheAuthRefreshesOfABurstIsSilentAndTheNextOneIsRendered)
{
    ON_CALL(mockSessionMessageSender, PollLogin()).WillByDefault(::testing::Return(true));
    handler.OnMessageReceived(TokenExpiringSoon);
    handler.PollSession();
    handler.OnMessageReceived(TokenExpiringSoon);
    handler.PollSession();

    EXPECT_CALL(mockMessageRenderer, Apply(AnAuthSuccess())).Times(1);

    handler.OnMessageReceived(AuthSuccess);
    handler.OnMessageReceived(AuthSuccess);
    handler.OnMessageReceived(AuthSuccess);
}

TEST_F(ConnectionEventHandlerTest, AnAuthRefreshLeftUnansweredDoesNotSilenceTheAuthSuccessOfTheNextConnection)
{
    ON_CALL(mockSessionMessageSender, PollLogin()).WillByDefault(::testing::Return(true));
    handler.OnMessageReceived(AuthSuccess);
    handler.OnMessageReceived(TokenExpiringSoon);
    handler.PollSession();
    handler.OnConnectionLost();
    handler.OnConnectionEstablished();

    EXPECT_CALL(mockMessageRenderer, Apply(AnAuthSuccess())).Times(1);

    handler.OnMessageReceived(AuthSuccess);
}

TEST_F(ConnectionEventHandlerTest, AuthSuccessAnsweringAnAuthRefreshResetsTheCountOfFailedReconnections)
{
    ON_CALL(mockSessionMessageSender, PollLogin()).WillByDefault(::testing::Return(true));
    handler.OnMessageReceived(AuthSuccess);
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionEstablished();
    handler.OnMessageReceived(TokenExpiringSoon);
    handler.PollSession();

    handler.OnMessageReceived(AuthSuccess);

    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
    EXPECT_FALSE(policy.IsEliminated());
}

TEST_F(ConnectionEventHandlerTest, AuthSuccessAnsweringAnAuthRefreshLeavesTheArmedModeUntouched)
{
    ON_CALL(mockSessionMessageSender, PollLogin()).WillByDefault(::testing::Return(true));
    handler.OnMessageReceived("{\"type\":\"question_open\"}");
    handler.OnMessageReceived(TokenExpiringSoon);
    handler.PollSession();

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    handler.OnMessageReceived(AuthSuccess);
    buzzerBehavior.OnBuzzPressed();
}

class ConnectionEventHandlerRenewalRenderingTest : public ::testing::Test
{
protected:
    ::testing::NiceMock<MockHubMessageSender> mockHubMessageSender;
    ::testing::NiceMock<MockLocalFeedback> mockLocalFeedback;
    ::testing::NiceMock<MockSessionMessageSender> mockSessionMessageSender;
    ::testing::NiceMock<MockScreen> mockScreen;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, buzzerBehavior, visibleTopBar};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};

    void SetUp() override
    {
        ON_CALL(mockSessionMessageSender, PollLogin()).WillByDefault(::testing::Return(true));
    }
};

TEST_F(ConnectionEventHandlerRenewalRenderingTest, AuthSuccessAnsweringAnAuthRefreshChangesNeitherTheScreenNorTheTopBar)
{
    handler.OnMessageReceived("{\"type\":\"question_choices\",\"choices\":[\"a\",\"b\",\"c\",\"d\"]}");
    handler.OnMessageReceived(TokenExpiringSoon);
    handler.PollSession();

    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(0);

    handler.OnMessageReceived(AuthSuccess);
}

TEST_F(ConnectionEventHandlerRenewalRenderingTest, AuthSuccessAfterTheOpeningOfAConnectionIsDisplayedEvenAfterARenewal)
{
    handler.OnMessageReceived(TokenExpiringSoon);
    handler.PollSession();
    handler.OnMessageReceived(AuthSuccess);
    handler.OnConnectionEstablished();

    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(1);

    handler.OnMessageReceived(AuthSuccess);
}

TEST_F(ConnectionEventHandlerRenewalRenderingTest, TokenExpiredChangesNeitherTheScreenNorTheTopBar)
{
    handler.OnMessageReceived("{\"type\":\"question_choices\",\"choices\":[\"a\",\"b\",\"c\",\"d\"]}");

    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(0);

    handler.OnMessageReceived("{\"type\":\"token_expired\"}");
}

class ConnectionEventHandlerRenewalSessionTest : public ::testing::Test
{
protected:
    ::testing::NiceMock<MockMessageRenderer> mockMessageRenderer;
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    ::testing::StrictMock<MockPayloadSocketWriter> mockPayloadSocketWriter;
    FakeAsyncLogin login;
    FakeClock clock;
    ::testing::StrictMock<MockLoginRefusalListener> mockLoginRefusalListener;
    TokenSessionMessageSender tokenSessionMessageSender{mockPayloadSocketWriter, MessageSerializer{}, login, clock, 5000, mockLoginRefusalListener};
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ::testing::NiceMock<MockOutOfGameDisplay> mockOutOfGameDisplay;
    ::testing::NiceMock<MockLinkStateDisplay> mockLinkStateDisplay;
    ConnectionEventHandler handler{policy, dispatcher, tokenSessionMessageSender, mockMessageRenderer, mockOutOfGameDisplay, mockLinkStateDisplay, networkHalt};

    // La connexion est ouverte et authentifiée : le login d'ouverture est terminé, `auth` est parti.
    void AnAuthenticatedConnection()
    {
        EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth\",\"token\":\"opening-token\"}")).Times(1);
        handler.OnConnectionEstablished();
        login.Finish(LoginResult{LoginStatus::Success, "opening-token"});
        handler.PollSession();
        handler.OnMessageReceived(AuthSuccess);
    }
};

TEST_F(ConnectionEventHandlerRenewalSessionTest, TokenExpiringSoonSendsAnAuthRefreshCarryingTheTokenOfAFreshLogin)
{
    AnAuthenticatedConnection();

    handler.OnMessageReceived(TokenExpiringSoon);
    EXPECT_TRUE(login.IsInFlight());
    login.Finish(LoginResult{LoginStatus::Success, "fresh-token"});

    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth_refresh\",\"token\":\"fresh-token\"}")).Times(1);

    handler.PollSession();
}

TEST_F(ConnectionEventHandlerRenewalSessionTest, ABurstOfTokenExpiringSoonRenewsTheSessionOncePerLoginEnded)
{
    AnAuthenticatedConnection();

    handler.OnMessageReceived(TokenExpiringSoon);
    handler.OnMessageReceived(TokenExpiringSoon);
    handler.OnMessageReceived(TokenExpiringSoon);

    EXPECT_EQ(login.starts, 2);
    login.Finish(LoginResult{LoginStatus::Success, "fresh-1"});
    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth_refresh\",\"token\":\"fresh-1\"}")).Times(1);
    handler.PollSession();

    handler.OnMessageReceived(TokenExpiringSoon);
    login.Finish(LoginResult{LoginStatus::Success, "fresh-2"});
    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth_refresh\",\"token\":\"fresh-2\"}")).Times(1);
    handler.PollSession();

    EXPECT_EQ(login.starts, 3);
}

TEST_F(ConnectionEventHandlerRenewalSessionTest, TheSingleAuthSuccessAnsweringTheAuthRefreshOfABurstIsSilentAndTheNextOneIsRendered)
{
    AnAuthenticatedConnection();
    handler.OnMessageReceived(TokenExpiringSoon);
    handler.OnMessageReceived(TokenExpiringSoon);
    login.Finish(LoginResult{LoginStatus::Success, "fresh-token"});
    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth_refresh\",\"token\":\"fresh-token\"}")).Times(1);
    handler.PollSession();

    EXPECT_CALL(mockMessageRenderer, Apply(AnAuthSuccess())).Times(1);

    handler.OnMessageReceived(AuthSuccess);
    handler.OnMessageReceived(AuthSuccess);
}

TEST_F(ConnectionEventHandlerRenewalSessionTest, TokenExpiringSoonWithAFailedLoginSendsNothingAndStopsNothing)
{
    AnAuthenticatedConnection();

    handler.OnMessageReceived(TokenExpiringSoon);
    login.Finish(LoginResult{LoginStatus::Failed, ""});
    handler.PollSession();

    EXPECT_FALSE(networkHalt.IsHalted());
    EXPECT_FALSE(policy.IsEliminated());
}

TEST_F(ConnectionEventHandlerRenewalSessionTest, TokenExpiringSoonWithARefusedLoginSendsNothingAndStopsNothing)
{
    AnAuthenticatedConnection();

    EXPECT_CALL(mockConnectionCloser, Close()).Times(0);

    handler.OnMessageReceived(TokenExpiringSoon);
    login.Finish(LoginResult{LoginStatus::InvalidCredentials, ""});
    handler.PollSession();

    EXPECT_FALSE(networkHalt.IsHalted());
    EXPECT_FALSE(policy.IsEliminated());
}

TEST_F(ConnectionEventHandlerRenewalSessionTest, TokenExpiringSoonWhoseLoginExceedsFiveSecondsSendsNothingAndStopsNothing)
{
    AnAuthenticatedConnection();

    handler.OnMessageReceived(TokenExpiringSoon);
    clock.Advance(5000);
    handler.PollSession();

    EXPECT_EQ(login.abandons, 1);
    EXPECT_FALSE(networkHalt.IsHalted());
    EXPECT_FALSE(policy.IsEliminated());
}

TEST_F(ConnectionEventHandlerRenewalSessionTest, TokenExpiringSoonReceivedDuringTheOpeningLoginIsIgnored)
{
    handler.OnConnectionEstablished();

    handler.OnMessageReceived(TokenExpiringSoon);

    EXPECT_EQ(login.starts, 1);
    login.Finish(LoginResult{LoginStatus::Success, "opening-token"});
    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth\",\"token\":\"opening-token\"}")).Times(1);
    handler.PollSession();
}

TEST_F(ConnectionEventHandlerRenewalSessionTest, TheOpeningLoginDoesNotBlockTheReceptionOfMessages)
{
    handler.OnConnectionEstablished();

    EXPECT_CALL(mockMessageRenderer, Apply(::testing::_)).Times(1);

    handler.OnMessageReceived("{\"type\":\"question_choices\"}");
    EXPECT_TRUE(login.IsInFlight());
}

TEST_F(ConnectionEventHandlerRenewalSessionTest, AConnectionClosedDuringTheRenewalLoginReceivesNothingWhenItEnds)
{
    AnAuthenticatedConnection();
    handler.OnMessageReceived(TokenExpiringSoon);

    handler.OnConnectionLost();
    login.Finish(LoginResult{LoginStatus::Success, "late-token"});

    handler.PollSession();
}

TEST_F(ConnectionEventHandlerRenewalSessionTest, AnAuthRefreshNeverSentLeavesTheAuthSuccessOfTheNextConnectionRendered)
{
    AnAuthenticatedConnection();
    handler.OnMessageReceived(TokenExpiringSoon);
    handler.OnConnectionLost();
    login.Finish(LoginResult{LoginStatus::Success, "late-token"});
    handler.PollSession();
    handler.OnConnectionEstablished();
    login.Finish(LoginResult{LoginStatus::Failed, ""});
    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth\",\"token\":\"late-token\"}")).Times(1);
    handler.PollSession();

    EXPECT_CALL(mockMessageRenderer, Apply(AnAuthSuccess())).Times(1);

    handler.OnMessageReceived(AuthSuccess);
}

TEST_F(ConnectionEventHandlerRenewalSessionTest, ANewConnectionAbandonsTheLoginInFlightAndStartsAFreshOne)
{
    AnAuthenticatedConnection();
    handler.OnMessageReceived(TokenExpiringSoon);
    handler.OnConnectionLost();

    handler.OnConnectionEstablished();

    EXPECT_EQ(login.abandons, 1);
    EXPECT_EQ(login.starts, 3);
}

TEST_F(ConnectionEventHandlerRenewalSessionTest, TheLoginOfTheOpeningIsRefusedWhileTheConnectionIsOpenStopsTheBuzzer)
{
    EXPECT_CALL(mockLoginRefusalListener, OnLoginRefused()).Times(1);

    handler.OnConnectionEstablished();
    login.Finish(LoginResult{LoginStatus::InvalidCredentials, ""});
    handler.PollSession();
}

TEST_F(ConnectionEventHandlerRenewalSessionTest, TheRefusalOfTheLoginOfAClosedConnectionIsIgnored)
{
    handler.OnConnectionEstablished();
    handler.OnConnectionLost();
    login.Finish(LoginResult{LoginStatus::InvalidCredentials, ""});

    EXPECT_CALL(mockLoginRefusalListener, OnLoginRefused()).Times(0);

    handler.PollSession();
}

TEST_F(ConnectionEventHandlerRenewalSessionTest, TheEliminationAbandonsTheLoginInFlightAndNothingIsSentAfterwards)
{
    handler.OnMessageReceived(AuthSuccess);
    handler.OnConnectionEstablished();
    handler.OnConnectionLost();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
    handler.OnConnectionAttemptFailed();
    ASSERT_TRUE(policy.IsEliminated());

    login.Finish(LoginResult{LoginStatus::Success, "abandoned-token"});
    handler.PollSession();

    EXPECT_EQ(login.abandons, 1);
}

TEST_F(ConnectionEventHandlerRenewalSessionTest, TokenExpiredSendsNothingAndAsksForNoLogin)
{
    AnAuthenticatedConnection();

    handler.OnMessageReceived("{\"type\":\"token_expired\"}");

    EXPECT_EQ(login.starts, 1);
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

namespace
{
// Le câblage de `main.cpp` pour un envoi qui échoue : le feedback local réel, `RetainingScreen` et `TappableZonesScreen` sous l'écran factice
// (US-FW-002 CA-10, US-FW-003 CA-11). Le joueur appuie par `BuzzerBehavior` ; l'envoi échoue sans coupure signalée.
class UnsentPressRenderingTest : public ::testing::Test
{
protected:
    ::testing::NiceMock<MockHubMessageSender> mockHubMessageSender;
    ::testing::NiceMock<MockSessionMessageSender> mockSessionMessageSender;
    ::testing::NiceMock<MockScreen> mockScreen;
    TappableZonesScreen tappableZonesScreen{mockScreen, 800, 480};
    RetainingScreen retainingScreen{tappableZonesScreen};
    VisibleTopBar visibleTopBar;
    ScreenLocalFeedback localFeedback{retainingScreen, visibleTopBar, retainingScreen};
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, localFeedback};
    LcdRenderer lcdRenderer{retainingScreen, buzzerBehavior, visibleTopBar};
    HubMessageDispatcher dispatcher{buzzerBehavior};
    ReconnectionPolicy policy;
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    ConnectionEventHandler handler{policy, dispatcher, mockSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};

    const TopBar topBar = TopBar::ForPlayer("quiz_buzzer_01");
    const std::vector<std::string> fourZones{"A : un", "B : deux", "C : trois", "D : quatre"};

    void ArmAnMcqQuestion()
    {
        handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);
        handler.OnMessageReceived("{\"type\":\"question_choices\",\"choices\":[\"un\",\"deux\",\"trois\",\"quatre\"]}");
    }
};
}

TEST_F(UnsentPressRenderingTest, AnAnswerThatIsNotSentBringsTheQuestionZonesBackActiveWithNothingChosenAndNoFailureText)
{
    ArmAnMcqQuestion();
    ON_CALL(mockHubMessageSender, SendAnswer('C')).WillByDefault(::testing::Return(false));

    ::testing::InSequence inSequence;
    EXPECT_CALL(mockScreen, Display(topBar, ScreenModel{ChosenZoneScreen{2, fourZones}})).Times(1);
    EXPECT_CALL(mockScreen, Display(topBar, ScreenModel{ZonesScreen{fourZones}})).Times(1);

    buzzerBehavior.OnMcqPressed('C');

    EXPECT_EQ(tappableZonesScreen.Zones().size(), 4u);
    EXPECT_TRUE(buzzerBehavior.IsArmed());
}

TEST_F(UnsentPressRenderingTest, AnAnswerThatIsSentLeavesTheChosenZoneAndTheZonesInactive)
{
    ArmAnMcqQuestion();

    buzzerBehavior.OnMcqPressed('C');

    EXPECT_TRUE(tappableZonesScreen.Zones().empty());
    EXPECT_FALSE(buzzerBehavior.IsArmed());
}

TEST_F(UnsentPressRenderingTest, ABuzzThatIsNotSentBringsTheOpenQuestionScreenBackWithNoFailureText)
{
    handler.OnMessageReceived(AuthSuccessOfQuizBuzzer01);
    handler.OnMessageReceived("{\"type\":\"question_open\",\"title\":\"Quel est le plus long fleuve ?\"}");
    ON_CALL(mockHubMessageSender, SendBuzz()).WillByDefault(::testing::Return(false));

    ::testing::InSequence inSequence;
    EXPECT_CALL(mockScreen, Display(topBar, ScreenModel{TextScreen{"Buzz envoyé"}})).Times(1);
    EXPECT_CALL(mockScreen, Display(topBar, ScreenModel{TextScreen{"Quel est le plus long fleuve ?"}})).Times(1);

    buzzerBehavior.OnBuzzPressed();

    EXPECT_TRUE(buzzerBehavior.IsArmed());
}

TEST_F(UnsentPressRenderingTest, TheConnectionLostScreenAfterAnUnsentAnswerDimsTheQuestionAsItWasAndCountsTheFirstAttempt)
{
    ArmAnMcqQuestion();
    ON_CALL(mockHubMessageSender, SendAnswer('C')).WillByDefault(::testing::Return(false));
    buzzerBehavior.OnMcqPressed('C');

    ConnectionLostScreen lost = std::get<ConnectionLostScreen>(ScreenPresenter{}.ScreenForConnectionLost(1));
    lost.dimmed = DimmedScreen{"", fourZones, std::nullopt};
    EXPECT_CALL(mockScreen, Display(topBar.WithLinkCut(), ScreenModel{lost})).Times(1);

    handler.OnConnectionLost();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}