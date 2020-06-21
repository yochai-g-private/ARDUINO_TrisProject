#include <dummy.h>
#include <Wire.h>
#include <DS3231.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include <Hash.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
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

static ResetAllBeforeSetup resetor;

//------------------------------------------------------
//	PREDECLARATIONS
//------------------------------------------------------

//------------------------------------------------------
//	SETUP
//------------------------------------------------------

void setup() {

    Logger::Initialize();
    
    TRACING = true;

    //delay(1000);

    bool RTC_ok = RTC::Begin();

    Signal(RTC_ok);
    Serial.println(RTC_ok ? "RTC is OK" : "RTC initialization failed");

    Settings::Load();

    delay(1000);
    TestLeds();

    //relays.On();

    TestRelays();

    InitializeWebServices();

    LOGGER << "Button " << (button_observer.Get() ? "pressed" : "released") << NL;

    LOGGER << "Ready!" << NL;
}
//------------------------------------------------------

//------------------------------------------------------
//	LOOP
//------------------------------------------------------
void loop() 
{
    bool pressed;

    if (button_observer.TestChanged(pressed))
    {
        LOGGER << "Button " << (pressed ? "pressed" : "released") << NL;
    }
}


extern const char*	gbl_build_date = __DATE__;
extern const char*	gbl_build_time = __TIME__;
