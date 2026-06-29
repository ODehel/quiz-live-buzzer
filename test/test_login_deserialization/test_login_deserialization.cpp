#include <gtest/gtest.h>

#include <string>
#include "LoginResponseDeserializer.h"

TEST(LoginDeserializationTest, DeserializesTokenFromLoginResponseBody)
{
    LoginResponseDeserializer deserializer;
    EXPECT_EQ(deserializer.DeserializeToken("{\"token\":\"any-token\",\"expires_in\":3600,\"token_type\":\"Bearer\"}"), "any-token");
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}