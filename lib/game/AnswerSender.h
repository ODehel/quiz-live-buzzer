#ifndef ANSWER_SENDER_H
#define ANSWER_SENDER_H

class AnswerSender
{
public:
    virtual ~AnswerSender() = default;
    virtual void SendAnswer(char value) = 0;
};

#endif