#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <ostream>
#include <string>
#include "../mocks/StartupMocks.h"
#include "HttpTokenFetcher.h"
#include "TokenAcquisition.h"

using ::testing::Return;

const ServerCredentials AnyCredentials{"buzzer01", "secret"};
const char *const AnyHubUrl = "http://hub.local:3000";
const char *const BodyWithToken = "{\"token\":\"any-token\",\"expires_in\":3600,\"token_type\":\"Bearer\"}";

TEST(HttpTokenFetcherTest, PostsTheSerializedCredentialsToTheLoginRoute)
{
    MockHttpPoster mockHttpPoster;
    HttpTokenFetcher httpTokenFetcher{mockHttpPoster, AnyHubUrl, AnyCredentials};

    EXPECT_CALL(mockHttpPoster, PostJson("http://hub.local:3000/api/v1/token", "{\"username\":\"buzzer01\",\"password\":\"secret\"}"))
        .WillOnce(Return(HttpResponse{200, BodyWithToken}));

    httpTokenFetcher.Fetch();
}

TEST(HttpTokenFetcherTest, SucceedsWithTheTokenOfTheBodyWhenTheHubAnswers200)
{
    MockHttpPoster mockHttpPoster;
    HttpTokenFetcher httpTokenFetcher{mockHttpPoster, AnyHubUrl, AnyCredentials};
    EXPECT_CALL(mockHttpPoster, PostJson(::testing::_, ::testing::_)).WillOnce(Return(HttpResponse{200, BodyWithToken}));

    LoginResult result = httpTokenFetcher.Fetch();

    EXPECT_EQ(LoginStatus::Success, result.status);
    EXPECT_EQ("any-token", result.token);
}

TEST(HttpTokenFetcherTest, RefusesTheCredentialsWhenTheHubAnswers401)
{
    MockHttpPoster mockHttpPoster;
    HttpTokenFetcher httpTokenFetcher{mockHttpPoster, AnyHubUrl, AnyCredentials};
    EXPECT_CALL(mockHttpPoster, PostJson(::testing::_, ::testing::_)).WillOnce(Return(HttpResponse{401, ""}));

    LoginResult result = httpTokenFetcher.Fetch();

    EXPECT_EQ(LoginStatus::InvalidCredentials, result.status);
}

struct HttpStatus
{
    const char *name;
    int status;
};

std::ostream &operator<<(std::ostream &out, const HttpStatus &httpStatus)
{
    return out << httpStatus.name;
}

class LoginAnsweredWithAnyOtherStatusTest : public ::testing::TestWithParam<HttpStatus>
{
};

TEST_P(LoginAnsweredWithAnyOtherStatusTest, FailsEvenWhenTheBodyCarriesAToken)
{
    MockHttpPoster mockHttpPoster;
    HttpTokenFetcher httpTokenFetcher{mockHttpPoster, AnyHubUrl, AnyCredentials};
    EXPECT_CALL(mockHttpPoster, PostJson(::testing::_, ::testing::_)).WillOnce(Return(HttpResponse{GetParam().status, BodyWithToken}));

    LoginResult result = httpTokenFetcher.Fetch();

    EXPECT_EQ(LoginStatus::Failed, result.status);
}

INSTANTIATE_TEST_SUITE_P(
    HttpStatuses,
    LoginAnsweredWithAnyOtherStatusTest,
    ::testing::Values(
        HttpStatus{"BadRequest400", 400},
        HttpStatus{"TooManyRequests429", 429},
        HttpStatus{"InternalServerError500", 500},
        HttpStatus{"ServiceUnavailable503", 503},
        HttpStatus{"TransportErrorMinus1", -1},
        HttpStatus{"TransportErrorMinus11", -11},
        HttpStatus{"Created201", 201},
        HttpStatus{"NoContent204", 204},
        HttpStatus{"Found302", 302}),
    [](const ::testing::TestParamInfo<HttpStatus> &info)
    { return std::string(info.param.name); });

TEST(HttpTokenFetcherTest, SucceedsWithAnEmptyTokenWhenTheHubAnswers200WithoutToken)
{
    MockHttpPoster mockHttpPoster;
    HttpTokenFetcher httpTokenFetcher{mockHttpPoster, AnyHubUrl, AnyCredentials};
    EXPECT_CALL(mockHttpPoster, PostJson(::testing::_, ::testing::_)).WillOnce(Return(HttpResponse{200, "{}"}));

    LoginResult result = httpTokenFetcher.Fetch();

    EXPECT_EQ(LoginStatus::Success, result.status);
    EXPECT_EQ("", result.token);
}

TEST(HttpTokenFetcherTest, AcquisitionRetriesAfterAnAnswer200WithoutToken)
{
    MockHttpPoster mockHttpPoster;
    MockSleeper mockSleeper;
    HttpTokenFetcher httpTokenFetcher{mockHttpPoster, AnyHubUrl, AnyCredentials};
    TokenAcquisition tokenAcquisition{httpTokenFetcher, mockSleeper, 5000};

    EXPECT_CALL(mockHttpPoster, PostJson(::testing::_, ::testing::_))
        .WillOnce(Return(HttpResponse{200, "{}"}))
        .WillOnce(Return(HttpResponse{200, BodyWithToken}));
    EXPECT_CALL(mockSleeper, Sleep(5000)).Times(1);

    LoginResult result = tokenAcquisition.Acquire();

    EXPECT_EQ("any-token", result.token);
}

TEST(HttpTokenFetcherTest, AcquisitionRetriesAfterAnyOtherStatusUntilTheLoginSucceeds)
{
    MockHttpPoster mockHttpPoster;
    MockSleeper mockSleeper;
    HttpTokenFetcher httpTokenFetcher{mockHttpPoster, AnyHubUrl, AnyCredentials};
    TokenAcquisition tokenAcquisition{httpTokenFetcher, mockSleeper, 5000};

    EXPECT_CALL(mockHttpPoster, PostJson(::testing::_, ::testing::_))
        .WillOnce(Return(HttpResponse{-1, ""}))
        .WillOnce(Return(HttpResponse{503, ""}))
        .WillOnce(Return(HttpResponse{429, ""}))
        .WillOnce(Return(HttpResponse{200, BodyWithToken}));
    EXPECT_CALL(mockSleeper, Sleep(5000)).Times(3);

    LoginResult result = tokenAcquisition.Acquire();

    EXPECT_EQ(LoginStatus::Success, result.status);
    EXPECT_EQ("any-token", result.token);
}

TEST(HttpTokenFetcherTest, AcquisitionStopsWithoutRetryWhenTheHubAnswers401)
{
    MockHttpPoster mockHttpPoster;
    MockSleeper mockSleeper;
    HttpTokenFetcher httpTokenFetcher{mockHttpPoster, AnyHubUrl, AnyCredentials};
    TokenAcquisition tokenAcquisition{httpTokenFetcher, mockSleeper, 5000};

    EXPECT_CALL(mockHttpPoster, PostJson(::testing::_, ::testing::_)).WillOnce(Return(HttpResponse{401, ""}));
    EXPECT_CALL(mockSleeper, Sleep(::testing::_)).Times(0);

    LoginResult result = tokenAcquisition.Acquire();

    EXPECT_EQ(LoginStatus::InvalidCredentials, result.status);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
