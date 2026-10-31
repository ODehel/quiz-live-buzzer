#ifndef TOKEN_FETCHER_H
#define TOKEN_FETCHER_H

#include "LoginResult.h"

class TokenFetcher
{
public:
    virtual ~TokenFetcher() = default;
    virtual LoginResult Fetch() = 0;
};

#endif
