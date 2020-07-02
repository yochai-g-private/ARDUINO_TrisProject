
// Include from Arduino libraries
#include <EEPROM.h>
#include <TimeLib.h>
#include <Wire.h>
#include <DS3231.h>
#include <Ticker.h>

#include "Tris.h"
#include "LedMgr.h"
#include "Motor.h"

// Include from my libraries 
#include "TimeEx.h"
#include "WebServices.h"
#include "RTC.h"
#include "Scheduler.h"

//-----------------------------------------------------------
void setup()
{
    Logger::Initialize();
    Motor::Initialize();

    bool RTC_ok = RTC::Begin();

    if (!RTC_ok)
    {
        ErrorMgr::Report("RTC initialization failed");
    }

    RTC::SetOnError(ErrorMgr::Report);

    Settings::Load();

    LedMgr::Test();

    Motor::TestRelays();

    InitializeWebServices();

    if (Error != gbl_State)
    {
        gbl_State = settings.states.manual ? Manual : Ready;
        LOGGER << "State set to " << ((Manual == gbl_State) ? "MANUAL" : "READY") << NL;
    }

    Motor::Schedule();

    LOGGER << "Started!" << NL;
    //TRACING = true;
}
//-----------------------------------------------------------
void loop()
{
    Scheduler::Proceed();
    LedMgr::OnLoop();
    Motor::OnLoop();
}
//-----------------------------------------------------------
void StateMgr::operator = (State state)
{
    if (m_state == Error)
        return;

    State prev = m_state;
    m_state    = state;

    LedMgr::OnStateChanged(prev);
}
//-----------------------------------------------------------
void OnSettingsChanged()
{
    Motor::Schedule();
}
//-----------------------------------------------------------
extern const char*	gbl_build_date = __DATE__;
extern const char*	gbl_build_time = __TIME__;
StateMgr            StateMgr::instance;

#include <Hash.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

#include "WebServices.cxx"
