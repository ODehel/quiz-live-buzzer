#include <gtest/gtest.h>

#include <ostream>
#include <string>
#include "LoginResponseDeserializer.h"

TEST(LoginDeserializationTest, DeserializesTokenFromLoginResponseBody)
{
    LoginResponseDeserializer deserializer;
    EXPECT_EQ(deserializer.DeserializeToken("{\"token\":\"any-token\",\"expires_in\":3600,\"token_type\":\"Bearer\"}"), "any-token");
}

struct LoginBody
{
    const char *name;
    const char *body;
};

std::ostream &operator<<(std::ostream &out, const LoginBody &loginBody)
{
    return out << loginBody.name;
}

class LoginResponseWithoutUsableTokenTest : public ::testing::TestWithParam<LoginBody>
{
};

TEST_P(LoginResponseWithoutUsableTokenTest, YieldsNoToken)
{
    LoginResponseDeserializer deserializer;
    EXPECT_EQ(deserializer.DeserializeToken(GetParam().body), "");
}

INSTANTIATE_TEST_SUITE_P(
    LoginDeserializationTest,
    LoginResponseWithoutUsableTokenTest,
    ::testing::Values(
        LoginBody{"EmptyText", ""},
        LoginBody{"AnyText", "not json at all"},
        LoginBody{"EmptyArray", "[]"},
        LoginBody{"Null", "null"},
        LoginBody{"EmptyObject", "{}"},
        LoginBody{"TokenAbsent", "{\"expires_in\":3600}"},
        LoginBody{"TokenNull", "{\"token\":null}"},
        LoginBody{"TokenNumber", "{\"token\":42}"},
        LoginBody{"TokenBoolean", "{\"token\":true}"},
        LoginBody{"TokenEmptyText", "{\"token\":\"\"}"},
        LoginBody{"TruncatedInsideTheToken", "{\"token\":\"ab"}),
    [](const ::testing::TestParamInfo<LoginBody> &info)
    { return std::string(info.param.name); });

TEST(LoginDeserializationTest, ReadsTheTokenWhenTheBodyIsTruncatedAfterACompleteToken)
{
    LoginResponseDeserializer deserializer;
    EXPECT_EQ(deserializer.DeserializeToken("{\"token\":\"abc\""), "abc");
}

TEST(LoginDeserializationTest, ReadsTheTokenWhenTheBodyIsFollowedByStrayText)
{
    LoginResponseDeserializer deserializer;
    EXPECT_EQ(deserializer.DeserializeToken("{\"token\":\"abc\"} stray text"), "abc");
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}