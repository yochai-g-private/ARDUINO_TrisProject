#include "Tris.h"
#include "Motor.h"
#include "Timer.h"
#include "IOutput.h"
#include "Scheduler.h"
#include "Sun.h"

enum TrisPosition
{
    TP_Btm = 1,
    TP_Air,
    TP_Sun,
    TP_Top, __max_TP__ = TP_Top
};

#define TP_UNKNOWN     ((TrisPosition)0)

static TrisPosition st_tris_position  = TP_UNKNOWN;
static TrisPosition required_position = TP_UNKNOWN;

static DigitalOutputPin*    pPowerSwitchRelay,
                       *    pMotorSwitchRelay,
                       *    pMotorDirectionRelay;

static void set_power(bool on);
static void set_current_position(TrisPosition p);

void Motor::Initialize()
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
static Scheduler::Handler   next_position_handler;

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
void Motor::TestRelays()
{
    LOGGER << "Testing relays: ";
    test("POWER",   PowerSwitchRelay);
    test("MOTOR",   MotorSwitchRelay);
    test("UP_DN",   MotorDirectionRelay);
    LOGGER << "Done!" << NL;
}
//------------------------------------------------------
static void start(bool down)
{
    set_power(true);

    LOGGER << "Starting " << (down ? "DOWN" : "UP") << " motor" << NL;

    if (down)
    {
        MotorDirectionRelay.On();
        delay(200);
    }

    MotorSwitchRelay.On();

    gbl_State = down ? BusyDown : BusyUp;
}
//------------------------------------------------------
static void stop()
{
    if (!MotorDirectionRelay.Get())
        return;

    LOGGER << "Stopping motor" << NL;

    MotorStopTimer.Stop();

    MotorSwitchRelay.Off();
    delay(200);
    MotorDirectionRelay.Off();
    delay(300);

    gbl_State = Ready;
}
//------------------------------------------------------
static void set_power(bool on)
{
    if (pPowerSwitchRelay->Get() == on)
        return;

    LOGGER << "Setting power " << (on ? "ON" : "OFF") << NL;

    Scheduler::Cancel(next_position_handler, "OnPower");

    stop();
    pPowerSwitchRelay->Set(on);
    st_tris_position = required_position = TP_UNKNOWN;

    gbl_State = on ? Ready : Manual;
}
//------------------------------------------------------
void Motor::OnLoop()
{
    if (gbl_State == Error)
        return;

    if (MotorStopTimer.Test())
    {
        stop();

        if (required_position != TP_UNKNOWN)
        {
            set_current_position(required_position);
        }
    }
}
//------------------------------------------------------
struct Event
{
    FixTime         t;
    TrisPosition    p;
    const char*     d;
};
//------------------------------------------------------
static const char* GetPositionText(TrisPosition p)
{
    switch (p)
    {
        #define TREATE_CASE(id) case TP_##id : return #id
        TREATE_CASE(UNKNOWN);
        TREATE_CASE(Top);
        TREATE_CASE(Sun);
        TREATE_CASE(Air);
        TREATE_CASE(Btm);
        #undef  TREATE_CASE
    }

    return "?";
}
//------------------------------------------------------
static void set_current_position(TrisPosition p)
{
    if (st_tris_position == p)
        return;

    stop();

    bool   down;
    double seconds;

    LOGGER << "Current position is " << GetPositionText(st_tris_position) << NL;

    TrisPosition de_facto_p = p;

    switch (st_tris_position)
    {
        case TP_UNKNOWN : 
        {
            switch (p)
            {
                case TP_Btm:
                case TP_Air:
                case TP_Sun:
                {
                    down = true;
                    seconds = settings.timings.down.all;
                    de_facto_p = TP_Btm;
                    break;
                }

                case TP_Top:
                {
                    down = false;
                    seconds = settings.timings.up.all;
                    de_facto_p = TP_Top;
                    break;
                }
            }

            break;
        }

        case TP_Btm:
        {
            down = false;

            switch (p)
            {
                case TP_Air:
                {
                    seconds = settings.timings.up.air;
                    break;
                }

                case TP_Sun:
                {
                    seconds = settings.timings.up.sun;
                    break;
                }

                case TP_Top:
                {
                    seconds = settings.timings.up.all;
                    break;
                }
            }

            break;
        }

        case TP_Air:
        {
            down = false;

            switch (p)
            {
                case TP_Btm:
                {
                    down = true;
                    seconds = settings.timings.down.all - settings.timings.down.air;
                    break;
                }

                case TP_Sun:
                {
                    seconds = settings.timings.up.sun - settings.timings.up.air;
                    break;
                }

                case TP_Top:
                {
                    seconds = settings.timings.up.all - settings.timings.up.air;
                    break;
                }
            }

            break;
        }

        case TP_Sun:
        {
            down = true;

            switch (p)
            {
                case TP_Btm:
                {
                    seconds = settings.timings.down.all - settings.timings.down.sun;
                    break;
                }

                case TP_Air:
                {
                    seconds = settings.timings.down.all - settings.timings.down.air;
                    break;
                }

                case TP_Top:
                {
                    down = false;
                    seconds = settings.timings.up.all - settings.timings.up.sun;
                    break;
                }
            }

            break;
        }

        case TP_Top:
        {
            down = true;

            switch (p)
            {
                case TP_Btm:
                {
                    seconds = settings.timings.down.all;
                    break;
                }

                case TP_Air:
                {
                    seconds = settings.timings.down.air;
                    break;
                }

                case TP_Sun:
                {
                    seconds = settings.timings.down.sun;
                    break;
                }
            }

            break;
        }
    }

    LOGGER << "Setting position to " << GetPositionText(de_facto_p) << NL;

    start(down);

    LOGGER << "Motor will stop in " << seconds << " seconds" << NL;

    MotorStopTimer.StartOnce(seconds * 1000);

    required_position = (TP_UNKNOWN == st_tris_position) ? p : TP_UNKNOWN;
    st_tris_position  = p;
}
//------------------------------------------------------
static void set_motor_state(void* ctx)
{
    TrisPosition* p = (TrisPosition*)ctx;
    set_current_position(*p);
    Motor::Schedule();
}
//------------------------------------------------------
static FixTime get_time_from_minutes(const DstTime& now, uint16_t minutes)
{
    Times times = now;
    times.m = minutes % MINUTES_PER_HOUR;
    times.h = (minutes - times.m) / MINUTES_PER_HOUR;
    return FixTime(DstTime(times));
}
//------------------------------------------------------
void Motor::Schedule()
{
    if (gbl_State == Error)
        return;

    if (settings.states.manual)
        return;

    Event events[8];

    memset(events, 0, sizeof(events));

    DstTime dst = DstTime::Now();
    FixTime now = FixTime(dst);

    if (settings.states.nightly.mode != NM_DISABLED)
    {
        FixTime down_time = get_time_from_minutes(dst, settings.states.nightly.down);

        if (down_time < now)
        {
            // passee
            events[0].t = down_time;
            events[4].t = down_time + SECONDS_PER_DAY;
        }
        else
        {
            // in future
            events[0].t = down_time - SECONDS_PER_DAY;
            events[4].t = down_time;
        }

        events[0].p =
        events[4].p = (settings.states.nightly.mode == NM_AIR) ? TP_Air : TP_Btm;

        events[0].d =
        events[4].d = (settings.states.nightly.mode == NM_AIR) ? "Nightly AIR" : "Nightly BOTTOM";

        FixTime up_time = (Settings::SUNRISE == settings.states.nightly.up) ? Sun::GetTodayLocalRiseTime() : get_time_from_minutes(dst, settings.states.nightly.up);

        if (up_time < now)
        {
            // passee
            events[1].t = up_time;
            events[5].t = up_time + SECONDS_PER_DAY;
        }
        else
        {
            // in future
            events[1].t = up_time - SECONDS_PER_DAY;
            events[5].t = up_time;
        }

        events[1].p =
        events[5].p = TP_Top;

        events[1].d =
        events[5].d = "Nightly TOP";
    }

    if (settings.states.sun_protect.on)
    {
        FixTime down_time = Sun::GetTodayLocalRiseTime() + (SECONDS_PER_MINUTE * (int)settings.states.sun_protect.minutes_after_sun_rise);

        if (down_time < now)
        {
            // passee
            events[2].t = down_time;
            events[6].t = down_time + SECONDS_PER_DAY;
        }
        else
        {
            // in future
            events[2].t = down_time - SECONDS_PER_DAY;
            events[6].t = down_time;
        }

        events[2].p =
        events[6].p = TP_Sun;

        events[2].d =
        events[6].d = "Sun PROTECTED";

        FixTime up_time = down_time + +(SECONDS_PER_MINUTE * (int)settings.states.sun_protect.duration_minutes);

        if (up_time < now)
        {
            // passee
            events[3].t = up_time;
            events[7].t = up_time + SECONDS_PER_DAY;
        }
        else
        {
            // in future
            events[3].t = up_time - SECONDS_PER_DAY;
            events[7].t = up_time;
        }

        events[3].p =
        events[7].p = TP_Top;

        events[3].d =
        events[7].d = "Sun UNPROTECTED";

    }

    int idx, max_idx;

    for (idx = 0, max_idx = countof(events); idx < max_idx; idx++)
    {
        //  
        if ((idx + 1 < max_idx)                     &&      // not the last one
            (events[idx].p == TP_UNKNOWN        ||          // not set
             (events[idx+1].p != TP_UNKNOWN &&              // next set
              events[idx].t > events[idx+1].t)))            // next is earlier
        {
            memcpy(events + idx, 
                   events + idx + 1, 
                   sizeof(*events) * (max_idx - (idx + 1)));

            idx--;
            max_idx--;
        }
    }

    Event* curr_event = NULL,
         * next_event = NULL;

    for (idx = 0; idx < max_idx; idx++)
    {
        if (events[idx].t < now)
        {
            curr_event = events + idx;
        }
        else
        {
            next_event = events + idx;
            break;
        }
    }

    if (curr_event)
    {
        set_current_position(curr_event->p);
        static Event next = *next_event;
        next_position_handler = Scheduler::Add(set_motor_state, &next.p, next.d, next.t);

        return;
    }

    set_current_position(TP_Top);
}
//------------------------------------------------------
void Motor::PowerOff()
{
    set_power(false);
}
//------------------------------------------------------
#if 0
void Motor::SetState(TrisState state)
{
    if (TP_UNKNOW == st_tris_state)
    {
        st_tris_state = TP_Btm;
        StartMotor(TP_Top);
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
            case TP_Btm:  start = 0;        break;
            case TP_Air:  start = d.air;    break;
            case TP_Sun:  start = d.sun;    break;
            case TP_Top:  start = d.all;    break;
        }

        switch (state)
        {
            case TP_Btm:  end = 0;          break;
            case TP_Air:  end = d.air;      break;
            case TP_Sun:  end = d.sun;      break;
            case TP_Top:  end = d.all;      break;
        }
    }
    else
    {
        const Configuration::Timing::Direction& d = cfg.timings.down;

        switch (st_tris_state)
        {
            case TP_Btm:  start = d.all;    break;
            case TP_Air:  start = d.air;    break;
            case TP_Sun:  start = d.sun;    break;
            case TP_Top:  start = 0;        break;
        }

        switch (state)
        {
            case TP_Btm:  end = d.all;      break;
            case TP_Air:  end = d.air;      break;
            case TP_Sun:  end = d.sun;      break;
            case TP_Top:  end = 0;          break;
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
}
#endif
//---------------------------------------------------------------
void Motor::AddWebServices(AsyncWebServer& server)
{

}
//---------------------------------------------------------------
