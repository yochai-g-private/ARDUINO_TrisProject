
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
#include "Html.h"
#include "MicroController.h"
#include "SmartHomeWiFiApp.h"
#include "OTA.h"
#include "WiFiUtils.h"

static AnalogInputPin                               analog_button(BUTTON_PIN);
static Threshold                                    threshold(analog_button, 500);
static StableDigitalInput<5000, 10, millis>         button(threshold);
static DigitalObserver                              button_observer(button);

#if 0
#define UNDER_DEVELOPMENT       true
#else
#define UNDER_DEVELOPMENT       false
#endif

DEFINE_SMART_HOME_WIFI_APP(Tris, WIFI_STA, UNDER_DEVELOPMENT);

static void get_internet_time(const char* hostname, bool RTC_ok, bool& wifi_OK);
static void dst_changed_restart(void*)
{
	MicroController::Restart();
}
//-----------------------------------------------------------
//struct TestAlignment
//{
//	char		a;
//	char		ar[4];
//	uint32_t	b;
//	uint16_t	c;
//	uint64_t	d;
//	struct
//	{
//		char		a;
//		char		ar[4];
//		uint32_t	b;
//		uint16_t	c;
//		uint64_t	d;
//	} e;
//};
//-----------------------------------------------------------
void setup()
{
    Logger::Initialize();
	//LOGGER << "sizeof TestAlignment = " << (uint32_t)sizeof(TestAlignment)	<< NL;
	//LOGGER << "offset of ar  = " << (uint32_t)offsetof(TestAlignment, ar) << NL;
	//LOGGER << "offset of b   = " << (uint32_t)offsetof(TestAlignment, b)	<< NL;
	//LOGGER << "offset of e   = " << (uint32_t)offsetof(TestAlignment, e)	<< NL;
	//LOGGER << "offset of ear = " << (uint32_t)offsetof(TestAlignment, e.ar)	<< NL;
	//LOGGER << "offset of e.b = " << (uint32_t)offsetof(TestAlignment, e.b) << NL;

    Settings::Load();

    Settings::WriteApplicationInfoToLog();

    Motor::Initialize();

	bool RTC_ok = RTC::Begin();

    if (!RTC_ok)
    {
        ErrorMgr::Report("RTC initialization failed");
    }

    RTC::SetOnError(ErrorMgr::Report);

	bool wifi_OK;
	const char* hostname = "TRIS";

	get_internet_time(hostname, RTC_ok, wifi_OK);

	if(wifi_OK)
		wifi_app.ConnectToWiFi(settings.WIFI.SSID(), 
							   settings.WIFI.PASS(), 
							   settings.WIFI.GATEWAY());
	else
		WiFiUtils::CreateAP(hostname,	// SSID
							NULL);		// PASS

	OTA::Initialize("TRIS");

	InitializeWebServices();

	LedMgr::Test();

    //Motor::TestRelays();

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
	if (OTA::OnLoop())
		return;

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
static void get_internet_time(const char* hostname, bool RTC_ok, bool& wifi_OK)
{
	wifi_OK = WiFiUtils::ConnectToAP(settings.WIFI.SSID(), settings.WIFI.PASS(), hostname);

	if (!wifi_OK)
	{
		return;
	}

	InternetTime::Data internet_time;

	bool	internet_time_Ok;
	FixTime now = InternetTime::Get(internet_time, internet_time_Ok);

	if (now.IsValid())
	{
		if (RTC_ok)
			RTC::Set(now);
		else
			FixTime::Set(now);
	}

	if (internet_time_Ok)
	{
		Settings temp = settings;
		temp.internet_time = internet_time;
		Settings::Store(temp);

		now = FixTime::Now();

		FixTime dst_change_time = (now < internet_time.dstStart) ? internet_time.dstStart :
								  (now < internet_time.dstEnd)	 ? internet_time.dstEnd :
																   now.GetMidNight() + SECONDS_PER_DAY;

		FixTime dst_change_restart_time = dst_change_time + ((uint32_t)12 * (uint32_t)SECONDS_PER_HOUR);

		Scheduler::Add(NULL, dst_changed_restart, NULL, "DST Restart", dst_change_restart_time);
	}
	else if (now.IsValid())
	{
		FixTime mnt = now.GetMidNight();
		mnt += (uint32_t)36 * (uint32_t)SECONDS_PER_HOUR;
		Scheduler::Add(NULL, dst_changed_restart, NULL, "DST Retry Restart", mnt);
	}

	WiFi.disconnect();
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

