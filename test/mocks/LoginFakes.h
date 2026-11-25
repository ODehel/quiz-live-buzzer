#ifndef LOGIN_FAKES_H
#define LOGIN_FAKES_H

#include <optional>
#include "AsyncLogin.h"
#include "Clock.h"

// Un login non bloquant sous le contrôle du test : `Finish` pose le résultat d'un login en vol, `Poll` le rend une fois.
// Un login abandonné (ou jamais démarré) ne rend rien : comme l'adaptateur, qui jette le résultat d'un login abandonné.
class FakeAsyncLogin : public AsyncLogin
{
private:
    bool inFlight = false;
    std::optional<LoginResult> finished;

public:
    int starts = 0;
    int abandons = 0;

    void Start() override
    {
        starts++;
        inFlight = true;
        finished.reset();
    }

    std::optional<LoginResult> Poll() override
    {
        if (!inFlight || !finished)
            return std::nullopt;
        inFlight = false;
        std::optional<LoginResult> result = finished;
        finished.reset();
        return result;
    }

    void Abandon() override
    {
        abandons++;
        inFlight = false;
        finished.reset();
    }

    bool IsInFlight() const
    {
        return inFlight;
    }

    void Finish(const LoginResult &result)
    {
        if (inFlight)
            finished = result;
    }
};

class FakeClock : public Clock
{
private:
    unsigned long now = 0;

public:
    unsigned long NowMilliseconds() override
    {
        return now;
    }

    void Advance(unsigned long milliseconds)
    {
        now += milliseconds;
    }

    void SetNow(unsigned long milliseconds)
    {
        now = milliseconds;
    }
};

#endif
