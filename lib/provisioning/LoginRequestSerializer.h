#ifndef LOGIN_REQUEST_SERIALIZER_H
#define LOGIN_REQUEST_SERIALIZER_H

#include <string>
#include "ServerCredentials.h"

class LoginRequestSerializer
{
public:
    std::string SerializeLogin(const ServerCredentials &credentials)
    {
        return std::string("{\"username\":\"") + credentials.username + "\",\"password\":\"" + credentials.password + "\"}";
    }
};

#endif