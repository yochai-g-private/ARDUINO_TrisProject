#include <dummy.h>
#include <Wire.h>
#include <DS3231.h>
#include <TimeLib.h>
#include <EEPROM.h>
//#include <Hash.h>
//#include <SPI.h>
//#include <ESP8266WiFi.h>
//#include <ESPAsyncTCP.h>
//#include <ESPAsyncWebServer.h>
//#include <FS.h>
#include <Ticker.h>

#include "NYG.h"
#include "Timer.h"
#include "Toggler.h"

#include "PushButton.h"
#include "Observer.h"
#include "RTC.h"

#include "EspTris.h"
#include "WebServices.h"
#include "LedMgr.h"
#include "Motor.h"
#include "Threshold.h"

using namespace NYG;

//static DigitalOutputPin relays(RELAYS_PIN);
//static PullupPushButton button(BUTTON_PIN);
static AnalogInputPin   analog_button(BUTTON_PIN);
static Threshold        button(analog_button, 500);
static DigitalObserver  button_observer(button);

struct ResetAllBeforeSetup
{
    ResetAllBeforeSetup()
    {
        delay(1000);
        resetPin(MANUAL_RELAY_PIN);
        resetPin(MOTOR_RELAY_PIN);
        resetPin(DIRECTION_RELAY_PIN);
    }

    void resetPin(Pin pin)
    {
        pinMode(pin, OUTPUT); digitalWrite(pin, LOW);
    }
};

//static ResetAllBeforeSetup resetor;

//------------------------------------------------------
//	PREDECLARATIONS
//------------------------------------------------------

//------------------------------------------------------
//	SETUP
//------------------------------------------------------

void setup() {

    Logger::Initialize();
    
    //TRACING = true;

    //delay(1000);

    bool RTC_ok = RTC::Begin();

    Signal(RTC_ok);
    Serial.println(RTC_ok ? "RTC is OK" : "RTC initialization failed");

    Settings::Load();

    delay(1000);
    TestLeds();

    //TestRelays();

    InitializeWebServices();

    LOGGER << "Button " << (button_observer.Get() ? "pressed" : "released") << NL;

    LOGGER << "Ready!" << NL;
}
//------------------------------------------------------

//------------------------------------------------------
//	LOOP
//------------------------------------------------------

static void handleWebRequest();

void loop() 
{
    handleWebRequest();

    bool pressed;

    if (button_observer.TestChanged(pressed))
    {
        LOGGER << "Button " << (pressed ? "pressed" : "released") << NL;
    }

    static int x = 0;

    int y = millis() / 10000;

    if (x != y)
    {
        x = y;
        LOGGER << x << NL;
    }
}


extern const char*	gbl_build_date = __DATE__;
extern const char*	gbl_build_time = __TIME__;


#if 1

static const char* ssid = "HomeNet";
static const char* password = "1357864200";

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>

static ESP8266WebServer server(80);

static void handleWebRequest()
{
    server.handleClient();
}

static void HTTP_notFound()
{
    LOGGER << "HTTP_notFound: " << server.uri() << NL;
    server.send(400, "text/html", "Not found!!!");
    delay(1);
}

void InitializeWebServices()
{
    // Initialize SPIFFS
    if (!SPIFFS.begin()) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi..");
    }

    // Print ESP32 Local IP Address
    Serial.println(WiFi.localIP());

    server.onNotFound(HTTP_notFound);

    // Start server
    server.begin();
}

#endif