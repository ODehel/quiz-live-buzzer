#ifndef SCREEN_PRESENTER_H
#define SCREEN_PRESENTER_H

#include "DisplayState.h"
#include "HubMessage.h"
#include "ScreenModel.h"
#include "TopBar.h"

class ScreenPresenter
{
private:
    DisplayState displayState;

public:
    ScreenModel ScreenFor(const HubMessage &message, bool questionArmed) const
    {
        if (message.Type() == "timer_end")
            return questionArmed ? ScreenModel{TextScreen{displayState.LcdTextForTimeUp()}} : ScreenModel{ScreenUnchanged{}};
        // Un buzzer déjà armé ignore `question_open` et `buzz_unlocked` en entier (US-FW-002 CA-2) : l'écran ne bouge pas, seul un message qui arme est rendu (US-FW-012 CA-13).
        if (questionArmed && (message.Type() == "question_open" || message.Type() == "buzz_unlocked"))
            return ScreenUnchanged{};
        const std::vector<std::string> zones = displayState.ZoneTextsFor(message);
        // Un buzzer déjà armé ignore `question_choices` (US-FW-002 CA-2) : ses zones ne seraient pas actives, l'écran ne bouge pas (US-FW-011 CA-6).
        if (!zones.empty())
            return questionArmed ? ScreenModel{ScreenUnchanged{}} : ScreenModel{ZonesScreen{zones}};
        const std::string text = displayState.LcdTextFor(message);
        if (displayState.KeepsZonesFor(message))
            return BannerScreen{text};
        const int highlightedZone = displayState.HighlightedZoneFor(message);
        if (highlightedZone != DisplayState::NoZone)
            return HighlightedZoneScreen{static_cast<size_t>(highlightedZone), text};
        if (text.empty())
            return ScreenUnchanged{};
        return TextScreen{text};
    }

    // Seul l'écran « Connecté » porte le bandeau du haut (CDC §8.2) : le nom vient de l'`username` d'`auth_success`.
    TopBar TopBarFor(const HubMessage &message) const
    {
        if (message.Type() == "auth_success")
            return TopBar::ForPlayer(message.Username());
        return TopBar::Hidden();
    }

    // Le bandeau de l'init : visible dès la mise sous tension, nom vide, liaison « coupée » (US-FW-010 CA-1, US-FW-009 CA-3 et CA-4).
    TopBar TopBarForInit() const
    {
        return TopBar::ForPlayer("").WithLinkCut();
    }

    ScreenModel ScreenForBuzzSent() const
    {
        return TextScreen{displayState.LcdTextForBuzzSent()};
    }

    ScreenModel ScreenForInit() const
    {
        return TextScreen{displayState.LcdTextForInit()};
    }

    ScreenModel ScreenForLoginRefused() const
    {
        return TextScreen{displayState.LcdTextForLoginRefused()};
    }

    ScreenModel ScreenForOutOfGame() const
    {
        return OutOfGameScreen{displayState.OutOfGameTitle(), displayState.OutOfGameCause(), displayState.OutOfGameWayOut()};
    }

    // Le bandeau de coupure de la tentative en cours ; le dernier écran à atténuer est ajouté par `RetainingScreen`.
    ScreenModel ScreenForConnectionLost(int attempt) const
    {
        return ConnectionLostScreen{displayState.ConnectionLostTitle(), displayState.ConnectionLostAttemptLabel(attempt), attempt, DisplayState::ReconnectionAttemptCount};
    }

    ScreenModel ScreenForTappedAnswer(char letter) const
    {
        const int zone = displayState.ZoneIndexForAnswer(letter);
        if (zone == DisplayState::NoZone)
            return ScreenUnchanged{};
        return ChosenZoneScreen{static_cast<size_t>(zone)};
    }
};

#endif
