#ifndef CLOCK_H
#define CLOCK_H

// Le temps écoulé depuis la mise sous tension, en millisecondes. Le compteur peut déborder : on ne l'utilise que par différence (non signée).
class Clock
{
public:
    virtual ~Clock() = default;
    virtual unsigned long NowMilliseconds() = 0;
};

#endif
