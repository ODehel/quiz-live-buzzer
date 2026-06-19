#include "WebSocketPayloadSocketWriter.h"
#include "WebSocketsClient.h"


void WebSocketPayloadSocketWriter::Write(const std::string &payload)
{
    client.sendTXT(payload.c_str());
}