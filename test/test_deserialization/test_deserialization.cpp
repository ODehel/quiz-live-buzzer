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

TEST(MessageDeserializerTest, ParsedTimerTickHasItsRemainingSeconds)
{
    MessageDeserializer deserializer;

    HubMessage message = deserializer.Parse("{\"type\":\"timer_tick\",\"remaining_seconds\":25}");

    EXPECT_TRUE(message.HasRemainingSeconds());
    EXPECT_EQ(message.RemainingSeconds(), 25);
}

TEST(MessageDeserializerTest, ParsedTimerTickWithoutRemainingSecondsHasNone)
{
    MessageDeserializer deserializer;

    EXPECT_FALSE(deserializer.Parse("{\"type\":\"timer_tick\"}").HasRemainingSeconds());
}

TEST(MessageDeserializerTest, ParsedTimerTickWithATextRemainingSecondsHasNone)
{
    MessageDeserializer deserializer;

    EXPECT_FALSE(deserializer.Parse("{\"type\":\"timer_tick\",\"remaining_seconds\":\"25\"}").HasRemainingSeconds());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}