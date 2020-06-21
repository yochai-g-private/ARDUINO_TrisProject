/*
 Name:		NanoTest.ino
 Created:	5/24/2020 10:50:01 PM
 Author:	YLA
*/

#include <NYG.h>

#include "TimeEx.h"
#include "RTC.h"
#include "Sun.h"
#include "StdConfig.h"
#include "StdMenu.h"
#include "WireTest.h"

using namespace NYG;

Menu::Root MainMenu("Main Menu"); 
STDMENU_DaT(MainMenu)

StdConfig						cfg("NanoTest", 0);
StdConfig::SerialMenuContext	ctx(MainMenu, cfg);


// the setup function runs once when you press reset or power the board
void setup() 
{
    SerialMenu::Begin();

#define WIRE_TEST   1
#if WIRE_TEST
    Wire.begin();
#else
    RTC::Begin();

    TRACING = true;
    cfg.Load();
    cfg.Show();
#endif 

    LOGGER << "Ready!" << NL;
}

// the loop function runs over and over again until power down or reset
void loop() 
{
#if WIRE_TEST
    LOGGER << "Searching for available devices..." << NL;
    uint8_t found = WireTest::SearchForAvailableDevices();
    LOGGER << found << " devices found" << NL;
    LOGGER << "------------------------------------------------" << NL;

    delay(5000);
#else
    SerialMenu::Proceed(ctx);
#endif
}

extern const char*	gbl_build_date = __DATE__;
extern const char*	gbl_build_time = __TIME__;

