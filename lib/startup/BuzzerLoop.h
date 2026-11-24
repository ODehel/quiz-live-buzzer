#ifndef BUZZER_LOOP_H
#define BUZZER_LOOP_H

#include "ButtonPressDetector.h"
#include "NetworkHalt.h"
#include "NetworkLoop.h"
#include "StartupSequence.h"

// Un tour de `loop()` : le réseau puis les deux lectures d'appui (bouton physique, tactile). Rien du tout quand le
// démarrage est arrêté par un login refusé (US-FW-005 CA-5) ou quand le réseau est arrêté (élimination, `401` en reconnexion).
class BuzzerLoop
{
private:
    const StartupSequence &startupSequence;
    const NetworkHalt &networkHalt;
    NetworkLoop &networkLoop;
    ButtonPressDetector &buttonPressDetector;
    ButtonPressDetector &touchPressDetector;

public:
    BuzzerLoop(const StartupSequence &startupSequence, const NetworkHalt &networkHalt, NetworkLoop &networkLoop, ButtonPressDetector &buttonPressDetector, ButtonPressDetector &touchPressDetector)
        : startupSequence(startupSequence), networkHalt(networkHalt), networkLoop(networkLoop), buttonPressDetector(buttonPressDetector), touchPressDetector(touchPressDetector)
    {
    }

    void Tick()
    {
        if (startupSequence.IsHalted() || networkHalt.IsHalted())
            return;
        networkLoop.Loop();
        buttonPressDetector.Poll();
        touchPressDetector.Poll();
    }
};

#endif
