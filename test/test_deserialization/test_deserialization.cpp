#include <gtest/gtest.h>
#include "MessageDeserializer.h"

TEST(MessageDeserializerTest, QuestionOpenMessageIsTypeQuestionOpen)
{
    MessageDeserializer deserializer;
    EXPECT_EQ(deserializer.ExtractType("{\"type\":\"question_open\"}"), "question_open");
}

TEST(MessageDeserializerTest, QuestionChoicesMessageIsTypeQuestionChoices)
{
    MessageDeserializer deserializer;
    EXPECT_EQ(deserializer.ExtractType("{\"type\":\"question_choices\"}"), "question_choices");
}

TEST(MessageDeserializerTest, ExtractTokenReturnsTokenFromAuthSuccessMessage)
{
    MessageDeserializer deserializer;
    EXPECT_EQ(deserializer.ExtractToken("{\"type\":\"auth_success\",\"token\":\"any-token\"}"), "any-token");
}

TEST(MessageDeserializerTest, ExtractTokenReturnsEmptyStringFromAuthSuccessMessageWithNoToken)
{
    MessageDeserializer deserializer;
    EXPECT_EQ(deserializer.ExtractToken("{\"type\":\"auth_success\"}"), "");
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}