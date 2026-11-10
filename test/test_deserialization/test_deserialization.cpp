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

TEST(MessageDeserializerTest, ParsedBuzzLockedHasItsBuzzerUsername)
{
    MessageDeserializer deserializer;

    HubMessage message = deserializer.Parse("{\"type\":\"buzz_locked\",\"buzzer_username\":\"Alice\"}");

    EXPECT_TRUE(message.HasBuzzerUsername());
    EXPECT_EQ(message.BuzzerUsername(), "Alice");
}

TEST(MessageDeserializerTest, ParsedBuzzLockedWithoutBuzzerUsernameHasNone)
{
    MessageDeserializer deserializer;

    EXPECT_FALSE(deserializer.Parse("{\"type\":\"buzz_locked\"}").HasBuzzerUsername());
}

TEST(MessageDeserializerTest, ParsedBuzzLockedWithANumericBuzzerUsernameHasNone)
{
    MessageDeserializer deserializer;

    EXPECT_FALSE(deserializer.Parse("{\"type\":\"buzz_locked\",\"buzzer_username\":42}").HasBuzzerUsername());
}

TEST(MessageDeserializerTest, ParsedAuthSuccessHasItsUsername)
{
    MessageDeserializer deserializer;

    HubMessage message = deserializer.Parse("{\"type\":\"auth_success\",\"role\":\"buzzer\",\"username\":\"quiz_buzzer_01\",\"expires_in\":3600}");

    EXPECT_EQ(message.Username(), "quiz_buzzer_01");
}

TEST(MessageDeserializerTest, ParsedAuthSuccessWithoutUsernameHasAnEmptyOne)
{
    MessageDeserializer deserializer;

    EXPECT_EQ(deserializer.Parse("{\"type\":\"auth_success\"}").Username(), "");
}

TEST(MessageDeserializerTest, ParsedAuthSuccessWithANumericUsernameHasAnEmptyOne)
{
    MessageDeserializer deserializer;

    EXPECT_EQ(deserializer.Parse("{\"type\":\"auth_success\",\"username\":42}").Username(), "");
}

TEST(MessageDeserializerTest, ParsedQuestionOpenHasItsTitle)
{
    MessageDeserializer deserializer;

    HubMessage message = deserializer.Parse("{\"type\":\"question_open\",\"title\":\"Quelle est la capitale du Portugal ?\"}");

    EXPECT_EQ(message.Title(), "Quelle est la capitale du Portugal ?");
}

TEST(MessageDeserializerTest, ParsedQuestionOpenWithoutTitleHasAnEmptyTitle)
{
    MessageDeserializer deserializer;

    EXPECT_EQ(deserializer.Parse("{\"type\":\"question_open\"}").Title(), "");
}

TEST(MessageDeserializerTest, ParsedQuestionOpenWithANumericTitleHasAnEmptyTitle)
{
    MessageDeserializer deserializer;

    EXPECT_EQ(deserializer.Parse("{\"type\":\"question_open\",\"title\":42}").Title(), "");
}

TEST(MessageDeserializerTest, ParsedQuestionTitleHasItsQuestionIndex)
{
    MessageDeserializer deserializer;

    HubMessage message = deserializer.Parse("{\"type\":\"question_title\",\"question_index\":2,\"title\":\"Quelle est la capitale du Portugal ?\"}");

    EXPECT_TRUE(message.HasQuestionIndex());
    EXPECT_EQ(message.QuestionIndex(), 2);
}

TEST(MessageDeserializerTest, ParsedQuestionTitleWithTheFirstQuestionIndexHasIt)
{
    MessageDeserializer deserializer;

    HubMessage message = deserializer.Parse("{\"type\":\"question_title\",\"question_index\":0,\"title\":\"Quelle est la capitale de la France ?\"}");

    EXPECT_TRUE(message.HasQuestionIndex());
    EXPECT_EQ(message.QuestionIndex(), 0);
}

TEST(MessageDeserializerTest, ParsedQuestionTitleWithoutQuestionIndexHasNone)
{
    MessageDeserializer deserializer;

    EXPECT_FALSE(deserializer.Parse("{\"type\":\"question_title\",\"title\":\"Quelle est la capitale de la France ?\"}").HasQuestionIndex());
}

TEST(MessageDeserializerTest, ParsedQuestionTitleWithATextQuestionIndexHasNone)
{
    MessageDeserializer deserializer;

    EXPECT_FALSE(deserializer.Parse("{\"type\":\"question_title\",\"question_index\":\"2\"}").HasQuestionIndex());
}

TEST(MessageDeserializerTest, ParsedQuestionChoicesHasItsFourChoicesInOrder)
{
    MessageDeserializer deserializer;

    HubMessage message = deserializer.Parse("{\"type\":\"question_choices\",\"choices\":[\"Paris\",\"Lyon\",\"Nice\",\"Lille\"]}");

    EXPECT_TRUE(message.HasChoices());
    EXPECT_EQ(message.Choices(), (std::vector<std::string>{"Paris", "Lyon", "Nice", "Lille"}));
}

TEST(MessageDeserializerTest, ParsedQuestionChoicesWithoutChoicesHasNone)
{
    MessageDeserializer deserializer;

    EXPECT_FALSE(deserializer.Parse("{\"type\":\"question_choices\"}").HasChoices());
}

TEST(MessageDeserializerTest, ParsedQuestionChoicesWithATextChoicesFieldHasNone)
{
    MessageDeserializer deserializer;

    EXPECT_FALSE(deserializer.Parse("{\"type\":\"question_choices\",\"choices\":\"Paris\"}").HasChoices());
}

TEST(MessageDeserializerTest, ParsedQuestionChoicesWithANumberAmongTheChoicesHasNone)
{
    MessageDeserializer deserializer;

    EXPECT_FALSE(deserializer.Parse("{\"type\":\"question_choices\",\"choices\":[\"Paris\",2,\"Nice\",\"Lille\"]}").HasChoices());
}

TEST(MessageDeserializerTest, ParsedQuestionResultHasItsCorrectAnswer)
{
    MessageDeserializer deserializer;

    HubMessage message = deserializer.Parse("{\"type\":\"question_result\",\"correct_answer\":\"B\"}");

    EXPECT_TRUE(message.HasCorrectAnswer());
    EXPECT_EQ(message.CorrectAnswer(), "B");
}

TEST(MessageDeserializerTest, ParsedQuestionResultHasItsCorrectFlag)
{
    MessageDeserializer deserializer;

    HubMessage message = deserializer.Parse("{\"type\":\"question_result\",\"correct\":true}");

    EXPECT_TRUE(message.HasCorrect());
    EXPECT_TRUE(message.Correct());
}

TEST(MessageDeserializerTest, ParsedQuestionResultWithAFalseCorrectFlagIsNotCorrect)
{
    MessageDeserializer deserializer;

    HubMessage message = deserializer.Parse("{\"type\":\"question_result\",\"correct\":false}");

    EXPECT_TRUE(message.HasCorrect());
    EXPECT_FALSE(message.Correct());
}

TEST(MessageDeserializerTest, ParsedQuestionResultHasItsPlayerAnswer)
{
    MessageDeserializer deserializer;

    HubMessage message = deserializer.Parse("{\"type\":\"question_result\",\"player_answer\":\"C\"}");

    EXPECT_TRUE(message.HasPlayerAnswer());
    EXPECT_EQ(message.PlayerAnswer(), "C");
}

TEST(MessageDeserializerTest, ParsedQuestionResultWithoutCorrectAnswerHasNone)
{
    MessageDeserializer deserializer;

    EXPECT_FALSE(deserializer.Parse("{\"type\":\"question_result\"}").HasCorrectAnswer());
}

TEST(MessageDeserializerTest, ParsedQuestionResultWithANumberAsCorrectAnswerHasNone)
{
    MessageDeserializer deserializer;

    EXPECT_FALSE(deserializer.Parse("{\"type\":\"question_result\",\"correct_answer\":2}").HasCorrectAnswer());
}

TEST(MessageDeserializerTest, ParsedQuestionResultWithoutCorrectFlagHasNone)
{
    MessageDeserializer deserializer;

    EXPECT_FALSE(deserializer.Parse("{\"type\":\"question_result\"}").HasCorrect());
}

TEST(MessageDeserializerTest, ParsedQuestionResultWithATextCorrectFlagHasNone)
{
    MessageDeserializer deserializer;

    EXPECT_FALSE(deserializer.Parse("{\"type\":\"question_result\",\"correct\":\"true\"}").HasCorrect());
}

TEST(MessageDeserializerTest, ParsedQuestionResultWithoutPlayerAnswerHasNone)
{
    MessageDeserializer deserializer;

    EXPECT_FALSE(deserializer.Parse("{\"type\":\"question_result\"}").HasPlayerAnswer());
}

TEST(MessageDeserializerTest, ParsedQuestionResultWithANumberAsPlayerAnswerHasNone)
{
    MessageDeserializer deserializer;

    EXPECT_FALSE(deserializer.Parse("{\"type\":\"question_result\",\"player_answer\":3}").HasPlayerAnswer());
}

TEST(MessageDeserializerTest, ParsedQuestionResultWithANullPlayerAnswerHasNone)
{
    MessageDeserializer deserializer;

    EXPECT_FALSE(deserializer.Parse("{\"type\":\"question_result\",\"player_answer\":null}").HasPlayerAnswer());
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}