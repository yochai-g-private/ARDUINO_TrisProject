#include "Tris.h"
#include "RGB_Led.h"
#include "LedMgr.h"
#include "Logger.h"
#include "Toggler.h"

using namespace NYG;

static RGB_Led led(RED_LED_PIN, GREEN_LED_PIN, BLUE_LEN_PIN);
static Toggler toggler;

static void test(const char* color, IDigitalOutput& out)
{
    LOGGER << color << "... ";

    for (int cnt = 0; cnt < 3; cnt++)
    {
        out.On();    delay(50);
        out.Off();   delay(150);
    }
}
//------------------------------------------------------
void LedMgr::Test()
{
    LOGGER << "Testing leds: ";
    test("RED",     led.GetRed());
    test("GREEN",   led.GetGreen());
    test("BLUE",    led.GetBlue());
    LOGGER << "Done!" << NL;
}
//------------------------------------------------------
void LedMgr::OnStateChanged(State prev)
{
    State curr = gbl_State;

    if (curr == prev)
        return;

    toggler.Stop();

    switch (curr)
    {
        case Manual:        toggler.Start(     led.GetBlue(),       Toggler::OnTotal(500, 60000));      break;
        case HalfManual:    toggler.Start(     led.GetBlue(),       Toggler::OnTotal(200, 15000));      break;
        case Ready:         toggler.Start(     led.GetGreen(),      Toggler::OnTotal(500, 60000));      break;
        case BusyUp:        toggler.StartOnOff(led.GetGreen(),      250);                               break;
        case BusyDown:      toggler.StartOnOff(led.GetRed(),        250);                               break;
        case Error:         toggler.StartOnOff(led.GetRed(),       1000);                               break;
    }
}
//------------------------------------------------------
void LedMgr::OnLoop()
{
    toggler.Toggle();
}
//------------------------------------------------------
