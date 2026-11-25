#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <tuple>
#include "LcdRenderer.h"
#include "../mocks/ScreenMocks.h"
#include "../mocks/GameMocks.h"

namespace
{
// Avant tout `auth_success` : le bandeau de l'init, nom vide, liaison « coupée » (US-FW-009 CA-3 et CA-4).
const TopBar InitTopBar = ScreenPresenter{}.TopBarForInit();
}

TEST(LcdRendererTest, AuthSuccessMessageShowsTheWaitForTheGameMasterOnScreen)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01"), ScreenModel{TextScreen{"En attente du maître du jeu\nLa partie va bientôt commencer"}})).Times(1);

    lcdRenderer.Apply(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_01"));
}

using FixedTextCase = std::pair<std::string, std::string>;

class MessageWithFixedTextTest : public ::testing::TestWithParam<FixedTextCase>
{
};

TEST_P(MessageWithFixedTextTest, ShowsItsFixedTextOnScreen)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(InitTopBar, ScreenModel{TextScreen{GetParam().second}})).Times(1);

    lcdRenderer.Apply(HubMessage{GetParam().first});
}

INSTANTIATE_TEST_SUITE_P(
    MessagesWithAFixedText,
    MessageWithFixedTextTest,
    ::testing::Values(
        FixedTextCase{"buzz_accepted", "À toi !"},
        FixedTextCase{"buzz_invalidated", "Raté"},
        FixedTextCase{"buzz_unlocked", "À nouveau possible de buzzer"}));

TEST(LcdRendererTimerEndTest, ShowsTooLateWhenAQuestionIsArmed)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    ON_CALL(armedModeReader, IsArmed()).WillByDefault(testing::Return(true));
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(InitTopBar, ScreenModel{TextScreen{"Trop tard"}})).Times(1);

    lcdRenderer.Apply(HubMessage{"timer_end"});
}

TEST(LcdRendererTimerEndTest, LeavesTheScreenUnchangedWhenNoQuestionIsArmed)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    ON_CALL(armedModeReader, IsArmed()).WillByDefault(testing::Return(false));
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(testing::_, testing::_)).Times(0);

    lcdRenderer.Apply(HubMessage{"timer_end"});
}

using TimerTickCase = std::tuple<bool, int>;

class TimerTickTest : public ::testing::TestWithParam<TimerTickCase>
{
};

TEST_P(TimerTickTest, LeavesTheScreenUnchanged)
{
    testing::StrictMock<MockScreen> mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    ON_CALL(armedModeReader, IsArmed()).WillByDefault(testing::Return(std::get<0>(GetParam())));
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    lcdRenderer.Apply(HubMessage{"timer_tick"}.WithRemainingSeconds(std::get<1>(GetParam())));
}

INSTANTIATE_TEST_SUITE_P(
    ArmedOrNotWithRemainingSeconds,
    TimerTickTest,
    ::testing::Combine(
        ::testing::Bool(),
        ::testing::Values(25, 5, 0)));

TEST(LcdRendererTimerTickTest, LeavesTheScreenUnchangedWithoutRemainingSeconds)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(testing::_, testing::_)).Times(0);

    lcdRenderer.Apply(HubMessage{"timer_tick"});
}

using BuzzLockedCase = std::pair<std::string, std::string>;

class BuzzLockedTest : public ::testing::TestWithParam<BuzzLockedCase>
{
};

TEST_P(BuzzLockedTest, ShowsWhoBuzzedFirstOnScreen)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(InitTopBar, ScreenModel{TextScreen{GetParam().second}})).Times(1);

    lcdRenderer.Apply(HubMessage{"buzz_locked"}.WithBuzzerUsername(GetParam().first));
}

INSTANTIATE_TEST_SUITE_P(
    BuzzerUsernames,
    BuzzLockedTest,
    ::testing::Values(
        BuzzLockedCase{"Alice", "Alice a buzzé"},
        BuzzLockedCase{"Bob", "Bob a buzzé"}));

TEST(LcdRendererBuzzLockedTest, LeavesTheScreenUnchangedWithoutBuzzerUsername)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(testing::_, testing::_)).Times(0);

    lcdRenderer.Apply(HubMessage{"buzz_locked"});
}

using QuestionOpenCase = std::pair<std::string, std::string>;

class QuestionOpenTest : public ::testing::TestWithParam<QuestionOpenCase>
{
};

TEST_P(QuestionOpenTest, ShowsTheQuestionTitleOnScreen)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(InitTopBar, ScreenModel{TextScreen{GetParam().second}})).Times(1);

    lcdRenderer.Apply(HubMessage{"question_open"}.WithTitle(GetParam().first));
}

INSTANTIATE_TEST_SUITE_P(
    QuestionTitles,
    QuestionOpenTest,
    ::testing::Values(
        QuestionOpenCase{"Quelle est la capitale du Portugal ?", "Quelle est la capitale du Portugal ?"},
        QuestionOpenCase{"Qui a buzzé le premier hier soir ?", "Qui a buzzé le premier hier soir ?"}));

TEST(LcdRendererQuestionOpenTest, LeavesTheScreenUnchangedWithoutTitle)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(testing::_, testing::_)).Times(0);

    lcdRenderer.Apply(HubMessage{"question_open"});
}

TEST(LcdRendererQuestionTitleTest, ShowsTheQuestionNumberAndTitleOnScreen)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(InitTopBar, ScreenModel{TextScreen{"Question 3 : Quel fleuve traverse Lisbonne ?"}})).Times(1);

    lcdRenderer.Apply(HubMessage{"question_title"}.WithQuestionIndex(2).WithTitle("Quel fleuve traverse Lisbonne ?"));
}

TEST(LcdRendererQuestionTitleTest, LeavesTheScreenUnchangedWithoutQuestionIndex)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(testing::_, testing::_)).Times(0);

    lcdRenderer.Apply(HubMessage{"question_title"}.WithTitle("Quel fleuve traverse Lisbonne ?"));
}

class MessageWithoutDisplayTest : public ::testing::TestWithParam<std::string>
{
};

TEST_P(MessageWithoutDisplayTest, LeavesTheScreenUnchanged)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(testing::_, testing::_)).Times(0);

    lcdRenderer.Apply(HubMessage{GetParam()});
}

INSTANTIATE_TEST_SUITE_P(
    MessagesLeavingTheScreenUnchanged,
    MessageWithoutDisplayTest,
    ::testing::Values("play_sound_url", "play_system_sound", "token_expiring_soon", "unknown_type"));

TEST(LcdRendererTest, QuestionChoicesShowsTheFourChoicesAsZonesOnScreen)
{
    testing::StrictMock<MockScreen> mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    ON_CALL(armedModeReader, IsArmed()).WillByDefault(testing::Return(false));
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(InitTopBar, ScreenModel{ZonesScreen{std::vector<std::string>{"A : Paris", "B : Lyon", "C : Nice", "D : Lille"}}})).Times(1);

    lcdRenderer.Apply(HubMessage{"question_choices"}.WithChoices({"Paris", "Lyon", "Nice", "Lille"}));
}

TEST(LcdRendererTest, QuestionChoicesLeavesTheScreenUnchangedWhenAQuestionIsAlreadyArmed)
{
    testing::StrictMock<MockScreen> mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    ON_CALL(armedModeReader, IsArmed()).WillByDefault(testing::Return(true));
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    lcdRenderer.Apply(HubMessage{"question_choices"}.WithChoices({"Paris", "Lyon", "Nice", "Lille"}));
}

struct ArmingOpeningCase
{
    HubMessage message;
    std::string openingText;
};

class ArmingOpeningMessageTest : public ::testing::TestWithParam<ArmingOpeningCase>
{
};

TEST_P(ArmingOpeningMessageTest, LeavesTheScreenUnchangedWhenAQuestionIsAlreadyArmed)
{
    testing::StrictMock<MockScreen> mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    ON_CALL(armedModeReader, IsArmed()).WillByDefault(testing::Return(true));
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    lcdRenderer.Apply(GetParam().message);
}

TEST_P(ArmingOpeningMessageTest, ShowsTheOpeningScreenWhenNoQuestionIsArmed)
{
    testing::StrictMock<MockScreen> mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    ON_CALL(armedModeReader, IsArmed()).WillByDefault(testing::Return(false));
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(InitTopBar, ScreenModel{TextScreen{GetParam().openingText}})).Times(1);

    lcdRenderer.Apply(GetParam().message);
}

INSTANTIATE_TEST_SUITE_P(
    QuestionOpenAndBuzzUnlocked,
    ArmingOpeningMessageTest,
    ::testing::Values(
        ArmingOpeningCase{HubMessage{"question_open"}.WithTitle("Quelle est la capitale du Portugal ?"), "Quelle est la capitale du Portugal ?"},
        ArmingOpeningCase{HubMessage{"buzz_unlocked"}, "À nouveau possible de buzzer"}));

TEST(LcdRendererTest, McqQuestionResultHighlightsTheCorrectZoneWithTheResultText)
{
    testing::StrictMock<MockScreen> mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(InitTopBar, ScreenModel{HighlightedZoneScreen{1, "Bonne réponse : B\nVotre réponse : C"}})).Times(1);

    lcdRenderer.Apply(HubMessage{"question_result"}.WithCorrectAnswer("B").WithPlayerAnswer("C"));
}

TEST(LcdRendererTest, McqQuestionResultWithCorrectAnswerAHighlightsTheFirstZone)
{
    testing::StrictMock<MockScreen> mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(InitTopBar, ScreenModel{HighlightedZoneScreen{0, "Bonne réponse : A"}})).Times(1);

    lcdRenderer.Apply(HubMessage{"question_result"}.WithCorrectAnswer("A"));
}

TEST(LcdRendererTest, SpeedQuestionResultShowsItsTextWithoutHighlightingAZone)
{
    testing::StrictMock<MockScreen> mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(InitTopBar, ScreenModel{TextScreen{"Validé"}})).Times(1);

    lcdRenderer.Apply(HubMessage{"question_result"}.WithCorrectAnswer("Paris").WithCorrect(true));
}

TEST(LcdRendererTest, QuestionChoicesWithoutChoicesLeavesTheScreenUnchanged)
{
    testing::StrictMock<MockScreen> mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    lcdRenderer.Apply(HubMessage{"question_choices"});
}

TEST(LcdRendererTest, AnswerReceivedShowsItsTextInABannerKeepingTheZones)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(InitTopBar, ScreenModel{BannerScreen{"Réponse enregistrée"}})).Times(1);

    lcdRenderer.Apply(HubMessage{"answer_received"});
}

namespace
{
struct ChangingScreenCase
{
    HubMessage message;
    ScreenModel screen;
    bool armed;
};

const std::vector<std::string> FourChoices = {"A : Paris", "B : Lyon", "C : Nice", "D : Lille"};
}

// Le bandeau reste visible, même nom, pour tout écran qui change (US-FW-009 CA-3) : un texte, des zones, une zone en évidence, un bandeau de texte.
class ChangingScreenKeepsTheTopBarTest : public ::testing::TestWithParam<ChangingScreenCase>
{
};

TEST_P(ChangingScreenKeepsTheTopBarTest, ShowsTheNameOfTheAuthSuccessInTheTopBar)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    ON_CALL(armedModeReader, IsArmed()).WillByDefault(testing::Return(GetParam().armed));
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};
    lcdRenderer.Apply(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_01"));

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01"), GetParam().screen)).Times(1);

    lcdRenderer.Apply(GetParam().message);
}

INSTANTIATE_TEST_SUITE_P(
    EveryKindOfScreen,
    ChangingScreenKeepsTheTopBarTest,
    ::testing::Values(
        ChangingScreenCase{HubMessage{"question_open"}.WithTitle("Quelle est la capitale du Portugal ?"), TextScreen{"Quelle est la capitale du Portugal ?"}, false},
        ChangingScreenCase{HubMessage{"question_title"}.WithQuestionIndex(2).WithTitle("Quel fleuve ?"), TextScreen{"Question 3 : Quel fleuve ?"}, false},
        ChangingScreenCase{HubMessage{"question_choices"}.WithChoices({"Paris", "Lyon", "Nice", "Lille"}), ZonesScreen{FourChoices}, false},
        ChangingScreenCase{HubMessage{"answer_received"}, BannerScreen{"Réponse enregistrée"}, true},
        ChangingScreenCase{HubMessage{"question_result"}.WithCorrectAnswer("B").WithPlayerAnswer("C"), HighlightedZoneScreen{1, "Bonne réponse : B\nVotre réponse : C"}, false},
        ChangingScreenCase{HubMessage{"question_result"}.WithCorrectAnswer("Paris").WithCorrect(true), TextScreen{"Validé"}, false},
        ChangingScreenCase{HubMessage{"buzz_accepted"}, TextScreen{"À toi !"}, false},
        ChangingScreenCase{HubMessage{"buzz_locked"}.WithBuzzerUsername("Alice"), TextScreen{"Alice a buzzé"}, false},
        ChangingScreenCase{HubMessage{"buzz_invalidated"}, TextScreen{"Raté"}, false},
        ChangingScreenCase{HubMessage{"buzz_unlocked"}, TextScreen{"À nouveau possible de buzzer"}, false},
        ChangingScreenCase{HubMessage{"timer_end"}, TextScreen{"Trop tard"}, true}));

TEST(LcdRendererTopBarTest, TheNameOfALaterAuthSuccessReplacesThePreviousOneOnTheScreensThatFollow)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};
    lcdRenderer.Apply(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_01"));
    lcdRenderer.Apply(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_02"));

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_02"), ScreenModel{TextScreen{"À toi !"}})).Times(1);

    lcdRenderer.Apply(HubMessage{"buzz_accepted"});
}

TEST(LcdRendererTopBarTest, AnAuthSuccessWithoutUsernameLeavesTheTopBarVisibleWithAnEmptyNameOnTheScreensThatFollow)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};
    lcdRenderer.Apply(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_01"));
    lcdRenderer.Apply(HubMessage{"auth_success"});

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer(""), ScreenModel{TextScreen{"À toi !"}})).Times(1);

    lcdRenderer.Apply(HubMessage{"buzz_accepted"});
}

TEST(LcdRendererTopBarTest, TheScreensThatFollowALossOfTheLinkKeepTheCutLinkUntilTheNextAuthSuccess)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};
    lcdRenderer.Apply(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_01"));
    lcdRenderer.ShowLinkLost(1);

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01").WithLinkCut(), ScreenModel{TextScreen{"À toi !"}})).Times(1);

    lcdRenderer.Apply(HubMessage{"buzz_accepted"});
}

TEST(LcdRendererTopBarTest, TheLossOfTheLinkIsSharedWithTheOtherUsersOfTheVisibleTopBar)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};
    lcdRenderer.Apply(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_01"));

    lcdRenderer.ShowLinkLost(1);

    EXPECT_EQ(TopBar::ForPlayer("quiz_buzzer_01").WithLinkCut(), visibleTopBar.Current());
}

TEST(LcdRendererTopBarTest, AMessageThatLeavesTheScreenUnchangedLeavesTheTopBarUntouched)
{
    testing::StrictMock<MockScreen> mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};
    EXPECT_CALL(mockScreen, Display(testing::_, testing::_)).Times(1);
    lcdRenderer.Apply(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_01"));

    lcdRenderer.Apply(HubMessage{"timer_tick"}.WithRemainingSeconds(25));
    lcdRenderer.Apply(HubMessage{"question_open"});
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(LcdRendererOutOfGameTest, ShowsTheOutOfGameScreenWithTheLastKnownName)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};
    lcdRenderer.Apply(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_01"));

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01").WithLinkCut(), ScreenPresenter{}.ScreenForOutOfGame())).Times(1);

    lcdRenderer.ShowOutOfGame();
}

TEST(LcdRendererOutOfGameTest, KeepsTheNameOfTheLastAuthSuccessAfterOtherMessages)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};
    lcdRenderer.Apply(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_01"));
    lcdRenderer.Apply(HubMessage{"buzz_accepted"});
    lcdRenderer.Apply(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_02"));
    lcdRenderer.Apply(HubMessage{"buzz_accepted"});

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_02").WithLinkCut(), ScreenPresenter{}.ScreenForOutOfGame())).Times(1);

    lcdRenderer.ShowOutOfGame();
}

TEST(LcdRendererOutOfGameTest, ShowsAnEmptyNameWhenNoAuthSuccessWasEverReceived)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("").WithLinkCut(), ScreenPresenter{}.ScreenForOutOfGame())).Times(1);

    lcdRenderer.ShowOutOfGame();
}

TEST(LcdRendererLinkLostTest, ShowsTheConnectionLostScreenWithTheCutLinkAndTheLastKnownName)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};
    lcdRenderer.Apply(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_01"));

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01").WithLinkCut(), ScreenPresenter{}.ScreenForConnectionLost(1))).Times(1);

    lcdRenderer.ShowLinkLost(1);
}

TEST(LcdRendererLinkLostTest, NumbersTheAttemptItIsGiven)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(::testing::_, ScreenPresenter{}.ScreenForConnectionLost(3))).Times(1);

    lcdRenderer.ShowLinkLost(3);
}

TEST(LcdRendererLinkLostTest, KeepsTheNameOfTheLastAuthSuccessAfterOtherMessages)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};
    lcdRenderer.Apply(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_02"));
    lcdRenderer.Apply(HubMessage{"buzz_accepted"});

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_02").WithLinkCut(), ::testing::_)).Times(1);

    lcdRenderer.ShowLinkLost(1);
}

TEST(LcdRendererLinkLostTest, ShowsAnEmptyNameWhenNoAuthSuccessWasEverReceived)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("").WithLinkCut(), ::testing::_)).Times(1);

    lcdRenderer.ShowLinkLost(1);
}

TEST(LcdRendererLinkLostTest, AnAuthSuccessAfterTheLossShowsALinkedTopBarAgain)
{
    MockScreen mockScreen;
    testing::NiceMock<MockArmedModeReader> armedModeReader;
    VisibleTopBar visibleTopBar;
    LcdRenderer lcdRenderer{mockScreen, armedModeReader, visibleTopBar};
    lcdRenderer.Apply(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_01"));
    lcdRenderer.ShowLinkLost(1);

    EXPECT_CALL(mockScreen, Display(TopBar::ForPlayer("quiz_buzzer_01"), ScreenModel{TextScreen{"En attente du maître du jeu\nLa partie va bientôt commencer"}})).Times(1);

    lcdRenderer.Apply(HubMessage{"auth_success"}.WithUsername("quiz_buzzer_01"));
}
