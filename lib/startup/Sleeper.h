#ifndef SLEEPER_H
#define SLEEPER_H

class Sleeper
{
public:
    virtual ~Sleeper() = default;
    virtual void Sleep(unsigned long milliseconds) = 0;
};

#endif
