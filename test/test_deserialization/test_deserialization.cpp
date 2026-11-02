#include <gtest/gtest.h>
#include "MessageDeserializer.h"

TEST(MessageDeserializerTest, ParsedQuestionOpenMessageIsTypeQuestionOpen)
{
    MessageDeserializer deserializer;
    EXPECT_EQ(deserializer.Parse("{\"type\":\"question_open\"}").Type(), "question_open");
}

TEST(MessageDeserializerTest, ParsedQuestionChoicesMessageIsTypeQuestionChoices)
{
    MessageDeserializer deserializer;
    EXPECT_EQ(deserializer.Parse("{\"type\":\"question_choices\"}").Type(), "question_choices");
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}