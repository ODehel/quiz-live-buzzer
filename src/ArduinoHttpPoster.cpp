#include <Arduino.h>
#include <HTTPClient.h>
#include "ArduinoHttpPoster.h"

HttpResponse ArduinoHttpPoster::PostJson(const std::string &url, const std::string &body)
{
    HTTPClient http;
    http.begin(String(url.c_str()));
    http.addHeader("Content-Type", "application/json");
    int status = http.POST(String(body.c_str()));
    std::string responseBody = http.getString().c_str();
    http.end();

    Serial.print("Login HTTP status: ");
    Serial.print(status);
    Serial.print(", body size: ");
    Serial.println(responseBody.size());

    return HttpResponse{status, responseBody};
}
