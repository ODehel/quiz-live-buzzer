#ifndef ARDUINO_HTTP_POSTER_H
#define ARDUINO_HTTP_POSTER_H

#include "HttpPoster.h"

class ArduinoHttpPoster : public HttpPoster
{
public:
    HttpResponse PostJson(const std::string &url, const std::string &body) override;
};

#endif
