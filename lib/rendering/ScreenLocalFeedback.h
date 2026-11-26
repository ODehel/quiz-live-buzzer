#ifndef SCREEN_LOCAL_FEEDBACK_H
#define SCREEN_LOCAL_FEEDBACK_H

#include <variant>
#include "LastScreenReader.h"
#include "LocalFeedback.h"
#include "Screen.h"
#include "ScreenPresenter.h"
#include "VisibleTopBar.h"

class ScreenLocalFeedback : public LocalFeedback
{
private:
    Screen &screen;
    const VisibleTopBar &visibleTopBar;
    const LastScreenReader &lastScreenReader;
    ScreenPresenter presenter;
    ScreenModel screenBeforePress = ScreenUnchanged{};

public:
    ScreenLocalFeedback(Screen &screen, const VisibleTopBar &visibleTopBar, const LastScreenReader &lastScreenReader)
        : screen(screen), visibleTopBar(visibleTopBar), lastScreenReader(lastScreenReader)
    {
    }

    void AcknowledgeAnswer(char value) override
    {
        screenBeforePress = lastScreenReader.LastScreen();
        const ScreenModel screenModel = presenter.ScreenForTappedAnswer(value);
        if (!std::holds_alternative<ScreenUnchanged>(screenModel))
            screen.Display(visibleTopBar.Current(), screenModel);
    }

    void AcknowledgeBuzz() override
    {
        screenBeforePress = lastScreenReader.LastScreen();
        screen.Display(visibleTopBar.Current(), presenter.ScreenForBuzzSent());
    }

    // L'écran qu'avait le joueur au moment de l'appui, relevé avant le feedback (US-FW-002 CA-10, US-FW-003 CA-11) : pour une question à choix multiples,
    // les propositions redeviennent tactiles (`TappableZonesScreen`) et aucune zone n'est choisie (`RetainingScreen`).
    void RestoreScreenBeforePress() override
    {
        screen.Display(visibleTopBar.Current(), screenBeforePress);
    }
};

#endif
