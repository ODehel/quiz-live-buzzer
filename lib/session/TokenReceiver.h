#ifndef TOKEN_RECEIVER_H
#define TOKEN_RECEIVER_H

#include <string>

class TokenReceiver
{
public:
    virtual ~TokenReceiver() = default;
    virtual void UpdateToken(const std::string &token) = 0;
};

#endif