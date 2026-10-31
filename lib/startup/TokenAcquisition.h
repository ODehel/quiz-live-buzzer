#ifndef TOKEN_ACQUISITION_H
#define TOKEN_ACQUISITION_H

#include "LoginResult.h"
#include "TokenFetcher.h"
#include "Sleeper.h"

class TokenAcquisition
{
private:
    TokenFetcher &tokenFetcher;
    Sleeper &sleeper;
    const unsigned long retryDelayMilliseconds;

public:
    TokenAcquisition(TokenFetcher &tokenFetcher, Sleeper &sleeper, unsigned long retryDelayMilliseconds)
        : tokenFetcher(tokenFetcher), sleeper(sleeper), retryDelayMilliseconds(retryDelayMilliseconds)
    {
    }

    LoginResult Acquire()
    {
        while (true)
        {
            LoginResult result = tokenFetcher.Fetch();
            if (result.status == LoginStatus::InvalidCredentials)
                return result;
            if (result.status == LoginStatus::Success && !result.token.empty())
                return result;
            sleeper.Sleep(retryDelayMilliseconds);
        }
    }
};

#endif
