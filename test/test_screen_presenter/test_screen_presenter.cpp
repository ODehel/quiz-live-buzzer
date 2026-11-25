#include <gtest/gtest.h>
#include <tuple>
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

class ArmingOpeningMessageTest : public ::testing::TestWithParam<HubMessage>
{
};

TEST_P(ArmingOpeningMessageTest, LeavesTheScreenUnchangedWhenAQuestionIsAlreadyArmed)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenFor(GetParam(), true);

    EXPECT_EQ(ScreenModel{ScreenUnchanged{}}, screen);
}

TEST_P(ArmingOpeningMessageTest, PresentsItsTextWhenNoQuestionIsArmed)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenFor(GetParam(), false);

    EXPECT_TRUE(std::holds_alternative<TextScreen>(screen));
}

INSTANTIATE_TEST_SUITE_P(
    QuestionOpenAndBuzzUnlocked,
    ArmingOpeningMessageTest,
    ::testing::Values(HubMessage{"question_open"}.WithTitle("Quelle est la capitale du Portugal ?"), HubMessage{"buzz_unlocked"}));

// Un `timer_tick` ne remplace jamais l'écran, armé ou non, avec ou sans `remaining_seconds` (US-FW-011 CA-8, US-FW-012 CA-4).
class TimerTickTest : public ::testing::TestWithParam<std::tuple<bool, HubMessage>>
{
};

TEST_P(TimerTickTest, LeavesTheScreenUnchanged)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenFor(std::get<1>(GetParam()), std::get<0>(GetParam()));

    EXPECT_EQ(ScreenModel{ScreenUnchanged{}}, screen);
}

INSTANTIATE_TEST_SUITE_P(
    ArmedOrNotWithOrWithoutRemainingSeconds,
    TimerTickTest,
    ::testing::Combine(
        ::testing::Bool(),
        ::testing::Values(HubMessage{"timer_tick"}.WithRemainingSeconds(25), HubMessage{"timer_tick"}.WithRemainingSeconds(0), HubMessage{"timer_tick"})));

TEST(ScreenPresenterQuestionChoicesTest, PresentsTheFourChoicesAsZones)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenFor(HubMessage{"question_choices"}.WithChoices({"Paris", "Lyon", "Nice", "Lille"}), false);

    const ScreenModel expected = ZonesScreen{{"A : Paris", "B : Lyon", "C : Nice", "D : Lille"}};
    EXPECT_EQ(expected, screen);
}

TEST(ScreenPresenterQuestionChoicesTest, LeavesTheScreenUnchangedWhenAQuestionIsAlreadyArmed)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenFor(HubMessage{"question_choices"}.WithChoices({"Paris", "Lyon", "Nice", "Lille"}), true);

    EXPECT_EQ(ScreenModel{ScreenUnchanged{}}, screen);
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

TEST(ScreenPresenterStartupTest, ARefusedLoginPresentsTheReasonAndWhoToWarn)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenForLoginRefused();

    EXPECT_EQ(ScreenModel{TextScreen{"Connexion refusée\nLe hub n’a pas accepté ce buzzer\nPréviens le maître du jeu"}}, screen);
}

TEST(ScreenPresenterStartupTest, TheInitScreenPresentsTheLogoAndTheConnectionLabelOnThreeLines)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenForInit();

    EXPECT_EQ(ScreenModel{TextScreen{"Buzz’\nO’Live\nConnexion…"}}, screen);
}

TEST(ScreenPresenterTopBarTest, AuthSuccessShowsTheUsernameInTheTopBar)
{
    ScreenPresenter presenter;

    const TopBar topBar = presenter.TopBarFor(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_01"), TopBar::ForPlayer("ancien nom").WithLinkCut());

    EXPECT_EQ(TopBar::ForPlayer("quiz_buzzer_01"), topBar);
}

TEST(ScreenPresenterTopBarTest, TheInitTopBarIsVisibleWithAnEmptyNameAndACutLink)
{
    ScreenPresenter presenter;

    const TopBar topBar = presenter.TopBarForInit();

    EXPECT_EQ(TopBar::ForPlayer("").WithLinkCut(), topBar);
}

class MessageKeepingTheTopBarTest : public ::testing::TestWithParam<std::string>
{
};

TEST_P(MessageKeepingTheTopBarTest, KeepsTheVisibleTopBarWithItsNameAndItsLink)
{
    ScreenPresenter presenter;

    const TopBar topBar = presenter.TopBarFor(HubMessage{GetParam()}, TopBar::ForPlayer("quiz_buzzer_01").WithLinkCut());

    EXPECT_EQ(TopBar::ForPlayer("quiz_buzzer_01").WithLinkCut(), topBar);
}

INSTANTIATE_TEST_SUITE_P(
    MessagesKeepingTheTopBar,
    MessageKeepingTheTopBarTest,
    ::testing::Values("game_resumed", "question_open", "question_title", "question_choices", "question_result",
                      "timer_tick", "timer_end", "buzz_locked", "buzz_accepted", "answer_received", "unknown_type"));

TEST(ScreenPresenterTopBarTest, AuthSuccessWithoutUsernameShowsAnEmptyName)
{
    ScreenPresenter presenter;

    const TopBar topBar = presenter.TopBarFor(HubMessage{"auth_success"}, TopBar::ForPlayer("ancien nom").WithLinkCut());

    EXPECT_EQ(TopBar::ForPlayer(""), topBar);
}

TEST(ScreenPresenterTopBarTest, AuthSuccessShowsALinkedTopBar)
{
    ScreenPresenter presenter;

    const TopBar topBar = presenter.TopBarFor(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_01"), TopBar::ForPlayer("ancien nom").WithLinkCut());

    EXPECT_FALSE(topBar.linkCut);
}

TEST(TopBarTest, ACutLinkKeepsTheNameAndTheVisibility)
{
    const TopBar cut = TopBar::ForPlayer("Camille").WithLinkCut();

    EXPECT_TRUE(cut.visible);
    EXPECT_EQ("Camille", cut.playerName);
    EXPECT_TRUE(cut.linkCut);
}

TEST(TopBarTest, ACutLinkIsNotTheLinkedTopBarOfTheSameName)
{
    EXPECT_FALSE(TopBar::ForPlayer("Camille") == TopBar::ForPlayer("Camille").WithLinkCut());
}

TEST(TopBarTest, AHiddenTopBarHasNoCutLink)
{
    EXPECT_FALSE(TopBar::Hidden().linkCut);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(ScreenPresenterOutOfGameTest, PresentsTheTitleTheCauseAndTheOnlyWayOut)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenForOutOfGame();

    const ScreenModel expected = OutOfGameScreen{"Hors jeu", "Connexion perdue après 3 tentatives", "Redémarre le buzzer pour revenir"};
    EXPECT_EQ(expected, screen);
}

TEST(ScreenPresenterConnectionLostTest, PresentsTheTitleAndTheAttemptInProgressOutOfThree)
{
    ScreenPresenter presenter;

    const ScreenModel screen = presenter.ScreenForConnectionLost(2);

    const ScreenModel expected = ConnectionLostScreen{"Connexion perdue", "Tentative 2/3", 2, 3};
    EXPECT_EQ(expected, screen);
}

TEST(ScreenPresenterConnectionLostTest, NumbersTheFirstAndTheLastAttempt)
{
    ScreenPresenter presenter;

    EXPECT_EQ((ScreenModel{ConnectionLostScreen{"Connexion perdue", "Tentative 1/3", 1, 3}}), presenter.ScreenForConnectionLost(1));
    EXPECT_EQ((ScreenModel{ConnectionLostScreen{"Connexion perdue", "Tentative 3/3", 3, 3}}), presenter.ScreenForConnectionLost(3));
}

TEST(ScreenPresenterConnectionLostTest, LeavesTheDimmedScreenToTheRetainedOne)
{
    ScreenPresenter presenter;

    const ConnectionLostScreen screen = std::get<ConnectionLostScreen>(presenter.ScreenForConnectionLost(1));

    EXPECT_EQ(DimmedScreen{}, screen.dimmed);
    EXPECT_FALSE(screen.bannerOnly);
}
