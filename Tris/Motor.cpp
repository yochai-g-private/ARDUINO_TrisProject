#include "Tris.h"
#include "Motor.h"
#include "Timer.h"
#include "IOutput.h"
#include "Scheduler.h"
#include "Sun.h"
#include "WebServices.h"

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
static void set_current_position(TrisPosition p, bool manual = false);

static int              rolling_seconds;
static unsigned long    rolling_started;

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

static Timer                SetPowerOnTimer;
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
    if (settings.states.manual)
    {
        LOGGER << "Motor starting " << (down ? "DOWN" : "UP") << " canceled due to manual state" << NL;
        return;
    }

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
    if (!MotorSwitchRelay.Get())
    {
        return;
    }

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

    if (!on)
    {
        stop();
        st_tris_position = required_position = TP_UNKNOWN;
    }

    if (settings.states.manual)
        on = false;

    LOGGER << "Setting power " << (on ? "ON" : "OFF") << NL;

    pPowerSwitchRelay->Set(on);

    gbl_State = on ? Ready : Manual;
}
//------------------------------------------------------
void Motor::OnLoop()
{
    if (gbl_State == Error)
        return;

    if (SetPowerOnTimer.Test())
    {
        set_power(true);
    }

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
static void set_current_position(TrisPosition p, bool manual)
{
    if (st_tris_position == p)
        return;

    stop();

    bool   down;
    double seconds;

    LOGGER << "Current position is " << GetPositionText(st_tris_position) << NL;
    LOGGER << "Required position is " << GetPositionText(p) << NL;

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

            #define ADDITIONLA_ROLLING_SECONDS  3

            switch (p)
            {
                case TP_Btm:
                {
                    down = true;
                    seconds = ADDITIONLA_ROLLING_SECONDS + settings.timings.down.all - settings.timings.down.air;
                    break;
                }

                case TP_Sun:
                {
                    seconds = settings.timings.up.sun - settings.timings.up.air;
                    break;
                }

                case TP_Top:
                {
                    seconds = ADDITIONLA_ROLLING_SECONDS + settings.timings.up.all - settings.timings.up.air;
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
                    seconds = ADDITIONLA_ROLLING_SECONDS + settings.timings.down.all - settings.timings.down.sun;
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
                    seconds = ADDITIONLA_ROLLING_SECONDS + settings.timings.up.all - settings.timings.up.sun;
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

    rolling_seconds = (int)seconds;
    rolling_started = millis();

    MotorStopTimer.StartOnce(seconds * 1000);

    LOGGER << "Motor will stop in " << seconds << " seconds" << NL;

    if (de_facto_p == p)
    {
        required_position = TP_UNKNOWN;
    }
    else
    {
        required_position = p;
        LOGGER << "Future position is " << GetPositionText(required_position) << NL;
    }

    st_tris_position  = (manual) ? TP_UNKNOWN : de_facto_p;
}
//------------------------------------------------------
static void set_motor_state(void* ctx)
{
    TrisPosition* p = (TrisPosition*)ctx;
    set_current_position(*p);
    Motor::Schedule();
}
//------------------------------------------------------
struct SchedulingTimes
{
    FixTime now;
    DstTime dst;
    FixTime sun_rise;
    FixTime sun_set;

    Event events[8];
    int day_first_event_idx;
    int max_idx;

    TrisPosition current_position;
    Event*       next_event;

    void Schedule()
    {
        memset(events, 0, sizeof(events));

        now = FixTime::Now();
        dst = DstTime(now);

        Sun::GetTodayLocalRiseSetTimes(sun_rise, sun_set);

        if (settings.states.nightly.mode != NM_DISABLED)
        {
            LOGGER << "NIGHTLY is enabled" << NL;
            FixTime down_time = GetFromMinutes(dst, settings.states.nightly.down);

            if (down_time <= now)
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

            events[0].p = events[4].p = (settings.states.nightly.mode == NM_AIR) ? TP_Air : TP_Btm;

            events[0].d = events[4].d = (settings.states.nightly.mode == NM_AIR) ? "Nightly AIR" : "Nightly BOTTOM";

            if (sun_rise < down_time)
            {
                // sun_rise of yesterday
                Sun::GetLocalRiseSetTimes(sun_rise + SECONDS_PER_DAY, sun_rise, sun_set);
            }

            FixTime up_time = (Settings::SUNRISE == settings.states.nightly.up) ? sun_rise : GetFromMinutes(dst, settings.states.nightly.up);

            if (up_time < down_time)
            {
                // passee
                events[1].t = up_time + SECONDS_PER_DAY;
                events[5].t = up_time + (SECONDS_PER_DAY * 2);
            }
            else
            {
                // in future
                events[1].t = up_time;
                events[5].t = up_time + SECONDS_PER_DAY;
            }

            events[1].p = events[5].p = TP_Top;

            events[1].d = events[5].d = "Nightly TOP";
        }

        //TRACE("settings.states.sun_protect.on");
        if (settings.states.sun_protect.on)
        {
            LOGGER << "SUN PROTECT is enabled" << NL;
            LOGGER << "Sunrise at " << DstTime(sun_rise).ToText() << NL;

            FixTime down_time = sun_rise + (SECONDS_PER_MINUTE * (int)settings.states.sun_protect.minutes_after_sun_rise);

            if (down_time < events[0].t)
                down_time += SECONDS_PER_DAY;

            events[2].t = down_time;
            events[6].t = down_time + SECONDS_PER_DAY;

            events[2].p = events[6].p = TP_Sun;
            events[2].d = events[6].d = "Sun PROTECTED";

            FixTime up_time = down_time + +(SECONDS_PER_MINUTE * (int)settings.states.sun_protect.duration_minutes);

            events[3].t = up_time;
            events[7].t = up_time + SECONDS_PER_DAY;

            events[3].p = events[7].p = TP_Top;
            events[3].d = events[7].d = "Sun UNPROTECTED";
        }

        max_idx = countof(events);

        //ListEvents("Before removing unset events");

        // remove unset events
        int idx;
        for (idx = 0; idx < max_idx; idx++)
        {
            if (events[idx].p == TP_UNKNOWN)
            {
                max_idx--;

                if (idx < max_idx)
                {
                    int shift_size = sizeof(*events) * (max_idx - idx);
                    memcpy(events + idx, events + idx + 1, shift_size);
                }

                idx--;
            }
        }

        //ListEvents("Before removing overlapped events");

        // remove overlapped events
        for (idx = 0; idx < max_idx - 1; idx++)
        {
            if (events[idx].t > events[idx + 1].t)
            {
                max_idx--;
                int shift_size = sizeof(*events) * (max_idx - idx);
                memcpy(events + idx, events + idx + 1, shift_size);
                idx--;
            }
        }

        //ListEvents("After removing overlapped events");

        current_position = TP_Top;
        next_event       = NULL;

        for (idx = 0; idx < max_idx; idx++)
        {
            if (events[idx].t > now)
            {
                next_event = &events[idx];
                break;
            }

            current_position = events[idx].p;
        }

        LOGGER << "Current position: " << GetPositionText(current_position) << NL;
        if (next_event)
        {
            LOGGER << "Next scheduled  : " << GetPositionText(next_event->p) << " at " << DstTime(next_event->t).ToText() << NL;
        }
        else
        {
            if (max_idx)
                ErrorMgr::Report("Bug in SchedulingTimes::Schedule()");
        }

        // Set the first event of the day
        Times tm  = dst;
        day_t day = tm.D;

        day_first_event_idx = 778;

        for (idx = 0; idx < max_idx; idx++)
        {
            DstTime event_dst = events[idx].t;
            Times   event_tm  = event_dst;
            day_t   event_day = event_tm.D;

            if (day == event_day)
            {
                day_first_event_idx = idx;
                break;
            }
        }
    }

    void ListEvents(const char* title)
    {
        LOGGER << title << " : " << max_idx << " events scheduled:";

        for (int idx = 0; idx < max_idx; idx++)
        {
            if (events[idx].p)
                LOGGER << "\n>       " << DstTime(events[idx].t).ToText() << " : " << GetPositionText(events[idx].p) << " = " << events[idx].d;
            else
                LOGGER << "\n>       <NOT SET>";
        }

        LOGGER << NL;
    }
} scheduling_times;
//------------------------------------------------------
void Motor::Schedule()
{
    if (gbl_State == Error)
        return;

    if (settings.states.manual)
    {
        set_power(false);
        Scheduler::Cancel(next_position_handler);
        return;
    }

    LOGGER << "Scheduling..." << NL;

    set_power(true);

    scheduling_times.Schedule();

    set_current_position(scheduling_times.current_position);

    Scheduler::Cancel(next_position_handler);

    if (scheduling_times.next_event)
    {
        next_position_handler = Scheduler::Add(set_motor_state, &scheduling_times.next_event->p, scheduling_times.next_event->d, scheduling_times.next_event->t);
    }
}
//------------------------------------------------------
void Motor::PowerOff()
{
    SetPowerOnTimer.Stop();
    set_power(false);
}
//------------------------------------------------------
void Motor::TogglePower()
{
    bool is_on = pPowerSwitchRelay->Get();

    if (is_on)
    {
        SetPowerOnTimer.StartOnce(1 * SECONDS_PER_MINUTE * 1000);
    }
    else
    {
        SetPowerOnTimer.Stop();
    }

    set_power(!is_on);
}
//------------------------------------------------------
static String get_state_text()
{
    if (settings.states.manual)
    {
        return "MANUAL state";
    }

    State state = gbl_State;

    switch (state)
    {
        case Ready:         return "Ready";
        case Manual:        return "Powered OFF";

        case BusyUp:
        case BusyDown:      
        {
            char text[32];
            int delta_seconds = rolling_seconds - ((millis() - rolling_started) / 1000);
            sprintf(text, "Rolling %s for %d seconds", (state == BusyUp ? "UP" : "DOWN"), delta_seconds);

            return text;
        }
    }

    return "?";
}
//------------------------------------------------------
static String get_time(const FixTime& t)
{
    DstTime dst = t;
    TimeText tt = dst.ToText();
    tt.buffer[16] = 0;
    return tt.buffer + 11;
}
//------------------------------------------------------
static String get_curpos_text()
{
    switch (st_tris_position)
    {
        case TP_Top: return "TOP";
        case TP_Sun: return "SUN";
        case TP_Air: return "AIR";
        case TP_Btm: return "BTM";
    }
    
    return "???";
}
//------------------------------------------------------
static String get_position_description(TrisPosition p)
{
    switch (p)
    {
        case TP_Top: return "פתוח";                         
        case TP_Sun: return "פתוח מוגן שמש)";
        case TP_Air: return "סגור עם מרווח איוורור";       
        case TP_Btm: return "סגור עד למטה";                
    }

    return "בלתי ידוע";                       
}
//------------------------------------------------------
static String get_action_description(TrisPosition p)
{
    switch (p)
    {
        case TP_Top: return "פתיחה";                 
        case TP_Sun: return "פתיחה מוגנת שמש";  
        case TP_Air: return "סגירה עם מרווח איוורור";        
        case TP_Btm: return "סגירה עד למטה";                  
    }

    return "?";
}
//------------------------------------------------------
String get_day_action(int idx)
{
    if (settings.states.manual)
        return "";

    idx += scheduling_times.day_first_event_idx;
    if (idx >= scheduling_times.max_idx)
        return "";
    return get_action_description(scheduling_times.events[idx].p);
}
//------------------------------------------------------
String get_day_time(int idx)
{
    if (settings.states.manual)
        return "";

    idx += scheduling_times.day_first_event_idx;
    if (idx >= scheduling_times.max_idx)
        return "";

   return get_time(scheduling_times.events[idx].t);
}
//------------------------------------------------------
bool SetActionDisabled(const String& var, String& action_disabled_reason)
{
    if (var == "ACTION_DISABLED")
    {
        if (Error == gbl_State)
        {
            LOGGER << "popup: Under ERROR" << NL;
            action_disabled_reason = "Under ERROR";
        }
        else
        if (settings.states.manual)
        {
            LOGGER << "popup: System is in MANUAL state" << NL;
            action_disabled_reason = "System is in MANUAL state";
        }
        else
        action_disabled_reason = "";

        return true;
    }

    return false;
}
//------------------------------------------------------
static String processor(const String& var)
{
    if (var == "STATE")
        return get_state_text();

    String retval;

    if (var == "CURRENT_POSITION")
    {
        retval = "מצב נוכחי: ";
        retval += get_position_description(st_tris_position);

        return retval;
    }

    if (var == "NEXT_EVENT")
    {
        if (!scheduling_times.next_event || !Scheduler::IsScheduled(next_position_handler))
            return "";

        retval = "הפעולה הבאה: ";
        String when = get_time(scheduling_times.next_event->t);
        String description = get_action_description(scheduling_times.next_event->p);

        retval += description + " בשעה " + when;

        return retval;
    }

    if (var == "CURPOS")
    {
        return get_curpos_text();
    }

    if (var == "SUNRISE")
    {
        if (settings.states.manual)
            return "";

        return get_time(scheduling_times.sun_rise);
    }

    if (var == "SUNSET")
    {
        if (settings.states.manual)
            return "";

        return get_time(scheduling_times.sun_set);
    }

    #define EVENT(N)\
        if (var == "A" #N)    { return get_day_action(N); }     \
        if (var == "T" #N)    { return get_day_time(N); }

    EVENT(0);
    EVENT(1);
    EVENT(2);
    EVENT(3);

    CheckActionDisabled(var);

    return "";
}
//------------------------------------------------------
static String get_position_and_state_text()
{
    return get_curpos_text() + get_state_text();
}
//------------------------------------------------------
void Motor::AddWebServices(AsyncWebServer& server)
{
    server.on("/motor_manual", HTTP_GET, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
        request->send(SPIFFS, "/motor_manual.html", String(), false, processor);
        });

    server.on("/motor_up", HTTP_POST, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
        if(gbl_State == Ready)
            set_current_position(TP_Top, true);
        });

    server.on("/motor_down", HTTP_POST, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
        if (gbl_State == Ready)
            set_current_position(TP_Btm, true);
        });

    server.on("/motor_stop", HTTP_POST, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
        if (gbl_State != Ready)
            stop();
        });

    server.on("/motor_toggle_power", HTTP_POST, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
        TogglePower();
        st_tris_position = TP_UNKNOWN;
        });

    server.on("/motor_state", HTTP_GET, [](AsyncWebServerRequest *request) {
        SendText(get_state_text().c_str(), *request);
        });

    server.on("/motor_position_and_state", HTTP_GET, [](AsyncWebServerRequest *request) {
        SendText(get_position_and_state_text().c_str(), *request);
        });

    server.on("/motor_status", HTTP_GET, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
//        if (action_disabled(*request))   return;
        request->send(SPIFFS, "/motor_status.html", String(), false, processor);
        });

    server.on("/motor_day_times", HTTP_GET, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
//        if (action_disabled(*request))   return;
        request->send(SPIFFS, "/motor_day_times.html", String(), false, processor);
        });

    server.on("/motor_positions", HTTP_GET, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
//        if (action_disabled(*request))   return;
        request->send(SPIFFS, "/motor_positions.html", String(), false, processor);
        });

    server.on("/motor_top", HTTP_POST, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
        if (gbl_State == Ready)
            set_current_position(TP_Top);
        });
    server.on("/motor_sun", HTTP_POST, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
        if (gbl_State == Ready)
            set_current_position(TP_Sun);
        });
    server.on("/motor_air", HTTP_POST, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
        if (gbl_State == Ready)
            set_current_position(TP_Air);
        });
    server.on("/motor_btm", HTTP_POST, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
        if (gbl_State == Ready)
            set_current_position(TP_Btm);
        });

}
//------------------------------------------------------
