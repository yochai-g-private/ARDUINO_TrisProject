#pragma once

#include "Settings.h"

namespace NYG 
{
    enum IoPins
    {
#if 0 // OLD PINOUT
        BUTTON_PIN          = D3,
        SCL_PIN                 = D1,
        SDA_PIN                 = D2,
        MANUAL_RELAY_PIN    = D0,
        MOTOR_RELAY_PIN     = D4,
        DIRECTION_RELAY_PIN = D5,
        BLUE_LEN_PIN        = D6,
        GREEN_LED_PIN       = D7,
        RED_LED_PIN         = D8,
#endif
        BUTTON_PIN          = A0,       //ADC0
        RELAYS_PIN          = D3,       //GPIO0
        SCL_PIN                 = D1,   //GPIO5
        SDA_PIN                 = D2,   //GPIO4     - HIGH
        MANUAL_RELAY_PIN    = D2,       //GPIO16    
        MOTOR_RELAY_PIN     = D4,       //GPIO2
        DIRECTION_RELAY_PIN = D5,       //GPIO14
        BLUE_LEN_PIN        = D6,       //GPIO12
        GREEN_LED_PIN       = D7,       //GPIO13
        RED_LED_PIN         = D8,       //GPIO15
    };
};


