#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../mocks/ScreenMocks.h"
#include "../mocks/StartupMocks.h"
#include "LoginOutcomeDisplay.h"
#include "TokenAcquisition.h"

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

TEST(LoginOutcomeDisplayTest, ShowsTheRefusalScreenWithoutTopBarWhenTheCredentialsAreRefused)
{
    MockScreen mockScreen;
    LoginOutcomeDisplay loginOutcomeDisplay{mockScreen};

    EXPECT_CALL(mockScreen, Display(TopBar::Hidden(), ScreenModel{ScreenPresenter{}.ScreenForLoginRefused()})).Times(1);

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

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
