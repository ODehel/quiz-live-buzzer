#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <climits>
#include "../mocks/ConnectionMocks.h"
#include "../mocks/LoginFakes.h"
#include "../mocks/ScreenMocks.h"
#include "../mocks/SessionMocks.h"
#include "../mocks/StartupMocks.h"
#include "TokenSessionMessageSender.h"
#include "LoginRefusalHalt.h"
#include "MessageSerializer.h"
#include "SessionPollingNetworkLoop.h"

using ::testing::StrictMock;

constexpr unsigned long LoginTimeoutMilliseconds = 5000;

static LoginResult AFreshToken(const std::string &token)
{
    return LoginResult{LoginStatus::Success, token};
}

static const LoginResult ALoginThatFailed{LoginStatus::Failed, ""};
static const LoginResult ALoginWithoutToken{LoginStatus::Success, ""};
static const LoginResult ARefusedLogin{LoginStatus::InvalidCredentials, ""};

static std::string AuthWith(const std::string &token)
{
    return "{\"type\":\"auth\",\"token\":\"" + token + "\"}";
}

static std::string AuthRefreshWith(const std::string &token)
{
    return "{\"type\":\"auth_refresh\",\"token\":\"" + token + "\"}";
}

class TokenSessionMessageSenderTest : public ::testing::Test
{
protected:
    StrictMock<MockPayloadSocketWriter> mockPayloadSocketWriter;
    FakeAsyncLogin login;
    FakeClock clock;
    StrictMock<MockLoginRefusalListener> mockLoginRefusalListener;
    TokenSessionMessageSender sender{mockPayloadSocketWriter, MessageSerializer{}, login, clock, LoginTimeoutMilliseconds, mockLoginRefusalListener};
};

// ---- Login d'ouverture (US-FW-005 CA-9, CA-12) ----

TEST_F(TokenSessionMessageSenderTest, StartsALoginWithoutSendingAnythingWhenAConnectionOpens)
{
    sender.SendAuth();

    EXPECT_EQ(login.starts, 1);
    EXPECT_TRUE(login.IsInFlight());
}

TEST_F(TokenSessionMessageSenderTest, SendsNothingWhileTheLoginIsInFlight)
{
    sender.SendAuth();

    EXPECT_FALSE(sender.PollLogin());
    clock.Advance(LoginTimeoutMilliseconds - 1);
    EXPECT_FALSE(sender.PollLogin());
}

TEST_F(TokenSessionMessageSenderTest, SendsAuthCarryingAFreshlyFetchedToken)
{
    sender.UpdateToken("startup-token");
    sender.SendAuth();
    login.Finish(AFreshToken("fresh-token"));

    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("fresh-token"))).Times(1);

    EXPECT_FALSE(sender.PollLogin());
}

TEST_F(TokenSessionMessageSenderTest, SendsAuthOnlyOnceWhenTheLoginEnded)
{
    sender.SendAuth();
    login.Finish(AFreshToken("fresh-token"));

    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("fresh-token"))).Times(1);

    sender.PollLogin();
    sender.PollLogin();
}

TEST_F(TokenSessionMessageSenderTest, FetchesAFreshTokenOnEveryAuth)
{
    sender.SendAuth();
    login.Finish(AFreshToken("first-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("first-token"))).Times(1);
    sender.PollLogin();

    sender.OnConnectionClosed();
    sender.SendAuth();
    login.Finish(AFreshToken("second-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("second-token"))).Times(1);
    sender.PollLogin();

    EXPECT_EQ(login.starts, 2);
}

TEST_F(TokenSessionMessageSenderTest, SendsAuthCarryingTheLastKnownTokenWhenTheFetchFails)
{
    sender.UpdateToken("any-token");
    sender.SendAuth();
    login.Finish(ALoginThatFailed);

    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("any-token"))).Times(1);

    sender.PollLogin();
}

TEST_F(TokenSessionMessageSenderTest, SendsAuthCarryingTheLastKnownTokenWhenTheFetchReturnsNoToken)
{
    sender.UpdateToken("any-token");
    sender.SendAuth();
    login.Finish(ALoginWithoutToken);

    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("any-token"))).Times(1);

    sender.PollLogin();
}

TEST_F(TokenSessionMessageSenderTest, SendsNoAuthAndReportsTheRefusalWhenTheLoginIsRefused)
{
    sender.UpdateToken("any-token");
    sender.SendAuth();
    login.Finish(ARefusedLogin);

    EXPECT_CALL(mockLoginRefusalListener, OnLoginRefused()).Times(1);

    sender.PollLogin();
}

TEST_F(TokenSessionMessageSenderTest, SendsAuthCarryingTheLastKnownTokenWhenTheLoginExceedsFiveSeconds)
{
    sender.UpdateToken("any-token");
    sender.SendAuth();
    clock.Advance(LoginTimeoutMilliseconds - 1);
    EXPECT_FALSE(sender.PollLogin());
    clock.Advance(1);

    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("any-token"))).Times(1);

    sender.PollLogin();
}

TEST_F(TokenSessionMessageSenderTest, AbandonsTheLoginThatExceededFiveSeconds)
{
    sender.SendAuth();
    clock.Advance(LoginTimeoutMilliseconds);
    EXPECT_CALL(mockPayloadSocketWriter, Write(::testing::_)).Times(1);

    sender.PollLogin();

    EXPECT_EQ(login.abandons, 1);
    EXPECT_FALSE(login.IsInFlight());
}

TEST_F(TokenSessionMessageSenderTest, MeasuresTheFiveSecondsFromTheStartOfTheLogin)
{
    clock.Advance(12345);
    sender.SendAuth();
    clock.Advance(LoginTimeoutMilliseconds - 1);

    EXPECT_FALSE(sender.PollLogin());
    EXPECT_EQ(login.abandons, 0);
}

TEST_F(TokenSessionMessageSenderTest, MeasuresTheFiveSecondsAcrossTheOverflowOfTheClock)
{
    clock.SetNow(ULONG_MAX - 1000);
    sender.SendAuth();
    clock.Advance(500);
    EXPECT_FALSE(sender.PollLogin());
    EXPECT_EQ(login.abandons, 0);
    clock.Advance(LoginTimeoutMilliseconds - 501);

    EXPECT_FALSE(sender.PollLogin());
    EXPECT_EQ(login.abandons, 0);

    clock.Advance(1);
    EXPECT_CALL(mockPayloadSocketWriter, Write(::testing::_)).Times(1);

    sender.PollLogin();
}

TEST_F(TokenSessionMessageSenderTest, SendsNothingWhenTheLoginEndsAfterTheConnectionClosed)
{
    sender.UpdateToken("any-token");
    sender.SendAuth();
    sender.OnConnectionClosed();
    login.Finish(AFreshToken("late-token"));

    EXPECT_FALSE(sender.PollLogin());
}

TEST_F(TokenSessionMessageSenderTest, SendsNothingWhenTheLoginFailsAfterTheConnectionClosed)
{
    sender.SendAuth();
    sender.OnConnectionClosed();
    login.Finish(ALoginThatFailed);

    EXPECT_FALSE(sender.PollLogin());
}

TEST_F(TokenSessionMessageSenderTest, SendsNothingWhenTheLoginTimesOutAfterTheConnectionClosed)
{
    sender.SendAuth();
    sender.OnConnectionClosed();
    clock.Advance(LoginTimeoutMilliseconds);

    EXPECT_FALSE(sender.PollLogin());
    EXPECT_EQ(login.abandons, 1);
}

TEST_F(TokenSessionMessageSenderTest, KeepsTheFreshTokenOfALoginEndedAfterTheConnectionClosedAsTheLastKnownToken)
{
    sender.UpdateToken("startup-token");
    sender.SendAuth();
    sender.OnConnectionClosed();
    login.Finish(AFreshToken("late-token"));
    sender.PollLogin();

    sender.SendAuth();
    login.Finish(ALoginThatFailed);

    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("late-token"))).Times(1);

    sender.PollLogin();
}

TEST_F(TokenSessionMessageSenderTest, IgnoresTheRefusalOfALoginEndedAfterTheConnectionClosed)
{
    sender.SendAuth();
    sender.OnConnectionClosed();
    login.Finish(ARefusedLogin);

    EXPECT_CALL(mockLoginRefusalListener, OnLoginRefused()).Times(0);

    EXPECT_FALSE(sender.PollLogin());
}

TEST_F(TokenSessionMessageSenderTest, AbandonsTheLoginInFlightWhenANewConnectionOpens)
{
    sender.SendAuth();
    sender.OnConnectionClosed();
    sender.SendAuth();

    EXPECT_EQ(login.abandons, 1);
    EXPECT_EQ(login.starts, 2);
    EXPECT_TRUE(login.IsInFlight());
}

TEST_F(TokenSessionMessageSenderTest, KeepsNoTokenFromTheLoginAbandonedByANewConnection)
{
    sender.UpdateToken("startup-token");
    sender.SendAuth();
    login.Finish(AFreshToken("abandoned-token"));
    sender.OnConnectionClosed();
    sender.SendAuth();
    login.Finish(ALoginThatFailed);

    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("startup-token"))).Times(1);

    sender.PollLogin();
}

TEST_F(TokenSessionMessageSenderTest, AbandonsTheLoginInFlightWhenTheBuzzerIsEliminated)
{
    sender.UpdateToken("startup-token");
    sender.SendAuth();
    sender.OnConnectionClosed();

    sender.Abandon();
    login.Finish(AFreshToken("abandoned-token"));

    EXPECT_EQ(login.abandons, 1);
    EXPECT_FALSE(sender.PollLogin());
}

TEST_F(TokenSessionMessageSenderTest, KeepsNoTokenFromTheLoginAbandonedByTheElimination)
{
    sender.UpdateToken("startup-token");
    sender.SendAuth();
    login.Finish(AFreshToken("abandoned-token"));
    sender.OnConnectionClosed();
    sender.Abandon();

    sender.SendAuth();
    login.Finish(ALoginThatFailed);

    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("startup-token"))).Times(1);

    sender.PollLogin();
}

TEST_F(TokenSessionMessageSenderTest, NeverPollsAgainTheLoginAbandonedByTheElimination)
{
    sender.SendAuth();
    sender.OnConnectionClosed();
    sender.Abandon();

    clock.Advance(LoginTimeoutMilliseconds);

    EXPECT_FALSE(sender.PollLogin());
    EXPECT_EQ(login.abandons, 1);
}

TEST_F(TokenSessionMessageSenderTest, PollsNothingWhenNoLoginIsInFlight)
{
    clock.Advance(2 * LoginTimeoutMilliseconds);

    EXPECT_FALSE(sender.PollLogin());
    EXPECT_EQ(login.abandons, 0);
}

TEST_F(TokenSessionMessageSenderTest, AbandonsNothingWhenNoLoginIsInFlight)
{
    sender.Abandon();

    EXPECT_EQ(login.abandons, 0);
}

// ---- Login de renouvellement (US-FW-007 CA-2, CA-3, CA-4, CA-5, CA-8, CA-9) ----

TEST_F(TokenSessionMessageSenderTest, StartsALoginWithoutSendingAnythingWhenTheHubAnnouncesTheExpiry)
{
    sender.SendAuth();
    login.Finish(AFreshToken("opening-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("opening-token"))).Times(1);
    sender.PollLogin();

    sender.SendAuthRefresh();

    EXPECT_EQ(login.starts, 2);
    EXPECT_TRUE(login.IsInFlight());
    EXPECT_FALSE(sender.PollLogin());
}

TEST_F(TokenSessionMessageSenderTest, SendsAuthRefreshCarryingAFreshlyFetchedTokenNeverTheOldOne)
{
    sender.UpdateToken("old-token");
    sender.SendAuth();
    login.Finish(AFreshToken("old-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("old-token"))).Times(1);
    sender.PollLogin();

    sender.SendAuthRefresh();
    login.Finish(AFreshToken("fresh-token"));

    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthRefreshWith("fresh-token"))).Times(1);

    sender.PollLogin();
}

TEST_F(TokenSessionMessageSenderTest, ReportsTheAuthRefreshAsSentOnceItCarriesAFreshToken)
{
    sender.SendAuth();
    login.Finish(AFreshToken("opening-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(::testing::_)).Times(2);
    sender.PollLogin();

    sender.SendAuthRefresh();
    login.Finish(AFreshToken("fresh-token"));

    EXPECT_TRUE(sender.PollLogin());
    EXPECT_FALSE(sender.PollLogin());
}

TEST_F(TokenSessionMessageSenderTest, SendsNoAuthRefreshWhenTheRenewalLoginFails)
{
    sender.SendAuth();
    login.Finish(AFreshToken("opening-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("opening-token"))).Times(1);
    sender.PollLogin();

    sender.SendAuthRefresh();
    login.Finish(ALoginThatFailed);

    EXPECT_FALSE(sender.PollLogin());
}

TEST_F(TokenSessionMessageSenderTest, SendsNoAuthRefreshWhenTheRenewalLoginReturnsNoToken)
{
    sender.SendAuth();
    login.Finish(AFreshToken("opening-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("opening-token"))).Times(1);
    sender.PollLogin();

    sender.SendAuthRefresh();
    login.Finish(ALoginWithoutToken);

    EXPECT_FALSE(sender.PollLogin());
}

TEST_F(TokenSessionMessageSenderTest, SendsNoAuthRefreshAndStopsNothingWhenTheRenewalLoginIsRefused)
{
    sender.SendAuth();
    login.Finish(AFreshToken("opening-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("opening-token"))).Times(1);
    sender.PollLogin();

    sender.SendAuthRefresh();
    login.Finish(ARefusedLogin);

    EXPECT_CALL(mockLoginRefusalListener, OnLoginRefused()).Times(0);

    EXPECT_FALSE(sender.PollLogin());
}

TEST_F(TokenSessionMessageSenderTest, SendsNoAuthRefreshAndAbandonsTheRenewalLoginThatExceedsFiveSeconds)
{
    sender.SendAuth();
    login.Finish(AFreshToken("opening-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("opening-token"))).Times(1);
    sender.PollLogin();

    sender.SendAuthRefresh();
    clock.Advance(LoginTimeoutMilliseconds - 1);
    EXPECT_FALSE(sender.PollLogin());
    clock.Advance(1);

    EXPECT_FALSE(sender.PollLogin());
    EXPECT_EQ(login.abandons, 1);
    EXPECT_FALSE(login.IsInFlight());
}

TEST_F(TokenSessionMessageSenderTest, SendsAuthCarryingTheRenewedTokenWhenTheFetchFailsAfterARenewal)
{
    sender.UpdateToken("startup-token");
    sender.SendAuth();
    login.Finish(AFreshToken("opening-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("opening-token"))).Times(1);
    sender.PollLogin();
    sender.SendAuthRefresh();
    login.Finish(AFreshToken("renewed-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthRefreshWith("renewed-token"))).Times(1);
    sender.PollLogin();

    sender.OnConnectionClosed();
    sender.SendAuth();
    login.Finish(ALoginThatFailed);

    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("renewed-token"))).Times(1);

    sender.PollLogin();
}

TEST_F(TokenSessionMessageSenderTest, IgnoresTheAnnouncementOfTheExpiryWhileTheOpeningLoginIsInFlight)
{
    sender.UpdateToken("startup-token");
    sender.SendAuth();

    sender.SendAuthRefresh();

    EXPECT_EQ(login.starts, 1);
    login.Finish(AFreshToken("opening-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("opening-token"))).Times(1);
    EXPECT_FALSE(sender.PollLogin());
    EXPECT_FALSE(sender.PollLogin());
}

TEST_F(TokenSessionMessageSenderTest, IgnoresTheAnnouncementOfTheExpiryWhileARenewalLoginIsInFlight)
{
    sender.SendAuth();
    login.Finish(AFreshToken("opening-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("opening-token"))).Times(1);
    sender.PollLogin();

    sender.SendAuthRefresh();
    sender.SendAuthRefresh();
    sender.SendAuthRefresh();

    EXPECT_EQ(login.starts, 2);
    EXPECT_EQ(login.abandons, 0);
    login.Finish(AFreshToken("renewed-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthRefreshWith("renewed-token"))).Times(1);
    sender.PollLogin();
}

TEST_F(TokenSessionMessageSenderTest, RenewsAgainWhenTheAnnouncementArrivesAfterASuccessfulRenewalLogin)
{
    sender.SendAuth();
    login.Finish(AFreshToken("opening-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("opening-token"))).Times(1);
    sender.PollLogin();
    sender.SendAuthRefresh();
    login.Finish(AFreshToken("renewed-1"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthRefreshWith("renewed-1"))).Times(1);
    sender.PollLogin();

    sender.SendAuthRefresh();
    login.Finish(AFreshToken("renewed-2"));

    EXPECT_EQ(login.starts, 3);
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthRefreshWith("renewed-2"))).Times(1);
    sender.PollLogin();
}

TEST_F(TokenSessionMessageSenderTest, RenewsAgainWhenTheAnnouncementArrivesAfterAFailedRenewalLogin)
{
    sender.SendAuth();
    login.Finish(AFreshToken("opening-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("opening-token"))).Times(1);
    sender.PollLogin();
    sender.SendAuthRefresh();
    login.Finish(ALoginThatFailed);
    sender.PollLogin();

    sender.SendAuthRefresh();

    EXPECT_EQ(login.starts, 3);
    EXPECT_TRUE(login.IsInFlight());
}

TEST_F(TokenSessionMessageSenderTest, SendsNothingWhenTheRenewalLoginEndsAfterTheConnectionClosed)
{
    sender.SendAuth();
    login.Finish(AFreshToken("opening-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("opening-token"))).Times(1);
    sender.PollLogin();
    sender.SendAuthRefresh();
    sender.OnConnectionClosed();
    login.Finish(AFreshToken("late-token"));

    EXPECT_FALSE(sender.PollLogin());
}

TEST_F(TokenSessionMessageSenderTest, KeepsTheFreshTokenOfARenewalLoginEndedAfterTheConnectionClosedAsTheLastKnownToken)
{
    sender.SendAuth();
    login.Finish(AFreshToken("opening-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("opening-token"))).Times(1);
    sender.PollLogin();
    sender.SendAuthRefresh();
    sender.OnConnectionClosed();
    login.Finish(AFreshToken("late-token"));
    sender.PollLogin();

    sender.SendAuth();
    login.Finish(ALoginThatFailed);

    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("late-token"))).Times(1);

    sender.PollLogin();
}

TEST_F(TokenSessionMessageSenderTest, IgnoresTheRefusalOfARenewalLoginEndedAfterTheConnectionClosed)
{
    sender.SendAuth();
    login.Finish(AFreshToken("opening-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("opening-token"))).Times(1);
    sender.PollLogin();
    sender.SendAuthRefresh();
    sender.OnConnectionClosed();
    login.Finish(ARefusedLogin);

    EXPECT_CALL(mockLoginRefusalListener, OnLoginRefused()).Times(0);

    EXPECT_FALSE(sender.PollLogin());
}

TEST_F(TokenSessionMessageSenderTest, AbandonsTheRenewalLoginInFlightWhenANewConnectionOpensAndStartsAFreshLogin)
{
    sender.SendAuth();
    login.Finish(AFreshToken("opening-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("opening-token"))).Times(1);
    sender.PollLogin();
    sender.SendAuthRefresh();
    login.Finish(AFreshToken("abandoned-token"));
    sender.OnConnectionClosed();

    sender.SendAuth();
    login.Finish(ALoginThatFailed);

    EXPECT_EQ(login.abandons, 1);
    EXPECT_EQ(login.starts, 3);
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("opening-token"))).Times(1);
    sender.PollLogin();
}

TEST_F(TokenSessionMessageSenderTest, AbandonsTheRenewalLoginInFlightWhenTheBuzzerIsEliminated)
{
    sender.SendAuth();
    login.Finish(AFreshToken("opening-token"));
    EXPECT_CALL(mockPayloadSocketWriter, Write(AuthWith("opening-token"))).Times(1);
    sender.PollLogin();
    sender.SendAuthRefresh();
    sender.OnConnectionClosed();

    sender.Abandon();
    login.Finish(AFreshToken("abandoned-token"));

    EXPECT_EQ(login.abandons, 1);
    EXPECT_FALSE(sender.PollLogin());
}

TEST(LoginRefusalHaltTest, NetworkIsNotHaltedBeforeAnyRefusal)
{
    ::testing::StrictMock<MockScreen> mockScreen;
    LoginOutcomeDisplay loginOutcomeDisplay{mockScreen};
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    LoginRefusalHalt halt{loginOutcomeDisplay, networkHalt};

    EXPECT_FALSE(networkHalt.IsHalted());
}

TEST(LoginRefusalHaltTest, HaltsTheNetworkAndShowsTheRefusalScreenWhenTheLoginIsRefused)
{
    MockScreen mockScreen;
    LoginOutcomeDisplay loginOutcomeDisplay{mockScreen};
    ::testing::NiceMock<MockConnectionCloser> mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    LoginRefusalHalt halt{loginOutcomeDisplay, networkHalt};

    EXPECT_CALL(mockScreen, Display(ScreenPresenter{}.TopBarForInit(), ScreenModel{ScreenPresenter{}.ScreenForLoginRefused()})).Times(1);

    halt.OnLoginRefused();

    EXPECT_TRUE(networkHalt.IsHalted());
}

TEST(LoginRefusalHaltTest, ClosesTheConnectionWhenTheLoginIsRefused)
{
    ::testing::NiceMock<MockScreen> mockScreen;
    LoginOutcomeDisplay loginOutcomeDisplay{mockScreen};
    MockConnectionCloser mockConnectionCloser;
    NetworkHalt networkHalt{mockConnectionCloser};
    LoginRefusalHalt halt{loginOutcomeDisplay, networkHalt};

    EXPECT_CALL(mockConnectionCloser, Close()).Times(1);

    halt.OnLoginRefused();
}

class MockNetworkLoop : public NetworkLoop
{
public:
    MOCK_METHOD(void, Loop, (), (override));
};

class MockSessionPoller : public SessionPoller
{
public:
    MOCK_METHOD(void, PollSession, (), (override));
};

TEST(SessionPollingNetworkLoopTest, RunsTheNetworkThenPollsTheSessionOnEveryTurn)
{
    MockNetworkLoop mockNetworkLoop;
    MockSessionPoller mockSessionPoller;
    SessionPollingNetworkLoop loop{mockNetworkLoop, mockSessionPoller};
    ::testing::InSequence inSequence;

    EXPECT_CALL(mockNetworkLoop, Loop()).Times(1);
    EXPECT_CALL(mockSessionPoller, PollSession()).Times(1);
    EXPECT_CALL(mockNetworkLoop, Loop()).Times(1);
    EXPECT_CALL(mockSessionPoller, PollSession()).Times(1);

    loop.Loop();
    loop.Loop();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
