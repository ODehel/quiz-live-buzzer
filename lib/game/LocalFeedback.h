#ifndef LOCAL_FEEDBACK_H
#define LOCAL_FEEDBACK_H

class LocalFeedback
{
public:
    virtual ~LocalFeedback() = default;
    virtual void AcknowledgeAnswer() = 0;
    virtual void AcknowledgeBuzz() = 0;
};

#endif