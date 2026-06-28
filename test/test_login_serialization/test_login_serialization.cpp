#include <gtest/gtest.h>

#include "LoginRequestSerializer.h"
#include "ServerCredentials.h"

TEST(LoginSerializationTest, SerializesCredentialsIntoLoginRequestBody)
{
    LoginRequestSerializer serializer;
    EXPECT_EQ(serializer.SerializeLogin(ServerCredentials{"buzzer01", "secret"}), "{\"username\":\"buzzer01\",\"password\":\"secret\"}");
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}