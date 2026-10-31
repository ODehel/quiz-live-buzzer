#include <Arduino.h>
#include <string>
#include <WebSocketsClient.h>
#include <WiFi.h>
#include <vector>
#include "LcdRenderer.h"
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
#include "HttpTokenFetcher.h"
#include "ArduinoSleeper.h"
#include "TokenAcquisition.h"
#include "secrets.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include "GfxScreen.h"

class LGFX : public lgfx::LGFX_Device
{
public:
    lgfx::Bus_RGB _bus_instance;
    lgfx::Panel_RGB _panel_instance;
    LGFX(void)
    {
        {
            auto cfg = _bus_instance.config();
            cfg.panel = &_panel_instance;
            cfg.pin_d0 = GPIO_NUM_8;
            cfg.pin_d1 = GPIO_NUM_3;
            cfg.pin_d2 = GPIO_NUM_46;
            cfg.pin_d3 = GPIO_NUM_9;
            cfg.pin_d4 = GPIO_NUM_1; // B0-B4
            cfg.pin_d5 = GPIO_NUM_5;
            cfg.pin_d6 = GPIO_NUM_6;
            cfg.pin_d7 = GPIO_NUM_7;
            cfg.pin_d8 = GPIO_NUM_15;
            cfg.pin_d9 = GPIO_NUM_16;
            cfg.pin_d10 = GPIO_NUM_4; // G0-G5
            cfg.pin_d11 = GPIO_NUM_45;
            cfg.pin_d12 = GPIO_NUM_48;
            cfg.pin_d13 = GPIO_NUM_47;
            cfg.pin_d14 = GPIO_NUM_21;
            cfg.pin_d15 = GPIO_NUM_14; // R0-R4
            cfg.pin_henable = GPIO_NUM_40;
            cfg.pin_vsync = GPIO_NUM_41;
            cfg.pin_hsync = GPIO_NUM_39;
            cfg.pin_pclk = GPIO_NUM_0;
            cfg.freq_write = 15000000;
            cfg.hsync_polarity = 0;
            cfg.hsync_front_porch = 8;
            cfg.hsync_pulse_width = 4;
            cfg.hsync_back_porch = 43;
            cfg.vsync_polarity = 0;
            cfg.vsync_front_porch = 8;
            cfg.vsync_pulse_width = 4;
            cfg.vsync_back_porch = 12;
            cfg.pclk_active_neg = 1;
            cfg.de_idle_high = 0;
            cfg.pclk_idle_high = 0;
            _bus_instance.config(cfg);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.memory_width = 800;
            cfg.memory_height = 480;
            cfg.panel_width = 800;
            cfg.panel_height = 480;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            _panel_instance.config(cfg);
        }
        _panel_instance.setBus(&_bus_instance);
        setPanel(&_panel_instance);
    }
};
LGFX lcd;

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
GfxScreen gfxScreen{lcd};
LcdRenderer lcdRenderer{gfxScreen};
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
ConnectionEventHandler connectionEventHandler{reconnectionPolicy, hubMessageDispatcher, tokenSessionMessageSender, tokenSessionMessageSender, lcdRenderer};
BuzzerEventTranslator buzzerEventTranslator{connectionEventHandler};
WebSocketEventListener webSocketEventListener{client, buzzerEventTranslator};

constexpr unsigned long LOGIN_RETRY_DELAY_MS = 5000;
HttpTokenFetcher httpTokenFetcher{"http://192.168.1.183:3000", ServerCredentials{HUB_USERNAME, HUB_PASSWORD}};
ArduinoSleeper arduinoSleeper;
TokenAcquisition tokenAcquisition{httpTokenFetcher, arduinoSleeper, LOGIN_RETRY_DELAY_MS};
bool startupHalted = false;

void setup()
{
    Serial.begin(115200);
    gt911.Begin();
    pinBuzzReader.Begin();
    // localFeedback.Begin();   // DETTE §6 : pins 6/7/8/9 en collision avec le bus RGB — voir décision ci-dessous
    webSocketEventListener.Begin();

    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH); // backlight dalle (GPIO2)
    lcd.init();            // pilote la dalle DIS07050H
    lcd.fillScreen(TFT_BLACK);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
        delay(500);
    LoginResult login = tokenAcquisition.Acquire();
    if (login.status == LoginStatus::InvalidCredentials)
    {
        Serial.println("Login refused: invalid credentials, check secrets.h. Startup halted.");
        startupHalted = true;
        return;
    }
    Serial.println("Login OK, opening the WebSocket.");
    tokenSessionMessageSender.UpdateToken(login.token);
    client.begin("192.168.1.183", 3000, "/ws");
}

void loop()
{
    if (startupHalted)
        return;
    webSocketEventListener.Loop();
    buttonPressDetector.Poll();
    touchPressDetector.Poll();
}