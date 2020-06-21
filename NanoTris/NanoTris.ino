/*
 Name:		NanoTest.ino
 Created:	5/24/2020 10:50:01 PM
 Author:	YLA
*/

#include "Tris.h"
#include "Motor.h"
#include "IInput.h"
#include "IOutput.h"
#include "Observer.h"
#include "StableInput.h"

Menu::Root MainMenu("Main Menu");
STDMENU_DaT(MainMenu)

Configuration                   cfg;
StdConfig::SerialMenuContext	ctx(MainMenu, cfg);

static DigitalOutputPin         ActiveSwitchRelay(ACTIVE_SWITCH_RELAY);

static DigitalPullupInputPin    DipSwitch_Active    (DIP_SWITCH_ACTIVE, false),
                                DipSwitch_Dst       (DIP_SWITCH_DST, true),
                                DipSwitch_SunProtect(DIP_SWITCH_SUN_PROTECT, true),
                                DipSwitch_NightClose(DIP_SWITCH_NIGHT_CLOSE, true);

typedef StableDigitalInput<100, 100, millis>	BouncingDipSwitch;
#define BOUNCING_DS(id)         BDS_##id(DipSwitch_##id)
static BouncingDipSwitch        BOUNCING_DS(Active),
                                BOUNCING_DS(Dst),
                                BOUNCING_DS(SunProtect),
                                BOUNCING_DS(NightClose);

#define DS_OBSERVER(id)         ODC_##id(BDS_##id)

static DigitalObserver          DS_OBSERVER(Active),
                                DS_OBSERVER(Dst),
                                DS_OBSERVER(SunProtect),
                                DS_OBSERVER(NightClose);

bool                            IsActive; 
Toggler                         LedToggler;
RGB_Led                         Led(LED_RED, LED_GREEN, LED_BLUE);

DigitalOutputPin			    BuiltinLed(LED_BUILTIN);
Toggler						    BuiltinLedToggler;

// the setup function runs once when you press reset or power the board
void setup()
{
    delay(1000);

    SerialMenu::Begin();
    ODC_Dst.Get();
    RTC::Begin();

    cfg.Load();
    cfg.Show();

    BuiltinLedToggler.StartOnOff(BuiltinLed, 1000);

    LOGGER << "Ready!" << NL;
}

// the loop function runs over and over again until power down or reset
void loop()
{
    BuiltinLedToggler.Toggle();
    ODC_Dst.Get();

    if (ODC_Active.TestChanged(IsActive))
    {
        Led.SetOff();
        LedToggler.Stop();

        if (IsActive)
        {
            ActiveSwitchRelay.On();
        }
        else
        {
            StopMotor();
            LedToggler.Start(Led.GetRed(), Toggler::OnTotal(100, 10000));
            ActiveSwitchRelay.Off();
        }
    }

    LedToggler.Toggle();

    if (!IsActive)
        return;

    TestMotor();

    //SerialMenu::Proceed(ctx);
}

extern const char*	gbl_build_date = __DATE__;
extern const char*	gbl_build_time = __TIME__;

namespace NYG {
    bool GetDst()
    {
        return ODC_Dst.GetValue();
    }
}