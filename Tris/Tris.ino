
// Include from Arduino libraries
#include <EEPROM.h>
#include <TimeLib.h>
#include <Wire.h>
#include <DS3231.h>
#include <Ticker.h>

#include "Tris.h"
#include "LedMgr.h"

// Include from my libraries 
#include "TimeEx.h"
#include "WebServices.h"
#include "RTC.h"

void setup() {
    // Serial port for debugging purposes
    Logger::Initialize();

    bool RTC_ok = RTC::Begin();

//  Signal(RTC_ok);
    Serial.println(RTC_ok ? "RTC is OK" : "RTC initialization failed");

    Settings::Load();

    InitializeWebServices();
}

void loop()
{
    Scheduler::Proceed();
}

extern const char*	gbl_build_date = __DATE__;
extern const char*	gbl_build_time = __TIME__;

#include <Hash.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

#include "WebServices.cxx"
