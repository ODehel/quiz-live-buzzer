#include <gtest/gtest.h>

#include "LoginRequestSerializer.h"
#include "ServerCredentials.h"

TEST(LoginSerializationTest, SerializesCredentialsIntoLoginRequestBody)
{
    LoginRequestSerializer serializer;
    EXPECT_EQ(serializer.SerializeLogin(ServerCredentials{"buzzer01", "secret"}), "{\"username\":\"buzzer01\",\"password\":\"secret\"}");
}

TEST(LoginSerializationTest, EscapesQuotesInCredentials)
{
    LoginRequestSerializer serializer;
    EXPECT_EQ(serializer.SerializeLogin(ServerCredentials{"bu\"zzer", "pa\"ss"}), "{\"username\":\"bu\\\"zzer\",\"password\":\"pa\\\"ss\"}");
}

TEST(LoginSerializationTest, EscapesBackslashesInCredentials)
{
    LoginRequestSerializer serializer;
    EXPECT_EQ(serializer.SerializeLogin(ServerCredentials{"bu\\zzer", "pa\\ss"}), "{\"username\":\"bu\\\\zzer\",\"password\":\"pa\\\\ss\"}");
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}