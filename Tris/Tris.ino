#include <EEPROM.h>
#include <TimeLib.h>


#include "NYG.h"
#include "TimeEx.h"

#include "WebServices.h"
#include "Settings.h"

void setup() {
    // Serial port for debugging purposes
    Logger::Initialize();
    Settings::Load();
    InitializeWebServices();
}

void loop() {

}

#include <Hash.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

#include "WebServices.cxx"
