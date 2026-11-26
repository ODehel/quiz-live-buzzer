#include <Arduino.h>
#include <string>
#include <WebSocketsClient.h>
#include <WiFi.h>
#include <vector>
#include "LcdRenderer.h"
#include "ArduinoGpioPinReader.h"
#include "ReconnectionPolicy.h"
#include "WebSocketPayloadSocketWriter.h"
#include "WebSocketMessageSender.h"
#include "PhysicalButtonReader.h"
#include "BuzzerBehavior.h"
#include "ScreenLocalFeedback.h"
#include "VisibleTopBar.h"
#include "TokenSessionMessageSender.h"
#include "LoginRefusalHalt.h"
#include "NetworkHalt.h"
#include "WebSocketConnectionCloser.h"
#include "BuzzerButtonPressTranslator.h"
#include "HubMessageDispatcher.h"
#include "ButtonPressDetector.h"
#include "ConnectionEventHandler.h"
#include "BuzzerEventTranslator.h"
#include "WebSocketEventListener.h"
#include "TouchZoneReader.h"
#include "TappableZonesScreen.h"
#include "RetainingScreen.h"
#include "Gt911TouchReader.h"
#include "TouchButtonReader.h"
#include "ServerCredentials.h"
#include "ArduinoHttpPoster.h"
#include "ArduinoClock.h"
#include "TaskAsyncLogin.h"
#include "SessionPollingNetworkLoop.h"
#include "HttpTokenFetcher.h"
#include "ArduinoSleeper.h"
#include "TokenAcquisition.h"
#include "LoginOutcomeDisplay.h"
#include "InitScreenDisplay.h"
#include "StartupSequence.h"
#include "BuzzerLoop.h"
#include "WebSocketConnectionOpener.h"
#include "secrets.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include "GfxScreen.h"

// Taille de la dalle : source unique de la configuration LGFX et des zones tactiles (`TappableZonesScreen`).
constexpr int PanelWidth = 800;
constexpr int PanelHeight = 480;

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
            cfg.memory_width = PanelWidth;
            cfg.memory_height = PanelHeight;
            cfg.panel_width = PanelWidth;
            cfg.panel_height = PanelHeight;
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

Gt911TouchReader gt911;
GfxScreen gfxScreen{lcd};
// Les zones tactiles suivent l'affichage : actives seulement quand les propositions A–D sont à l'écran (CDC §8.2).
TappableZonesScreen tappableZonesScreen{gfxScreen, PanelWidth, PanelHeight};
// Retient le dernier écran affiché, quel qu'en soit le producteur (message du hub ou feedback local), pour l'atténuer sous l'écran de coupure
// (US-FW-015 CA-2) ; `LcdRenderer` et `ScreenLocalFeedback` passent par lui, le refus du login (terminal) reste direct.
RetainingScreen retainingScreen{tappableZonesScreen};
TouchZoneReader touchZoneReader{tappableZonesScreen.Zones()};
TouchButtonReader touchButtonReader{gt911, touchZoneReader};
// Le bandeau du haut visible, partagé : `LcdRenderer` l'écrit, `ScreenLocalFeedback` le lit (US-FW-009 CA-3).
VisibleTopBar visibleTopBar;
ScreenLocalFeedback localFeedback{retainingScreen, visibleTopBar, retainingScreen};
ReconnectionPolicy reconnectionPolicy;
WebSocketsClient client;
WebSocketPayloadSocketWriter payloadSocketWriter{client};
WebSocketMessageSender messageSender{MessageSerializer{}, client};
PhysicalButtonReader physicalButtonReader{pinBuzzReader};
BuzzerBehavior buzzerBehavior{messageSender, localFeedback};
LcdRenderer lcdRenderer{retainingScreen, buzzerBehavior, visibleTopBar};
ArduinoHttpPoster arduinoHttpPoster;
HttpTokenFetcher httpTokenFetcher{arduinoHttpPoster, "http://192.168.1.183:3000", ServerCredentials{HUB_USERNAME, HUB_PASSWORD}};
// Passe par `tappableZonesScreen` pour que les zones tactiles restent masquées sur l'écran de refus.
LoginOutcomeDisplay loginOutcomeDisplay{tappableZonesScreen};
// L'écran d'init passe aussi par `tappableZonesScreen` : zones masquées (US-FW-010 CA-1, CA-2).
InitScreenDisplay initScreenDisplay{tappableZonesScreen};
// Arrêt terminal du réseau (US-FW-006 CA-10) : ferme le client et fait que `loop()` ne fait plus rien. Deux causes, un
// seul mécanisme : l'élimination (`ConnectionEventHandler`) et un `401` au login d'une reconnexion (`LoginRefusalHalt`).
WebSocketConnectionCloser connectionCloser{client};
NetworkHalt networkHalt{connectionCloser};
LoginRefusalHalt loginRefusalHalt{loginOutcomeDisplay, networkHalt};
// Le login en jeu (à l'ouverture du WebSocket, au renouvellement) tourne dans une tâche, borné à 5 s depuis son lancement (US-FW-005 CA-8, CA-9) :
// `loop()` continue pendant ce temps. Le login de `setup()` reste bloquant (`TokenAcquisition`).
constexpr unsigned long LOGIN_TIMEOUT_MS = 5000;
TaskAsyncLogin taskAsyncLogin{httpTokenFetcher};
ArduinoClock arduinoClock;
TokenSessionMessageSender tokenSessionMessageSender{payloadSocketWriter, MessageSerializer{}, taskAsyncLogin, arduinoClock, LOGIN_TIMEOUT_MS, loginRefusalHalt};
BuzzerButtonPressTranslator buzzerButtonPressTranslator{buzzerBehavior};
HubMessageDispatcher hubMessageDispatcher{buzzerBehavior};
ButtonPressDetector buttonPressDetector{physicalButtonReader, buzzerButtonPressTranslator};
ButtonPressDetector touchPressDetector{touchButtonReader, buzzerButtonPressTranslator};
ConnectionEventHandler connectionEventHandler{reconnectionPolicy, hubMessageDispatcher, tokenSessionMessageSender, lcdRenderer, lcdRenderer, lcdRenderer, networkHalt};
BuzzerEventTranslator buzzerEventTranslator{connectionEventHandler};
WebSocketEventListener webSocketEventListener{client, buzzerEventTranslator};
// Un tour de réseau : le WebSocket, puis le login en vol (résultat, délai de 5 s).
SessionPollingNetworkLoop sessionPollingNetworkLoop{webSocketEventListener, connectionEventHandler};

constexpr unsigned long LOGIN_RETRY_DELAY_MS = 5000;
// US-FW-006 CA-8 : 5 s fixes entre deux tentatives de reconnexion (la bibliothèque rouvre toutes les 500 ms par défaut).
constexpr unsigned long RECONNECT_INTERVAL_MS = 5000;
ArduinoSleeper arduinoSleeper;
TokenAcquisition tokenAcquisition{httpTokenFetcher, arduinoSleeper, LOGIN_RETRY_DELAY_MS};
WebSocketConnectionOpener webSocketConnectionOpener{client, "192.168.1.183", 3000, "/ws"};
// Séquence de `setup()` après le Wi-Fi (US-FW-005) et tour de `loop()` : leur logique vit dans `lib/startup/`, ici on les câble.
StartupSequence startupSequence{tokenAcquisition, loginOutcomeDisplay, tokenSessionMessageSender, webSocketConnectionOpener, RECONNECT_INTERVAL_MS};
BuzzerLoop buzzerLoop{startupSequence, networkHalt, sessionPollingNetworkLoop, buttonPressDetector, touchPressDetector};

void setup()
{
    Serial.begin(115200);
    gt911.Begin();
    pinBuzzReader.Begin();
    webSocketEventListener.Begin();

    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH); // backlight dalle (GPIO2)
    lcd.init();            // pilote la dalle DIS07050H
    lcd.fillScreen(TFT_BLACK);
    initScreenDisplay.Show(); // avant toute attente réseau : il reste jusqu'au premier message rendu (US-FW-010 CA-1, CA-2)

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
        delay(500);
    if (startupSequence.Run() == StartupOutcome::Halted)
        Serial.println("Login refused: invalid credentials, check secrets.h. Startup halted.");
    else
        Serial.println("Login OK, opening the WebSocket.");
}

void loop()
{
    buzzerLoop.Tick();
}