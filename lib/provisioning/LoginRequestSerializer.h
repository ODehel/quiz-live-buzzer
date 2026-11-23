#ifndef LOGIN_REQUEST_SERIALIZER_H
#define LOGIN_REQUEST_SERIALIZER_H

#include <string>
#include "ServerCredentials.h"

class LoginRequestSerializer
{
public:
    std::string SerializeLogin(const ServerCredentials &credentials)
    {
        return std::string("{\"username\":\"") + Escape(credentials.username) + "\",\"password\":\"" + Escape(credentials.password) + "\"}";
    }

private:
    static std::string Escape(const std::string &value)
    {
        std::string escaped;
        for (char character : value)
        {
            if (character == '"' || character == '\\')
            {
                escaped += '\\';
            }
            escaped += character;
        }
        return escaped;
    }
};

#endif