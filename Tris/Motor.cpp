#include "Tris.h"
#include "Motor.h"
#include "Timer.h"
#include "IOutput.h"

#define TS_UNKNOW     ((TrisState)-1)

static TrisState st_tris_state = TS_UNKNOW;

static DigitalOutputPin*    pPowerSwitchRelay,
                       *    pMotorSwitchRelay,
                       *    pMotorDirectionRelay;

static void initialize()
{
    static DigitalOutputPin     sPowerSwitchRelay(MANUAL_RELAY_PIN),
                                sMotorSwitchRelay(MOTOR_RELAY_PIN),
                                sMotorDirectionRelay(DIRECTION_RELAY_PIN);

    pPowerSwitchRelay       = &sPowerSwitchRelay;
    pMotorSwitchRelay       = &sMotorSwitchRelay;
    pMotorDirectionRelay    = &sMotorDirectionRelay;

    #define PowerSwitchRelay        (*pPowerSwitchRelay)
    #define MotorSwitchRelay        (*pMotorSwitchRelay)
    #define MotorDirectionRelay     (*pMotorDirectionRelay)
}

static Timer                MotorStopTimer;

static void test(const char* name, IDigitalOutput& out)
{
    LOGGER << name << "... ";

    for (int cnt = 0; cnt < 5; cnt++)
    {
        out.On();    delay(500);
        out.Off();   delay(500);
    }
}
//------------------------------------------------------
void TestRelays()
{
    initialize();
    LOGGER << "Testing relays: ";
    test("POWER",   PowerSwitchRelay);
    test("MOTOR",   MotorSwitchRelay);
    test("UP_DN",   MotorDirectionRelay);
    LOGGER << "Done!" << NL;
}
//------------------------------------------------------
void StartMotor(TrisState state)
{
#if 0
    if (TS_UNKNOW == st_tris_state)
    {
        st_tris_state = TS_Btm;
        StartMotor(TS_Top);
    }

    if (st_tris_state == state)
        return;

    bool move_up = st_tris_state < state;

    const Configuration::Timing::Direction& d = (move_up) ? cfg.timings.up : cfg.timings.down;
    double start, end;

    if (move_up)
    {
        const Configuration::Timing::Direction& d = cfg.timings.up;

        switch (st_tris_state)
        {
            case TS_Btm:  start = 0;        break;
            case TS_Air:  start = d.air;    break;
            case TS_Sun:  start = d.sun;    break;
            case TS_Top:  start = d.all;    break;
        }

        switch (state)
        {
            case TS_Btm:  end = 0;          break;
            case TS_Air:  end = d.air;      break;
            case TS_Sun:  end = d.sun;      break;
            case TS_Top:  end = d.all;      break;
        }
    }
    else
    {
        const Configuration::Timing::Direction& d = cfg.timings.down;

        switch (st_tris_state)
        {
            case TS_Btm:  start = d.all;    break;
            case TS_Air:  start = d.air;    break;
            case TS_Sun:  start = d.sun;    break;
            case TS_Top:  start = 0;        break;
        }

        switch (state)
        {
            case TS_Btm:  end = d.all;      break;
            case TS_Air:  end = d.air;      break;
            case TS_Sun:  end = d.sun;      break;
            case TS_Top:  end = 0;          break;
        }
    }

    Led.SetOff();
    LedToggler.StartOnOff(move_up ? Led.GetGreen() : Led.GetBlue(), 500);
    double seconds = end - start;

    MotorDirectionRelay.Set(move_up);
    delay(200);
    MotorSwitchRelay.On();

    MotorStopTimer.StartOnce((long)seconds * 1000);

    st_tris_state = state;
#endif
}
//---------------------------------------------------------------
void TestMotor()
{
    if (MotorStopTimer.Test())
        StopMotor();
}
//---------------------------------------------------------------
void StopMotor(bool leave_it_in_unknown_state)
{
    if (leave_it_in_unknown_state)
        st_tris_state = TS_UNKNOW;

    if (!MotorDirectionRelay.Get())
        return;

 //?????  Led.SetOff();
   MotorSwitchRelay.Off();
   delay(200);
   MotorDirectionRelay.Off();
}
//---------------------------------------------------------------
