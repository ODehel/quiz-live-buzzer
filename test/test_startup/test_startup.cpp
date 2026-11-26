#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../mocks/ConnectionMocks.h"
#include "../mocks/ScreenMocks.h"
#include "../mocks/StartupMocks.h"
#include "BuzzerLoop.h"
#include "ButtonPressDetector.h"
#include "ConnectionOpener.h"
#include "InitScreenDisplay.h"
#include "LoginOutcomeDisplay.h"
#include "NetworkHalt.h"
#include "NetworkLoop.h"
#include "StartupSequence.h"
#include "TokenAcquisition.h"
#include "TappableZonesScreen.h"
#include "TokenUpdater.h"

using ::testing::Return;

TEST(TokenAcquisitionTest, ReturnsTheTokenWithoutWaitingWhenTheLoginSucceedsImmediately)
{
    MockTokenFetcher mockTokenFetcher;
    MockSleeper mockSleeper;
    TokenAcquisition tokenAcquisition{mockTokenFetcher, mockSleeper, 5000};

    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(Return(LoginResult{LoginStatus::Success, "any-token"}));
    EXPECT_CALL(mockSleeper, Sleep(::testing::_)).Times(0);

    LoginResult result = tokenAcquisition.Acquire();

    EXPECT_EQ(LoginStatus::Success, result.status);
    EXPECT_EQ("any-token", result.token);
}

TEST(TokenAcquisitionTest, WaitsTheRetryDelayThenTriesAgainWhenTheLoginFails)
{
    MockTokenFetcher mockTokenFetcher;
    MockSleeper mockSleeper;
    TokenAcquisition tokenAcquisition{mockTokenFetcher, mockSleeper, 1234};

    ::testing::InSequence sequence;
    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(Return(LoginResult{LoginStatus::Failed, ""}));
    EXPECT_CALL(mockSleeper, Sleep(1234)).Times(1);
    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(Return(LoginResult{LoginStatus::Success, "any-token"}));

    LoginResult result = tokenAcquisition.Acquire();

    EXPECT_EQ(LoginStatus::Success, result.status);
    EXPECT_EQ("any-token", result.token);
}

TEST(TokenAcquisitionTest, KeepsTryingAsLongAsTheLoginFails)
{
    MockTokenFetcher mockTokenFetcher;
    MockSleeper mockSleeper;
    TokenAcquisition tokenAcquisition{mockTokenFetcher, mockSleeper, 5000};

    EXPECT_CALL(mockTokenFetcher, Fetch())
        .WillOnce(Return(LoginResult{LoginStatus::Failed, ""}))
        .WillOnce(Return(LoginResult{LoginStatus::Failed, ""}))
        .WillOnce(Return(LoginResult{LoginStatus::Failed, ""}))
        .WillOnce(Return(LoginResult{LoginStatus::Success, "any-token"}));
    EXPECT_CALL(mockSleeper, Sleep(5000)).Times(3);

    LoginResult result = tokenAcquisition.Acquire();

    EXPECT_EQ("any-token", result.token);
}

TEST(TokenAcquisitionTest, StopsAtOnceWhenTheCredentialsAreRefused)
{
    MockTokenFetcher mockTokenFetcher;
    MockSleeper mockSleeper;
    TokenAcquisition tokenAcquisition{mockTokenFetcher, mockSleeper, 5000};

    EXPECT_CALL(mockTokenFetcher, Fetch()).Times(1).WillOnce(Return(LoginResult{LoginStatus::InvalidCredentials, ""}));
    EXPECT_CALL(mockSleeper, Sleep(::testing::_)).Times(0);

    LoginResult result = tokenAcquisition.Acquire();

    EXPECT_EQ(LoginStatus::InvalidCredentials, result.status);
    EXPECT_EQ("", result.token);
}

TEST(TokenAcquisitionTest, StopsWhenTheCredentialsAreRefusedAfterAFailure)
{
    MockTokenFetcher mockTokenFetcher;
    MockSleeper mockSleeper;
    TokenAcquisition tokenAcquisition{mockTokenFetcher, mockSleeper, 5000};

    EXPECT_CALL(mockTokenFetcher, Fetch())
        .WillOnce(Return(LoginResult{LoginStatus::Failed, ""}))
        .WillOnce(Return(LoginResult{LoginStatus::InvalidCredentials, ""}));
    EXPECT_CALL(mockSleeper, Sleep(5000)).Times(1);

    LoginResult result = tokenAcquisition.Acquire();

    EXPECT_EQ(LoginStatus::InvalidCredentials, result.status);
}

TEST(TokenAcquisitionTest, TreatsASuccessWithoutTokenAsAFailure)
{
    MockTokenFetcher mockTokenFetcher;
    MockSleeper mockSleeper;
    TokenAcquisition tokenAcquisition{mockTokenFetcher, mockSleeper, 5000};

    EXPECT_CALL(mockTokenFetcher, Fetch())
        .WillOnce(Return(LoginResult{LoginStatus::Success, ""}))
        .WillOnce(Return(LoginResult{LoginStatus::Success, "any-token"}));
    EXPECT_CALL(mockSleeper, Sleep(5000)).Times(1);

    LoginResult result = tokenAcquisition.Acquire();

    EXPECT_EQ("any-token", result.token);
}

TEST(InitScreenDisplayTest, ShowsTheLogoAndTheConnectionLabelUnderAnEmptyTopBarWithTheLinkCut)
{
    MockScreen mockScreen;
    InitScreenDisplay initScreenDisplay{mockScreen};

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("").WithLinkCut(), ScreenModel{TextScreen{"Buzz’\nO’Live\nConnexion…"}})).Times(1);

    initScreenDisplay.Show();
}

TEST(InitScreenDisplayTest, HidesTheTappableZonesWhenItGoesThroughTheTappableZonesScreen)
{
    MockScreen mockScreen;
    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(::testing::AnyNumber());
    TappableZonesScreen tappableZonesScreen{mockScreen, 800, 480};
    InitScreenDisplay initScreenDisplay{tappableZonesScreen};
    tappableZonesScreen.Display(TopBar::Hidden(), ZonesScreen{{"A", "B", "C", "D"}});
    ASSERT_FALSE(tappableZonesScreen.Zones().empty());

    initScreenDisplay.Show();

    EXPECT_TRUE(tappableZonesScreen.Zones().empty());
}

TEST(LoginOutcomeDisplayTest, ShowsTheRefusalScreenUnderTheInitTopBarWhenTheCredentialsAreRefused)
{
    MockScreen mockScreen;
    LoginOutcomeDisplay loginOutcomeDisplay{mockScreen};

    EXPECT_CALL(mockScreen, Display(ScreenPresenter{}.TopBarForInit(), ScreenModel{ScreenPresenter{}.ScreenForLoginRefused()})).Times(1);

    loginOutcomeDisplay.Show(LoginResult{LoginStatus::InvalidCredentials, ""});
}

class LoginOutcomeWithoutRefusalTest : public ::testing::TestWithParam<LoginResult>
{
};

TEST_P(LoginOutcomeWithoutRefusalTest, LeavesTheScreenUntouched)
{
    MockScreen mockScreen;
    LoginOutcomeDisplay loginOutcomeDisplay{mockScreen};

    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(0);

    loginOutcomeDisplay.Show(GetParam());
}

INSTANTIATE_TEST_SUITE_P(
    OutcomesThatAreNotARefusal,
    LoginOutcomeWithoutRefusalTest,
    ::testing::Values(LoginResult{LoginStatus::Success, "any-token"}, LoginResult{LoginStatus::Failed, ""}));

class MockTokenUpdater : public TokenUpdater
{
public:
    MOCK_METHOD(void, UpdateToken, (const std::string &), (override));
};

class MockConnectionOpener : public ConnectionOpener
{
public:
    MOCK_METHOD(void, SetReconnectInterval, (unsigned long), (override));
    MOCK_METHOD(void, Open, (), (override));
};

class MockNetworkLoop : public NetworkLoop
{
public:
    MOCK_METHOD(void, Loop, (), (override));
};

class MockButtonReader : public ButtonReader
{
public:
    MOCK_METHOD(ButtonInput, DeduceButton, (), (override));
};

class MockButtonPressTranslator : public ButtonPressTranslator
{
public:
    MOCK_METHOD(void, TranslateButtonPress, (ButtonInput), (override));
};

// La séquence de démarrage et la boucle câblées comme dans `main.cpp` : mêmes objets, mêmes ports, dont les lecteurs d'appui.
class StartupAndLoopTest : public ::testing::Test
{
protected:
    static constexpr unsigned long RetryDelayMilliseconds = 5000;
    static constexpr unsigned long ReconnectIntervalMilliseconds = 4321;

    MockTokenFetcher mockTokenFetcher;
    MockSleeper mockSleeper;
    TokenAcquisition tokenAcquisition{mockTokenFetcher, mockSleeper, RetryDelayMilliseconds};
    MockScreen mockScreen;
    LoginOutcomeDisplay loginOutcomeDisplay{mockScreen};
    MockTokenUpdater mockTokenUpdater;
    MockConnectionOpener mockConnectionOpener;
    StartupSequence startupSequence{tokenAcquisition, loginOutcomeDisplay, mockTokenUpdater, mockConnectionOpener, ReconnectIntervalMilliseconds};

    MockConnectionCloser mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    MockNetworkLoop mockNetworkLoop;
    MockButtonReader mockPhysicalButtonReader;
    MockButtonReader mockTouchButtonReader;
    MockButtonPressTranslator mockButtonPressTranslator;
    ButtonPressDetector buttonPressDetector{mockPhysicalButtonReader, mockButtonPressTranslator};
    ButtonPressDetector touchPressDetector{mockTouchButtonReader, mockButtonPressTranslator};
    BuzzerLoop buzzerLoop{startupSequence, networkHalt, mockNetworkLoop, buttonPressDetector, touchPressDetector};

    void ExpectNothingIsReadOrPolled()
    {
        EXPECT_CALL(mockNetworkLoop, Loop()).Times(0);
        EXPECT_CALL(mockPhysicalButtonReader, DeduceButton()).Times(0);
        EXPECT_CALL(mockTouchButtonReader, DeduceButton()).Times(0);
    }

    void ExpectTheWebSocketIsNotOpened()
    {
        EXPECT_CALL(mockTokenUpdater, UpdateToken(::testing::_)).Times(0);
        EXPECT_CALL(mockConnectionOpener, SetReconnectInterval(::testing::_)).Times(0);
        EXPECT_CALL(mockConnectionOpener, Open()).Times(0);
    }

    void ExpectTheRefusalScreen()
    {
        EXPECT_CALL(mockScreen, Display(ScreenPresenter{}.TopBarForInit(), ScreenModel{ScreenPresenter{}.ScreenForLoginRefused()})).Times(1);
    }
};

TEST_F(StartupAndLoopTest, OpensTheWebSocketWithTheTokenOnceTheLoginSucceeds)
{
    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(Return(LoginResult{LoginStatus::Success, "any-token"}));
    EXPECT_CALL(mockSleeper, Sleep(::testing::_)).Times(0);
    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(0);
    ::testing::InSequence sequence;
    EXPECT_CALL(mockTokenUpdater, UpdateToken("any-token")).Times(1);
    EXPECT_CALL(mockConnectionOpener, SetReconnectInterval(ReconnectIntervalMilliseconds)).Times(1);
    EXPECT_CALL(mockConnectionOpener, Open()).Times(1);

    StartupOutcome outcome = startupSequence.Run();

    EXPECT_EQ(StartupOutcome::Started, outcome);
    EXPECT_FALSE(startupSequence.IsHalted());
}

TEST_F(StartupAndLoopTest, OpensTheWebSocketWithTheTokenOfTheLoginThatSucceedsAfterFailures)
{
    EXPECT_CALL(mockTokenFetcher, Fetch())
        .WillOnce(Return(LoginResult{LoginStatus::Failed, ""}))
        .WillOnce(Return(LoginResult{LoginStatus::Success, "second-token"}));
    EXPECT_CALL(mockSleeper, Sleep(RetryDelayMilliseconds)).Times(1);
    EXPECT_CALL(mockTokenUpdater, UpdateToken("second-token")).Times(1);
    EXPECT_CALL(mockConnectionOpener, SetReconnectInterval(ReconnectIntervalMilliseconds)).Times(1);
    EXPECT_CALL(mockConnectionOpener, Open()).Times(1);

    EXPECT_EQ(StartupOutcome::Started, startupSequence.Run());
}

TEST_F(StartupAndLoopTest, IsNotHaltedBeforeItRuns)
{
    EXPECT_FALSE(startupSequence.IsHalted());
}

TEST_F(StartupAndLoopTest, ShowsTheRefusalAndDoesNotOpenTheWebSocketWhenTheCredentialsAreRefusedAtOnce)
{
    EXPECT_CALL(mockTokenFetcher, Fetch()).Times(1).WillOnce(Return(LoginResult{LoginStatus::InvalidCredentials, ""}));
    EXPECT_CALL(mockSleeper, Sleep(::testing::_)).Times(0);
    ExpectTheRefusalScreen();
    ExpectTheWebSocketIsNotOpened();

    StartupOutcome outcome = startupSequence.Run();

    EXPECT_EQ(StartupOutcome::Halted, outcome);
    EXPECT_TRUE(startupSequence.IsHalted());
}

TEST_F(StartupAndLoopTest, ShowsTheRefusalAndDoesNotOpenTheWebSocketWhenTheCredentialsAreRefusedAfterFailures)
{
    EXPECT_CALL(mockTokenFetcher, Fetch())
        .Times(3)
        .WillOnce(Return(LoginResult{LoginStatus::Failed, ""}))
        .WillOnce(Return(LoginResult{LoginStatus::Failed, ""}))
        .WillOnce(Return(LoginResult{LoginStatus::InvalidCredentials, ""}));
    EXPECT_CALL(mockSleeper, Sleep(RetryDelayMilliseconds)).Times(2);
    ExpectTheRefusalScreen();
    ExpectTheWebSocketIsNotOpened();

    EXPECT_EQ(StartupOutcome::Halted, startupSequence.Run());
    EXPECT_TRUE(startupSequence.IsHalted());
}

TEST_F(StartupAndLoopTest, ReadsNoPressAndPollsNoNetworkWhileTheStartupRunsAndTheLoginSucceeds)
{
    ExpectNothingIsReadOrPolled();
    EXPECT_CALL(mockTokenFetcher, Fetch())
        .WillOnce(Return(LoginResult{LoginStatus::Failed, ""}))
        .WillOnce(Return(LoginResult{LoginStatus::Success, "any-token"}));
    EXPECT_CALL(mockSleeper, Sleep(::testing::_)).Times(1);
    EXPECT_CALL(mockTokenUpdater, UpdateToken(::testing::_)).Times(1);
    EXPECT_CALL(mockConnectionOpener, SetReconnectInterval(::testing::_)).Times(1);
    EXPECT_CALL(mockConnectionOpener, Open()).Times(1);

    startupSequence.Run();
}

TEST_F(StartupAndLoopTest, ReadsNoPressAndPollsNoNetworkWhileTheStartupRunsAndTheLoginIsRefused)
{
    ExpectNothingIsReadOrPolled();
    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(Return(LoginResult{LoginStatus::InvalidCredentials, ""}));
    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(1);

    startupSequence.Run();
}

TEST_F(StartupAndLoopTest, TheLoopPollsTheNetworkThenThePhysicalButtonThenTheTouchWhenNothingIsHalted)
{
    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(Return(LoginResult{LoginStatus::Success, "any-token"}));
    EXPECT_CALL(mockTokenUpdater, UpdateToken(::testing::_));
    EXPECT_CALL(mockConnectionOpener, SetReconnectInterval(::testing::_));
    EXPECT_CALL(mockConnectionOpener, Open());
    startupSequence.Run();

    ::testing::InSequence sequence;
    EXPECT_CALL(mockNetworkLoop, Loop()).Times(1);
    EXPECT_CALL(mockPhysicalButtonReader, DeduceButton()).WillOnce(Return(ButtonInput::Unknown));
    EXPECT_CALL(mockTouchButtonReader, DeduceButton()).WillOnce(Return(ButtonInput::Unknown));

    buzzerLoop.Tick();
}

TEST_F(StartupAndLoopTest, ThePressOfEachReaderReachesTheTranslatorWhenNothingIsHalted)
{
    EXPECT_CALL(mockNetworkLoop, Loop()).Times(1);
    EXPECT_CALL(mockPhysicalButtonReader, DeduceButton()).WillOnce(Return(ButtonInput::Buzz));
    EXPECT_CALL(mockTouchButtonReader, DeduceButton()).WillOnce(Return(ButtonInput::A));
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(ButtonInput::Buzz)).Times(1);
    EXPECT_CALL(mockButtonPressTranslator, TranslateButtonPress(ButtonInput::A)).Times(1);

    buzzerLoop.Tick();
}

TEST_F(StartupAndLoopTest, TheLoopDoesNothingOnceTheNetworkIsHalted)
{
    EXPECT_CALL(mockConnectionCloser, Close()).Times(1);
    networkHalt.Halt();
    ExpectNothingIsReadOrPolled();

    buzzerLoop.Tick();
    buzzerLoop.Tick();
}

class LoopAfterARefusedLoginTest : public StartupAndLoopTest, public ::testing::WithParamInterface<bool>
{
};

TEST_P(LoopAfterARefusedLoginTest, DoesNothingWhetherOrNotTheNetworkIsHalted)
{
    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(Return(LoginResult{LoginStatus::InvalidCredentials, ""}));
    EXPECT_CALL(mockScreen, Display(::testing::_, ::testing::_)).Times(1);
    startupSequence.Run();
    bool networkIsHalted = GetParam();
    EXPECT_CALL(mockConnectionCloser, Close()).Times(networkIsHalted ? 1 : 0);
    if (networkIsHalted)
        networkHalt.Halt();
    ASSERT_EQ(networkIsHalted, networkHalt.IsHalted());
    ExpectNothingIsReadOrPolled();

    buzzerLoop.Tick();
    buzzerLoop.Tick();
    buzzerLoop.Tick();
}

INSTANTIATE_TEST_SUITE_P(
    NetworkHaltedOrNot,
    LoopAfterARefusedLoginTest,
    ::testing::Values(false, true));

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
