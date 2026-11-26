#ifndef BUZZER_BEHAVIOR_H
#define BUZZER_BEHAVIOR_H

#include "ArmedModeReader.h"
#include "HubMessageSender.h"
#include "LocalFeedback.h"

enum class Mode
{
    Inert,
    McqArmed,
    SpeedArmed
};

class BuzzerBehavior : public ArmedModeReader
{
private:
    Mode mode = Mode::Inert;
    HubMessageSender &hubMessageSender;
    LocalFeedback &localFeedback;
    bool isEliminated = false;
    bool isLinkCut = false;
    void ArmFromInert(Mode armedMode)
    {
        if (!isEliminated && mode == Mode::Inert)
            mode = armedMode;
    }

    // Le verrou est resté posé jusqu'au retour de l'envoi (US-FW-002 CA-3) ; un message qui n'est pas parti est réarmé dans le mode d'origine,
    // l'écran de la question revient, sans réessai : le joueur rappuie s'il le veut (US-FW-002 CA-10, US-FW-003 CA-11, décision #91).
    void RearmAfterUnsentPress(Mode pressedMode)
    {
        mode = pressedMode;
        localFeedback.RestoreScreenBeforePress();
    }

public:
    BuzzerBehavior(HubMessageSender &hubMessageSender, LocalFeedback &localFeedback) : hubMessageSender(hubMessageSender), localFeedback(localFeedback)
    {
    }

    bool IsArmed() const override
    {
        return mode != Mode::Inert;
    }

    void OnMcqPressed(char value)
    {
        if (isEliminated || isLinkCut)
            return;

        if (mode == Mode::McqArmed)
        {
            mode = Mode::Inert;
            localFeedback.AcknowledgeAnswer(value);
            if (!hubMessageSender.SendAnswer(value))
                RearmAfterUnsentPress(Mode::McqArmed);
        }
    }

    void OnBuzzPressed()
    {
        if (isEliminated || isLinkCut)
            return;

        if (mode == Mode::SpeedArmed)
        {
            mode = Mode::Inert;
            localFeedback.AcknowledgeBuzz();
            if (!hubMessageSender.SendBuzz())
                RearmAfterUnsentPress(Mode::SpeedArmed);
        }
    }

    void OnQuestionChoices()
    {
        ArmFromInert(Mode::McqArmed);
    }

    void OnQuestionOpen()
    {
        ArmFromInert(Mode::SpeedArmed);
    }

    void OnTimerEnd()
    {
        mode = Mode::Inert;
    }

    void OnBuzzLocked()
    {
        if (mode == Mode::SpeedArmed)
            mode = Mode::Inert;
    }

    void OnBuzzUnlocked()
    {
        ArmFromInert(Mode::SpeedArmed);
    }

    // Pendant la coupure (de la perte de la connexion établie à l'`auth_success` suivant), un appui est ignoré : ni envoi, ni verrou,
    // ni feedback local, et le mode est conservé (US-FW-015 CA-4, CA-7).
    void OnLinkLost()
    {
        isLinkCut = true;
    }

    void OnLinkEstablished()
    {
        isLinkCut = false;
    }

    void OnEliminated()
    {
        isEliminated = true;
        mode = Mode::Inert;
    }
};

#endif