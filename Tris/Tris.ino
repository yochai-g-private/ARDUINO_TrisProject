
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
#include "Location.h"
#include "Scheduler.h"
#include "IInput.h"
#include "Threshold.h"
#include "StableInput.h"
#include "Observer.h"

static AnalogInputPin                               analog_button(BUTTON_PIN);
static Threshold                                    threshold(analog_button, 500);
static StableDigitalInput<5000, 10, millis>         button(threshold);
static DigitalObserver                              button_observer(button);

//-----------------------------------------------------------
void setup()
{
    Logger::Initialize();

    Settings::WriteApplicationInfoToLog();

    Motor::Initialize();

    bool RTC_ok = RTC::Begin();

    if (!RTC_ok)
    {
        ErrorMgr::Report("RTC initialization failed");
    }

    RTC::SetOnError(ErrorMgr::Report);

    Settings::Load();

    LedMgr::Test();

    //Motor::TestRelays();

    InitializeWebServices();

    if (Error != gbl_State)
    {
        gbl_State = settings.states.manual ? Manual : Ready;
        LOGGER << "State set to " << ((Manual == gbl_State) ? "MANUAL" : "READY") << NL;
    }

#define DEBUG_SCHEDULER     0

#if DEBUG_SCHEDULER
    Times test;
    //test.ParseDateAndTime("2020/07/05 05:38:00");
    test.ParseDateAndTime("2020/07/05 23:59:00");
    DstTime dst = test;
    RTC::Set(FixTime(dst));
#endif
    //TRACING = true;
    Motor::Schedule();

    // Consume the first button released event
    button_observer.Get();

    LOGGER << "Started!" << NL;
}
//-----------------------------------------------------------
void loop()
{
    Scheduler::Proceed();
    LedMgr::OnLoop();
    Motor::OnLoop();

    static unsigned long check_button;
    unsigned long decisec = millis() / 100;

    if (check_button != decisec)
    {
        check_button = decisec;

        bool pressed;
        if (button_observer.TestChanged(pressed) && !pressed)
            Motor::TogglePower();
    }
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


