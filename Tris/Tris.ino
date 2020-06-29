
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

 //   Motor::TestRelays();

    InitializeWebServices();

    Motor::Schedule();

    LOGGER << "Ready!" << NL;
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
#if 0
static FixTime get_time_from_minutes(const DstTime& now, uint16_t minutes)
{
    Times times = now;
    times.m = minutes % MINUTES_PER_HOUR;
    times.h = (minutes - times.m) / MINUTES_PER_HOUR;
    return FixTime(DstTime(times));
}
//-----------------------------------------------------------
static Scheduler::Handler   scheduled[4];

#define nightly_down        scheduled[0]
#define nightly_up          scheduled[1]
#define sun_protect_down    scheduled[2]
#define sun_protect_up      scheduled[3]
//-----------------------------------------------------------
static void schedule_next_event(void* unused = NULL);

static void set_motor_state(void* ctx)
{
    Motor::SetPower(true);

    TrisState state = (TrisState)(long)ctx;
    Motor::SetState(state);
    Scheduler::AddInSeconds(schedule_next_event, NULL, "Schedule next event", 1);
}
//-----------------------------------------------------------
static void schedule_motor(Scheduler::Handler& handler, TrisState state, const char* description, const FixTime& time)
{
    if (!Scheduler::IsScheduled(handler))
        handler = Scheduler::Add(set_motor_state, (void*)(long)state, description, time);
}
//-----------------------------------------------------------
static void schedule_next_event(void*)
{
    if (gbl_State == Error)
        return;

    if (settings.states.manual)
        return;

    delay(1500);

    DstTime dst = DstTime::Now();
    FixTime now = FixTime(dst);

    FixTime nightly_down_time,
            nightly_up_time,
            sun_protect_down_time,
            sun_protect_up_time;

    // Check NIGHTLY period
    if (settings.states.nightly.mode != NM_DISABLED)
    {
        nightly_up_time   = (Settings::SUNRISE == settings.states.nightly.up) ? Sun::GetTodayLocalRiseTime() : get_time_from_minutes(dst, settings.states.nightly.up);
        nightly_down_time = get_time_from_minutes(dst, settings.states.nightly.down);

        if (nightly_up_time < nightly_down_time)
            nightly_down_time -= SECONDS_PER_DAY;

        if (nightly_down_time <= now && nightly_up_time >= now)
        {
            if (settings.states.nightly.mode == NM_AIR)
                schedule_motor(nightly_down, TS_Air, "Motor AIR", now);
            else
                schedule_motor(nightly_down, TS_Btm, "Motor BOTTOM", now);

            schedule_motor(nightly_up, TS_Top, "Motor TOP", nightly_up_time);
        }
        else if (now < nightly_down_time)
        {
            if(settings.states.nightly.mode == NM_AIR)
                schedule_motor(nightly_down, TS_Air, "Motor AIR", nightly_down_time);
            else
                schedule_motor(nightly_down, TS_Btm, "Motor BOTTOM", nightly_down_time);

            schedule_motor(nightly_up, TS_Top, "Motor TOP", now);
        }
    }

    // Check SUN PROTECT
    if (settings.states.sun_protect.on)
    {
        sun_protect_down_time = Sun::GetTodayLocalRiseTime() + (SECONDS_PER_MINUTE * (int)settings.states.sun_protect.minutes_after_sun_rise);
        sun_protect_up_time   = sun_protect_down_time + +(SECONDS_PER_MINUTE * (int)settings.states.sun_protect.duration_minutes);

        if (sun_protect_down_time <= now && sun_protect_up_time >= now)
        {
            schedule_motor(sun_protect_down, TS_Sun, "Motor SUN", now);
            schedule_motor(sun_protect_up,   TS_Top, "Motor TOP", sun_protect_up_time);
        }
        else if (now < sun_protect_down_time)
        {
            schedule_motor(sun_protect_up, TS_Sun, "Motor SUN", sun_protect_up_time);
            // do not return;
        }
    }

    Motor::SetState(TS_Top);
}
//-----------------------------------------------------------
static void initialize()
{
    if (gbl_State == Error)
        return;

    if (settings.states.manual)
    {
        gbl_State = Manual;
        Motor::SetPower(false);

        for (int idx = 0; idx < countof(scheduled); idx++)
            Scheduler::Cancel(scheduled[idx], "Powered OFF");

        return;
    }

    gbl_State = Ready;
    Motor::SetPower(true);

    schedule_next_event();
}
//-----------------------------------------------------------
#endif
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
