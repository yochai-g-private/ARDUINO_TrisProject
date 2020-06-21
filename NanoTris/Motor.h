#pragma once

#include "Configuration.h"

enum TrisState
{
    TS_Btm = 0,
    TS_Air,
    TS_Sun,
    TS_Top,  __max_TS__ = TS_Top
};

void StartMotor(TrisState state);
void TestMotor();
void StopMotor(bool leave_it_in_unknown_state = false);
