#include <gtest/gtest.h>
#include "ScreenPresenter.h"

TEST(ScreenPresenterTest, AuthSuccessPresentsTheTextConnecte)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenFor(HubMessage{"auth_success"}, false);

    EXPECT_EQ(ScreenModel{TextScreen{"Connecté"}}, screen);
}

class MessageWithoutDisplayTest : public ::testing::TestWithParam<std::string>
{
};

TEST_P(MessageWithoutDisplayTest, LeavesTheScreenUnchanged)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenFor(HubMessage{GetParam()}, false);

    EXPECT_EQ(ScreenModel{ScreenUnchanged{}}, screen);
}

INSTANTIATE_TEST_SUITE_P(
    MessagesLeavingTheScreenUnchanged,
    MessageWithoutDisplayTest,
    ::testing::Values("play_sound_url", "play_system_sound", "token_expiring_soon", "unknown_type"));

TEST(ScreenPresenterTimerEndTest, PresentsTooLateWhenAQuestionIsArmed)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenFor(HubMessage{"timer_end"}, true);

    EXPECT_EQ(ScreenModel{TextScreen{"Trop tard"}}, screen);
}

TEST(ScreenPresenterTimerEndTest, LeavesTheScreenUnchangedWhenNoQuestionIsArmed)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenFor(HubMessage{"timer_end"}, false);

    EXPECT_EQ(ScreenModel{ScreenUnchanged{}}, screen);
}

TEST(ScreenPresenterQuestionChoicesTest, PresentsTheFourChoicesAsZones)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenFor(HubMessage{"question_choices"}.WithChoices({"Paris", "Lyon", "Nice", "Lille"}), false);

    const ScreenModel expected = ZonesScreen{{"A : Paris", "B : Lyon", "C : Nice", "D : Lille"}};
    EXPECT_EQ(expected, screen);
}

TEST(ScreenPresenterQuestionChoicesTest, LeavesTheScreenUnchangedWithoutChoices)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenFor(HubMessage{"question_choices"}, false);

    EXPECT_EQ(ScreenModel{ScreenUnchanged{}}, screen);
}

TEST(ScreenPresenterQuestionResultTest, McqResultHighlightsTheCorrectZoneWithTheResultText)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenFor(HubMessage{"question_result"}.WithCorrectAnswer("B").WithPlayerAnswer("C"), false);

    const ScreenModel expected = HighlightedZoneScreen{1, "Bonne réponse : B\nVotre réponse : C"};
    EXPECT_EQ(expected, screen);
}

TEST(ScreenPresenterQuestionResultTest, McqResultWithCorrectAnswerAHighlightsTheFirstZone)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenFor(HubMessage{"question_result"}.WithCorrectAnswer("A"), false);

    const ScreenModel expected = HighlightedZoneScreen{0, "Bonne réponse : A"};
    EXPECT_EQ(expected, screen);
}

TEST(ScreenPresenterQuestionResultTest, SpeedResultPresentsItsTextWithoutHighlightingAZone)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenFor(HubMessage{"question_result"}.WithCorrectAnswer("Paris").WithCorrect(true), false);

    EXPECT_EQ(ScreenModel{TextScreen{"Validé"}}, screen);
}

TEST(ScreenPresenterAnswerReceivedTest, PresentsItsTextInABannerKeepingTheZones)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenFor(HubMessage{"answer_received"}, false);

    EXPECT_EQ(ScreenModel{BannerScreen{"Réponse enregistrée"}}, screen);
}

using ChosenLetterCase = std::pair<char, size_t>;

class TappedAnswerTest : public ::testing::TestWithParam<ChosenLetterCase>
{
};

TEST_P(TappedAnswerTest, PresentsTheChosenZoneWithoutText)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenForTappedAnswer(GetParam().first);

    EXPECT_EQ(ScreenModel{ChosenZoneScreen{GetParam().second}}, screen);
}

INSTANTIATE_TEST_SUITE_P(
    TheFourChoiceLetters,
    TappedAnswerTest,
    ::testing::Values(ChosenLetterCase{'A', 0}, ChosenLetterCase{'B', 1}, ChosenLetterCase{'C', 2}, ChosenLetterCase{'D', 3}));

TEST(ScreenPresenterLocalFeedbackTest, AnUnknownTappedLetterLeavesTheScreenUnchanged)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenForTappedAnswer('Z');

    EXPECT_EQ(ScreenModel{ScreenUnchanged{}}, screen);
}

TEST(ScreenPresenterLocalFeedbackTest, ABuzzSentPresentsBuzzEnvoye)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenForBuzzSent();

    EXPECT_EQ(ScreenModel{TextScreen{"Buzz envoyé"}}, screen);
}

TEST(ScreenPresenterTopBarTest, AuthSuccessShowsTheUsernameInTheTopBar)
{
    ScreenPresenter presenter;

    const TopBar topBar = presenter.TopBarFor(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_01"));

    EXPECT_EQ(TopBar::ForPlayer("quiz_buzzer_01"), topBar);
}

class MessageWithoutTopBarTest : public ::testing::TestWithParam<std::string>
{
};

TEST_P(MessageWithoutTopBarTest, HidesTheTopBar)
{
    ScreenPresenter presenter;

    const TopBar topBar = presenter.TopBarFor(HubMessage{GetParam()});

    EXPECT_EQ(TopBar::Hidden(), topBar);
}

INSTANTIATE_TEST_SUITE_P(
    MessagesWithoutATopBar,
    MessageWithoutTopBarTest,
    ::testing::Values("game_resumed", "question_open", "question_title", "question_choices", "question_result",
                      "timer_tick", "timer_end", "buzz_locked", "buzz_accepted", "answer_received", "unknown_type"));

TEST(ScreenPresenterTopBarTest, AuthSuccessWithoutUsernameShowsAnEmptyName)
{
    ScreenPresenter presenter;

    const TopBar topBar = presenter.TopBarFor(HubMessage{"auth_success"});

    EXPECT_EQ(TopBar::ForPlayer(""), topBar);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
