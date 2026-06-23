#include <Arduino.h>
#include <WebSocketsClient.h>
#include <WiFi.h>
#include <vector>
#include "ArduinoGpioPinReader.h"
#include "ArduinoLocalFeedback.h"
#include "ReconnectionPolicy.h"
#include "WebSocketPayloadSocketWriter.h"
#include "WebSocketMessageSender.h"
#include "PhysicalButtonReader.h"
#include "BuzzerBehavior.h"
#include "TokenSessionMessageSender.h"
#include "BuzzerButtonPressTranslator.h"
#include "HubMessageDispatcher.h"
#include "ButtonPressDetector.h"
#include "ConnectionEventHandler.h"
#include "BuzzerEventTranslator.h"
#include "WebSocketEventListener.h"

ArduinoGpioPinReader pinAReader(1);
ArduinoGpioPinReader pinBReader(2);
ArduinoGpioPinReader pinCReader(3);
ArduinoGpioPinReader pinDReader(4);
ArduinoGpioPinReader pinBuzzReader(5);
std::vector<ButtonPin> buttonPins{
    {pinAReader, ButtonInput::A},
    {pinBReader, ButtonInput::B},
    {pinCReader, ButtonInput::C},
    {pinDReader, ButtonInput::D},
    {pinBuzzReader, ButtonInput::Buzz},
};
ArduinoLocalFeedback localFeedback{{6, 7, 8, 9}};
ReconnectionPolicy reconnectionPolicy;
WebSocketsClient client;
WebSocketPayloadSocketWriter payloadSocketWriter{client};
WebSocketMessageSender messageSender{MessageSerializer{}, client};
PhysicalButtonReader physicalButtonReader{buttonPins};
BuzzerBehavior buzzerBehavior{messageSender, localFeedback};
TokenSessionMessageSender tokenSessionMessageSender{payloadSocketWriter, MessageSerializer{}};
BuzzerButtonPressTranslator buzzerButtonPressTranslator{buzzerBehavior};
HubMessageDispatcher hubMessageDispatcher{buzzerBehavior};
ButtonPressDetector buttonPressDetector{physicalButtonReader, buzzerButtonPressTranslator};
ConnectionEventHandler connectionEventHandler{reconnectionPolicy, hubMessageDispatcher, tokenSessionMessageSender, tokenSessionMessageSender};
BuzzerEventTranslator buzzerEventTranslator{connectionEventHandler};
WebSocketEventListener webSocketEventListener{client, buzzerEventTranslator};

void setup()
{
    pinAReader.Begin();
    pinBReader.Begin();
    pinCReader.Begin();
    pinDReader.Begin();
    pinBuzzReader.Begin();
    localFeedback.Begin();
    webSocketEventListener.Begin();
    WiFi.begin("VOTRE_SSID", "VOTRE_MOT_DE_PASSE");
    while (WiFi.status() != WL_CONNECTED)
        delay(500);
    client.begin("192.168.1.83", 8282, "/ws");
}

void loop()
{
    webSocketEventListener.Loop();
    buttonPressDetector.Poll();
}