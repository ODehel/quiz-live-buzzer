#include <Arduino.h>
#include <string>
#include <WebSocketsClient.h>
#include <WiFi.h>
#include <vector>
#include <HTTPClient.h>
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
#include "TouchZoneReader.h"
#include "Gt911TouchReader.h"
#include "TouchButtonReader.h"
#include "ServerCredentials.h"
#include "LoginRequestSerializer.h"
#include "LoginResponseDeserializer.h"

ArduinoGpioPinReader pinBuzzReader(38);
std::vector<ButtonPin> buttonPins{{pinBuzzReader, ButtonInput::Buzz}};

// DETTE: zones figées en 2x2 provisoire pour valider la chaîne tactile au smoke.
// La disposition réelle est DYNAMIQUE par écran (change à chaque rendu LVGL, §8.2) :
// ce vecteur devra être réécrit/piloté par l'état de jeu, pas codé en dur ici.
std::vector<TouchZone> touchZones{
    {0, 0, 400, 240, ButtonInput::A},
    {400, 0, 800, 240, ButtonInput::B},
    {0, 240, 400, 480, ButtonInput::C},
    {400, 240, 800, 480, ButtonInput::D}};
Gt911TouchReader gt911;
TouchZoneReader touchZoneReader{touchZones};
TouchButtonReader touchButtonReader{gt911, touchZoneReader};
// DETTE: pins 6/7/8/9 provisoires (non câblés sur le hardware réel) — à fixer au plan de câblage feedback
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
ButtonPressDetector touchPressDetector{touchButtonReader, buzzerButtonPressTranslator};
ConnectionEventHandler connectionEventHandler{reconnectionPolicy, hubMessageDispatcher, tokenSessionMessageSender, tokenSessionMessageSender};
BuzzerEventTranslator buzzerEventTranslator{connectionEventHandler};
WebSocketEventListener webSocketEventListener{client, buzzerEventTranslator};

static std::string fetchToken(const String &hubUrl, const ServerCredentials &credentials)
{
    std::string token = "";
    LoginRequestSerializer loginRequestSerializer;
    std::string requestBody = loginRequestSerializer.SerializeLogin(credentials);
    HTTPClient http;
    http.begin(hubUrl + "/api/v1/token");
    http.addHeader("Content-Type", "application/json");
    int status = http.POST(String(requestBody.c_str()));
    if (status == 200)
    {
        std::string responseBody = http.getString().c_str();
        LoginResponseDeserializer loginResponseDeserializer;
        token = loginResponseDeserializer.DeserializeToken(responseBody);
    }
    else
    {
        Serial.print("Login failed, HTTP status: "); Serial.println(status);
    }
    http.end();

    return token;
}

void setup()
{
    gt911.Begin();
    pinBuzzReader.Begin();
    localFeedback.Begin();
    webSocketEventListener.Begin();
    WiFi.begin("VOTRE_SSID", "VOTRE_MOT_DE_PASSE"); // DETTE: Devra être renseigné par les valeurs reçues par le module NFC
    while (WiFi.status() != WL_CONNECTED)
        delay(500);
    std::string token = fetchToken("http://192.168.1.83:8282", ServerCredentials{"user", "password"});
    tokenSessionMessageSender.UpdateToken(token);
    client.begin("192.168.1.83", 8282, "/ws");
}

void loop()
{
    webSocketEventListener.Loop();
    buttonPressDetector.Poll();
    touchPressDetector.Poll();
}