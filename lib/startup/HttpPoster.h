#ifndef HTTP_POSTER_H
#define HTTP_POSTER_H

#include <string>

struct HttpResponse
{
    int status;
    std::string body;
};

class HttpPoster
{
public:
    virtual ~HttpPoster() = default;
    virtual HttpResponse PostJson(const std::string &url, const std::string &body) = 0;
};

#endif
