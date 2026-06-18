#include <gtest/gtest.h>
#include "MessageSerializer.h"

TEST(MessageSerializerTest, BuzzMessageIsTypeBuzz)
{
    MessageSerializer serializer;
    EXPECT_EQ(serializer.SerializeBuzz(), "{\"type\":\"buzz\"}");
}

TEST(MessageSerializerTest, AnswerAMessageCarriesTheValue)
{
    MessageSerializer serializer;
    EXPECT_EQ(serializer.SerializeAnswer('A'), "{\"type\":\"answer\",\"value\":\"A\"}");
}

TEST(MessageSerializerTest, AnswerBMessageCarriesTheValue)
{
    MessageSerializer serializer;
    EXPECT_EQ(serializer.SerializeAnswer('B'), "{\"type\":\"answer\",\"value\":\"B\"}");
}

TEST(MessageSerializerTest, AuthRefreshMessageCarriesTheToken)
{
    MessageSerializer serializer;
    EXPECT_EQ(serializer.SerializeAuthRefresh("any-token"), "{\"type\":\"auth_refresh\",\"token\":\"any-token\"}");
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}