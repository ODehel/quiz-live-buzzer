#ifndef HTTP_TOKEN_FETCHER_H
#define HTTP_TOKEN_FETCHER_H

#include <string>
#include "TokenFetcher.h"
#include "HttpPoster.h"
#include "ServerCredentials.h"
#include "LoginRequestSerializer.h"
#include "LoginResponseDeserializer.h"

class HttpTokenFetcher : public TokenFetcher
{
private:
    HttpPoster &httpPoster;
    const std::string hubUrl;
    const ServerCredentials credentials;

public:
    HttpTokenFetcher(HttpPoster &httpPoster, const std::string &hubUrl, const ServerCredentials &credentials)
        : httpPoster(httpPoster), hubUrl(hubUrl), credentials(credentials)
    {
    }

    LoginResult Fetch() override
    {
        LoginRequestSerializer loginRequestSerializer;
        HttpResponse response = httpPoster.PostJson(hubUrl + "/api/v1/token", loginRequestSerializer.SerializeLogin(credentials));

        if (response.status == 200)
        {
            LoginResponseDeserializer loginResponseDeserializer;
            return LoginResult{LoginStatus::Success, loginResponseDeserializer.DeserializeToken(response.body)};
        }
        if (response.status == 401)
            return LoginResult{LoginStatus::InvalidCredentials, ""};
        return LoginResult{LoginStatus::Failed, ""};
    }
};

#endif
