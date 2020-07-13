#pragma once

#include "NYG.h"

class AsyncWebServer;

#include "Settings.h"
#include "ErrorMgr.h"

enum IoPins
{
    // https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
    BUTTON_PIN          = A0,       //ADC0
    SCL_PIN                 = D1,   //GPIO5
    SDA_PIN                 = D2,   //GPIO4  
    MANUAL_RELAY_PIN    = D3,       //GPIO0   
    MOTOR_RELAY_PIN     = D6,       //GPIO2      - HIGH
    DIRECTION_RELAY_PIN = D5,       //GPIO14
    BLUE_LEN_PIN        = D4,       //GPIO12
    GREEN_LED_PIN       = D7,       //GPIO13
    RED_LED_PIN         = D8,       //GPIO15
};

enum State
{
    Manual,
    HalfManual,
    Ready,
    BusyUp,
    BusyDown,
    Error,
};

struct StateMgr
{
    void operator = (State state);
    operator State ()   const { return m_state; }

    static StateMgr instance;

    #define gbl_State   StateMgr::instance

protected:

    StateMgr() : m_state((State)-1)    {   }

private:    

    State m_state;

};

void OnSettingsChanged();
const char* GetYesNo(const bool& b);

#define GET_NUMERIC_PARAM(type, fld, min, max, val, parent_fld)    if (Get##type##Param(*request, #fld, min, max, true, val))   { temp.parent_fld.fld = val; } else;

#define GET_UNSIGNED_BYTE_PARAM(fld, min, max, val, parent_fld)     GET_NUMERIC_PARAM(UnsignedByte, fld, min, max, val, parent_fld)
#define GET_DOUBLE_PARAM(fld, min, max, val, parent_fld)            GET_NUMERIC_PARAM(Double, fld, min, max, val, parent_fld)
#define GET_BOOL_PARAM(fld, val, parent_fld)                        if (GetBoolParam(*request, #fld, val))      temp.parent_fld.fld = val; else;

bool SetActionDisabled(const String& var, String& action_disabled_reason);
#define CheckActionDisabled(var)    \
    String action_disabled_reason;\
    if (SetActionDisabled(var, action_disabled_reason))\
        return action_disabled_reason;;
