#ifndef LOGIN_RESULT_H
#define LOGIN_RESULT_H

#include <string>

enum class LoginStatus
{
    Success,
    InvalidCredentials,
    Failed
};

struct LoginResult
{
    LoginStatus status;
    std::string token;
};

#endif
