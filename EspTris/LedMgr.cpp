#include <NYG.h>
#include "Settings.h"
#include "RGB_Led.h"
#include "LedMgr.h"
#include "EspTris.h"
#include "Logger.h"

using namespace NYG;

static RGB_Led led(RED_LED_PIN, GREEN_LED_PIN, BLUE_LEN_PIN);

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
void TestLeds()
{
    LOGGER << "Testing leds: ";
    test("RED",     led.GetRed());
    test("GREEN",   led.GetGreen());
    test("BLUE",    led.GetBlue());
    LOGGER << "Done!" << NL;
}
//------------------------------------------------------
static void signal(IDigitalOutput& out)
{
    for (int cnt = 0; cnt < 2; cnt++)
    {
        out.On();    delay(50);
        out.Off();   delay(50);
    }
}
//------------------------------------------------------
void SignalOK()
{
    signal(led.GetGreen());
}
//------------------------------------------------------
void SignalError()
{
    signal(led.GetRed());
}
//------------------------------------------------------
void Signal(bool ok)
{
    signal(ok ? led.GetGreen() : led.GetRed());
}
