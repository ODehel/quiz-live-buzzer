#include <Arduino.h>
#include <HTTPClient.h>
#include "HttpTokenFetcher.h"
#include "LoginRequestSerializer.h"
#include "LoginResponseDeserializer.h"

LoginResult HttpTokenFetcher::Fetch()
{
    LoginRequestSerializer loginRequestSerializer;
    std::string requestBody = loginRequestSerializer.SerializeLogin(credentials);

    HTTPClient http;
    http.begin(String((hubUrl + "/api/v1/token").c_str()));
    http.addHeader("Content-Type", "application/json");
    int status = http.POST(String(requestBody.c_str()));

    LoginResult result{LoginStatus::Failed, ""};
    if (status == 200)
    {
        std::string responseBody = http.getString().c_str();
        LoginResponseDeserializer loginResponseDeserializer;
        result = LoginResult{LoginStatus::Success, loginResponseDeserializer.DeserializeToken(responseBody)};
        if (result.token.empty())
        {
            Serial.print("Login answered 200 without a token, body size: ");
            Serial.println(responseBody.size());
        }
    }
    else if (status == 401)
    {
        result = LoginResult{LoginStatus::InvalidCredentials, ""};
    }
    else
    {
        Serial.print("Login failed, HTTP status: ");
        Serial.println(status);
    }
    http.end();

    return result;
}
