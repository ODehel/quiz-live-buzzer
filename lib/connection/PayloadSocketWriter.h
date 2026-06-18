#ifndef PAYLOAD_SOCKET_WRITER_H
#define PAYLOAD_SOCKET_WRITER_H

#include <string>

class PayloadSocketWriter
{
public:
    virtual ~PayloadSocketWriter() = default;
    virtual void Write(const std::string &payload) = 0;
};

#endif