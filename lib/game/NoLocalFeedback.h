#ifndef NO_LOCAL_FEEDBACK_H
#define NO_LOCAL_FEEDBACK_H

#include "LocalFeedback.h"

class NoLocalFeedback : public LocalFeedback
{
public:
    void Acknowledge() override
    {
    }
};

#endif
