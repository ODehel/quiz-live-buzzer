#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../mocks/ConnectionMocks.h"
#include "../mocks/ScreenMocks.h"
#include "../mocks/SessionMocks.h"
#include "../mocks/StartupMocks.h"
#include "TokenSessionMessageSender.h"
#include "LoginRefusalHalt.h"
#include "MessageSerializer.h"

using ::testing::Return;
using ::testing::StrictMock;

TEST(TokenSessionMessageSenderTest, SendsAuthRefreshCarryingAFreshlyFetchedTokenNeverTheOldOne)
{
    MockPayloadSocketWriter mockPayloadSocketWriter;
    MockTokenFetcher mockTokenFetcher;
    StrictMock<MockLoginRefusalListener> mockLoginRefusalListener;
    MessageSerializer messageSerializer;
    TokenSessionMessageSender sender{mockPayloadSocketWriter, messageSerializer, mockTokenFetcher, mockLoginRefusalListener};

    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(Return(LoginResult{LoginStatus::Success, "fresh-token"}));
    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth_refresh\",\"token\":\"fresh-token\"}")).Times(1);

    sender.UpdateToken("old-token");
    sender.SendAuthRefresh();
}

TEST(TokenSessionMessageSenderTest, ReportsTheAuthRefreshAsSentOnceItCarriesAFreshToken)
{
    ::testing::NiceMock<MockPayloadSocketWriter> mockPayloadSocketWriter;
    MockTokenFetcher mockTokenFetcher;
    StrictMock<MockLoginRefusalListener> mockLoginRefusalListener;
    MessageSerializer messageSerializer;
    TokenSessionMessageSender sender{mockPayloadSocketWriter, messageSerializer, mockTokenFetcher, mockLoginRefusalListener};

    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(Return(LoginResult{LoginStatus::Success, "fresh-token"}));

    EXPECT_TRUE(sender.SendAuthRefresh());
}

TEST(TokenSessionMessageSenderTest, SendsNoAuthRefreshWhenTheRenewalLoginFails)
{
    MockPayloadSocketWriter mockPayloadSocketWriter;
    MockTokenFetcher mockTokenFetcher;
    StrictMock<MockLoginRefusalListener> mockLoginRefusalListener;
    MessageSerializer messageSerializer;
    TokenSessionMessageSender sender{mockPayloadSocketWriter, messageSerializer, mockTokenFetcher, mockLoginRefusalListener};

    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(Return(LoginResult{LoginStatus::Failed, ""}));
    EXPECT_CALL(mockPayloadSocketWriter, Write(::testing::_)).Times(0);

    sender.UpdateToken("old-token");
    EXPECT_FALSE(sender.SendAuthRefresh());
}

TEST(TokenSessionMessageSenderTest, SendsNoAuthRefreshWhenTheRenewalLoginReturnsNoToken)
{
    MockPayloadSocketWriter mockPayloadSocketWriter;
    MockTokenFetcher mockTokenFetcher;
    StrictMock<MockLoginRefusalListener> mockLoginRefusalListener;
    MessageSerializer messageSerializer;
    TokenSessionMessageSender sender{mockPayloadSocketWriter, messageSerializer, mockTokenFetcher, mockLoginRefusalListener};

    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(Return(LoginResult{LoginStatus::Success, ""}));
    EXPECT_CALL(mockPayloadSocketWriter, Write(::testing::_)).Times(0);

    sender.UpdateToken("old-token");
    EXPECT_FALSE(sender.SendAuthRefresh());
}

TEST(TokenSessionMessageSenderTest, SendsNoAuthRefreshAndStopsNothingWhenTheRenewalLoginIsRefused)
{
    MockPayloadSocketWriter mockPayloadSocketWriter;
    MockTokenFetcher mockTokenFetcher;
    StrictMock<MockLoginRefusalListener> mockLoginRefusalListener;
    MessageSerializer messageSerializer;
    TokenSessionMessageSender sender{mockPayloadSocketWriter, messageSerializer, mockTokenFetcher, mockLoginRefusalListener};

    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(Return(LoginResult{LoginStatus::InvalidCredentials, ""}));
    EXPECT_CALL(mockPayloadSocketWriter, Write(::testing::_)).Times(0);

    sender.UpdateToken("old-token");
    EXPECT_FALSE(sender.SendAuthRefresh());
}

TEST(TokenSessionMessageSenderTest, SendsAuthCarryingTheRenewedTokenWhenTheFetchFailsAfterARenewal)
{
    MockPayloadSocketWriter mockPayloadSocketWriter;
    MockTokenFetcher mockTokenFetcher;
    StrictMock<MockLoginRefusalListener> mockLoginRefusalListener;
    MessageSerializer messageSerializer;
    TokenSessionMessageSender sender{mockPayloadSocketWriter, messageSerializer, mockTokenFetcher, mockLoginRefusalListener};

    EXPECT_CALL(mockTokenFetcher, Fetch())
        .WillOnce(Return(LoginResult{LoginStatus::Success, "renewed-token"}))
        .WillOnce(Return(LoginResult{LoginStatus::Failed, ""}));
    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth_refresh\",\"token\":\"renewed-token\"}")).Times(1);
    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth\",\"token\":\"renewed-token\"}")).Times(1);

    sender.UpdateToken("startup-token");
    sender.SendAuthRefresh();
    sender.SendAuth();
}

TEST(TokenSessionMessageSenderTest, SendsAuthCarryingAFreshlyFetchedToken)
{
    MockPayloadSocketWriter mockPayloadSocketWriter;
    MockTokenFetcher mockTokenFetcher;
    StrictMock<MockLoginRefusalListener> mockLoginRefusalListener;
    MessageSerializer messageSerializer;
    TokenSessionMessageSender sender{mockPayloadSocketWriter, messageSerializer, mockTokenFetcher, mockLoginRefusalListener};

    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(Return(LoginResult{LoginStatus::Success, "fresh-token"}));
    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth\",\"token\":\"fresh-token\"}")).Times(1);

    sender.UpdateToken("startup-token");
    sender.SendAuth();
}

TEST(TokenSessionMessageSenderTest, FetchesAFreshTokenOnEveryAuth)
{
    MockPayloadSocketWriter mockPayloadSocketWriter;
    MockTokenFetcher mockTokenFetcher;
    StrictMock<MockLoginRefusalListener> mockLoginRefusalListener;
    MessageSerializer messageSerializer;
    TokenSessionMessageSender sender{mockPayloadSocketWriter, messageSerializer, mockTokenFetcher, mockLoginRefusalListener};

    EXPECT_CALL(mockTokenFetcher, Fetch())
        .WillOnce(Return(LoginResult{LoginStatus::Success, "first-token"}))
        .WillOnce(Return(LoginResult{LoginStatus::Success, "second-token"}));
    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth\",\"token\":\"first-token\"}")).Times(1);
    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth\",\"token\":\"second-token\"}")).Times(1);

    sender.SendAuth();
    sender.SendAuth();
}

TEST(TokenSessionMessageSenderTest, SendsAuthCarryingTheLastKnownTokenWhenTheFetchFails)
{
    MockPayloadSocketWriter mockPayloadSocketWriter;
    MockTokenFetcher mockTokenFetcher;
    StrictMock<MockLoginRefusalListener> mockLoginRefusalListener;
    MessageSerializer messageSerializer;
    TokenSessionMessageSender sender{mockPayloadSocketWriter, messageSerializer, mockTokenFetcher, mockLoginRefusalListener};

    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(Return(LoginResult{LoginStatus::Failed, ""}));
    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth\",\"token\":\"any-token\"}")).Times(1);

    sender.UpdateToken("any-token");
    sender.SendAuth();
}

TEST(TokenSessionMessageSenderTest, SendsAuthCarryingTheLastKnownTokenWhenTheFetchReturnsNoToken)
{
    MockPayloadSocketWriter mockPayloadSocketWriter;
    MockTokenFetcher mockTokenFetcher;
    StrictMock<MockLoginRefusalListener> mockLoginRefusalListener;
    MessageSerializer messageSerializer;
    TokenSessionMessageSender sender{mockPayloadSocketWriter, messageSerializer, mockTokenFetcher, mockLoginRefusalListener};

    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(Return(LoginResult{LoginStatus::Success, ""}));
    EXPECT_CALL(mockPayloadSocketWriter, Write("{\"type\":\"auth\",\"token\":\"any-token\"}")).Times(1);

    sender.UpdateToken("any-token");
    sender.SendAuth();
}

TEST(TokenSessionMessageSenderTest, SendsNoAuthAndReportsTheRefusalWhenTheLoginIsRefused)
{
    MockPayloadSocketWriter mockPayloadSocketWriter;
    MockTokenFetcher mockTokenFetcher;
    MockLoginRefusalListener mockLoginRefusalListener;
    MessageSerializer messageSerializer;
    TokenSessionMessageSender sender{mockPayloadSocketWriter, messageSerializer, mockTokenFetcher, mockLoginRefusalListener};

    EXPECT_CALL(mockTokenFetcher, Fetch()).WillOnce(Return(LoginResult{LoginStatus::InvalidCredentials, ""}));
    EXPECT_CALL(mockPayloadSocketWriter, Write(::testing::_)).Times(0);
    EXPECT_CALL(mockLoginRefusalListener, OnLoginRefused()).Times(1);

    sender.UpdateToken("any-token");
    sender.SendAuth();
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

    EXPECT_CALL(mockScreen, Display(TopBar::Hidden(), ScreenModel{ScreenPresenter{}.ScreenForLoginRefused()})).Times(1);

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

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
