#include <NYG.h>

#include "TimeEx.h"
#pragma once

#include "RTC.h"
#include "Sun.h"
#include "StdConfig.h"
#include "StdMenu.h"
#include "Toggler.h"
#include "RGB_Led.h"

using namespace NYG;

enum
{
    ACTIVE_SWITCH_RELAY     = D2,
    MOTOR_SWITCH_RELAY      = D3,
    MOTOR_DIRECTION_RELAY   = D4,

    DIP_SWITCH_ACTIVE       = D5,
    DIP_SWITCH_DST          = D6,
    DIP_SWITCH_SUN_PROTECT  = D7,
    DIP_SWITCH_NIGHT_CLOSE  = D8,

    LED_RED                 = D9,
    LED_GREEN               = D10,
    LED_BLUE                = D11,

};

extern bool         IsActive;
extern Toggler      LedToggler;
extern RGB_Led      Led;
