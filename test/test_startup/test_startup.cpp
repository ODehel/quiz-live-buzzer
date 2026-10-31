#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../mocks/StartupMocks.h"
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

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
