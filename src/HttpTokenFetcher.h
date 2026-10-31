#ifndef HTTP_TOKEN_FETCHER_H
#define HTTP_TOKEN_FETCHER_H

#include <string>
#include "TokenFetcher.h"
#include "ServerCredentials.h"

class HttpTokenFetcher : public TokenFetcher
{
private:
    const std::string hubUrl;
    const ServerCredentials credentials;

public:
    HttpTokenFetcher(const std::string &hubUrl, const ServerCredentials &credentials)
        : hubUrl(hubUrl), credentials(credentials)
    {
    }

    LoginResult Fetch() override;
};

#endif
