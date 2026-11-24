#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "../mocks/GameMocks.h"
#include "BuzzerBehavior.h"
#include "HubMessageDispatcher.h"

TEST(HubMessageDispatcherTest, QuestionOpenArmsSpeed)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    dispatcher.Dispatch("question_open");
    buzzerBehavior.OnBuzzPressed();
}

TEST(HubMessageDispatcherTest, QuestionChoicesArmsMcq)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);

    dispatcher.Dispatch("question_choices");
    buzzerBehavior.OnMcqPressed('A');
}

TEST(HubMessageDispatcherTest, TimerEndDisarmsArmedBuzzer)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    
    dispatcher.Dispatch("question_open");
    dispatcher.Dispatch("timer_end");
    buzzerBehavior.OnBuzzPressed();
}

TEST(HubMessageDispatcherTest, BuzzUnlockedArmsSpeed)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    dispatcher.Dispatch("buzz_unlocked");
    buzzerBehavior.OnBuzzPressed();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(HubMessageDispatcherTest, BuzzAcceptedKeepsSpeedArmed)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    dispatcher.Dispatch("question_open");
    dispatcher.Dispatch("buzz_accepted");

    EXPECT_TRUE(buzzerBehavior.IsArmed());
    buzzerBehavior.OnBuzzPressed();
}

TEST(HubMessageDispatcherTest, BuzzInvalidatedKeepsSpeedArmed)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    dispatcher.Dispatch("question_open");
    dispatcher.Dispatch("buzz_invalidated");

    EXPECT_TRUE(buzzerBehavior.IsArmed());
    buzzerBehavior.OnBuzzPressed();
}

TEST(HubMessageDispatcherTest, BuzzAcceptedKeepsInertBuzzerInert)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);

    dispatcher.Dispatch("buzz_accepted");

    EXPECT_FALSE(buzzerBehavior.IsArmed());
    buzzerBehavior.OnBuzzPressed();
}

TEST(HubMessageDispatcherTest, BuzzInvalidatedKeepsInertBuzzerInert)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);

    dispatcher.Dispatch("buzz_invalidated");

    EXPECT_FALSE(buzzerBehavior.IsArmed());
    buzzerBehavior.OnBuzzPressed();
}

TEST(HubMessageDispatcherTest, BuzzLockedDisarmsSpeedArmedBuzzer)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);

    dispatcher.Dispatch("question_open");
    dispatcher.Dispatch("buzz_locked");

    EXPECT_FALSE(buzzerBehavior.IsArmed());
    buzzerBehavior.OnBuzzPressed();
}

TEST(HubMessageDispatcherTest, BuzzUnlockedArmsAgainAfterBuzzLocked)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    dispatcher.Dispatch("question_open");
    dispatcher.Dispatch("buzz_locked");
    dispatcher.Dispatch("buzz_unlocked");

    EXPECT_TRUE(buzzerBehavior.IsArmed());
    buzzerBehavior.OnBuzzPressed();
}

TEST(HubMessageDispatcherTest, BuzzLockedKeepsInertBuzzerInert)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);
    EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(1);

    dispatcher.Dispatch("buzz_locked");

    EXPECT_FALSE(buzzerBehavior.IsArmed());

    dispatcher.Dispatch("buzz_unlocked");
    buzzerBehavior.OnBuzzPressed();
}

TEST(HubMessageDispatcherTest, BuzzLockedKeepsMcqArmed)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);

    dispatcher.Dispatch("question_choices");
    dispatcher.Dispatch("buzz_locked");

    EXPECT_TRUE(buzzerBehavior.IsArmed());
    buzzerBehavior.OnMcqPressed('A');
}

struct HubTypeCase
{
    const char *name;
    std::string type;
};

class ModeUnchangedByHubTypeTest : public ::testing::TestWithParam<HubTypeCase>
{
protected:
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    void DispatchLeavesNothingSent(const std::string &type)
    {
        EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);
        EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
        dispatcher.Dispatch(type);
        ::testing::Mock::VerifyAndClearExpectations(&mockHubMessageSender);
    }

    void ExpectMcqArmedStaysArmed(const std::string &type)
    {
        dispatcher.Dispatch("question_choices");

        DispatchLeavesNothingSent(type);

        EXPECT_TRUE(buzzerBehavior.IsArmed());
        EXPECT_CALL(mockHubMessageSender, SendAnswer('A')).Times(1);
        EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
        buzzerBehavior.OnMcqPressed('A');
    }

    void ExpectSpeedArmedStaysArmed(const std::string &type)
    {
        dispatcher.Dispatch("question_open");

        DispatchLeavesNothingSent(type);

        EXPECT_TRUE(buzzerBehavior.IsArmed());
        EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);
        EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);
        buzzerBehavior.OnBuzzPressed();
    }

    void ExpectInertStaysInert(const std::string &type)
    {
        DispatchLeavesNothingSent(type);

        EXPECT_FALSE(buzzerBehavior.IsArmed());
        EXPECT_CALL(mockHubMessageSender, SendAnswer(::testing::_)).Times(0);
        EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);
        EXPECT_CALL(mockLocalFeedback, AcknowledgeAnswer(::testing::_)).Times(0);
        EXPECT_CALL(mockLocalFeedback, AcknowledgeBuzz()).Times(0);
        buzzerBehavior.OnMcqPressed('A');
        buzzerBehavior.OnBuzzPressed();
    }
};

class KnownTypeWithoutEffectOnTheModeTest : public ModeUnchangedByHubTypeTest
{
};

class UnrecognizedTypeIsIgnoredTest : public ModeUnchangedByHubTypeTest
{
};

TEST_P(KnownTypeWithoutEffectOnTheModeTest, KeepsMcqArmedBuzzerArmed)
{
    ExpectMcqArmedStaysArmed(GetParam().type);
}

TEST_P(KnownTypeWithoutEffectOnTheModeTest, KeepsSpeedArmedBuzzerArmed)
{
    ExpectSpeedArmedStaysArmed(GetParam().type);
}

TEST_P(KnownTypeWithoutEffectOnTheModeTest, KeepsInertBuzzerInert)
{
    ExpectInertStaysInert(GetParam().type);
}

TEST_P(UnrecognizedTypeIsIgnoredTest, KeepsMcqArmedBuzzerArmed)
{
    ExpectMcqArmedStaysArmed(GetParam().type);
}

TEST_P(UnrecognizedTypeIsIgnoredTest, KeepsSpeedArmedBuzzerArmed)
{
    ExpectSpeedArmedStaysArmed(GetParam().type);
}

TEST_P(UnrecognizedTypeIsIgnoredTest, KeepsInertBuzzerInert)
{
    ExpectInertStaysInert(GetParam().type);
}

static std::string HubTypeCaseName(const ::testing::TestParamInfo<HubTypeCase> &info)
{
    return info.param.name;
}

INSTANTIATE_TEST_SUITE_P(
    EveryKnownTypeWithoutEffect,
    KnownTypeWithoutEffectOnTheModeTest,
    ::testing::Values(
        HubTypeCase{"QuestionTitle", "question_title"},
        HubTypeCase{"TimerTick", "timer_tick"},
        HubTypeCase{"AnswerReceived", "answer_received"},
        HubTypeCase{"BuzzAccepted", "buzz_accepted"},
        HubTypeCase{"BuzzInvalidated", "buzz_invalidated"},
        HubTypeCase{"QuestionResult", "question_result"},
        HubTypeCase{"GameResumed", "game_resumed"},
        HubTypeCase{"TokenExpired", "token_expired"},
        HubTypeCase{"PlaySoundUrl", "play_sound_url"},
        HubTypeCase{"PlaySystemSound", "play_system_sound"}),
    HubTypeCaseName);

INSTANTIATE_TEST_SUITE_P(
    UnknownEmptyMiscasedAndErrorTypes,
    UnrecognizedTypeIsIgnoredTest,
    ::testing::Values(
        HubTypeCase{"UnknownType", "some_unknown_type"},
        HubTypeCase{"EmptyType", ""},
        HubTypeCase{"UpperCaseQuestionOpen", "QUESTION_OPEN"},
        HubTypeCase{"MixedCaseQuestionChoices", "Question_Choices"},
        HubTypeCase{"UpperCaseBuzzUnlocked", "BUZZ_UNLOCKED"},
        HubTypeCase{"UpperCaseTimerEnd", "TIMER_END"},
        HubTypeCase{"LeadingSpaceQuestionOpen", " question_open"},
        HubTypeCase{"TrailingSpaceTimerEnd", "timer_end "},
        HubTypeCase{"ErrorFromTheHub", "error"},
        HubTypeCase{"UpperCaseError", "ERROR"}),
    HubTypeCaseName);

TEST(HubMessageDispatcherLinkTest, LinkLostMakesTheBuzzerIgnoreItsPresses)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(0);

    dispatcher.Dispatch("question_open");
    dispatcher.OnLinkLost();
    buzzerBehavior.OnBuzzPressed();
}

TEST(HubMessageDispatcherLinkTest, LinkEstablishedMakesTheBuzzerHearItsPressesAgain)
{
    MockHubMessageSender mockHubMessageSender;
    MockLocalFeedback mockLocalFeedback;
    BuzzerBehavior buzzerBehavior{mockHubMessageSender, mockLocalFeedback};
    HubMessageDispatcher dispatcher{buzzerBehavior};

    EXPECT_CALL(mockHubMessageSender, SendBuzz()).Times(1);

    dispatcher.Dispatch("question_open");
    dispatcher.OnLinkLost();
    dispatcher.OnLinkEstablished();
    buzzerBehavior.OnBuzzPressed();
}
