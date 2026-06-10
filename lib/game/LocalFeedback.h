#ifndef LOCAL_FEEDBACK_H
#define LOCAL_FEEDBACK_H

class LocalFeedback
{
public:
    virtual ~LocalFeedback() = default;
    virtual void Acknowledge() = 0;
};

#endif