#include "WebServices.h"
#include "Settings.h"

//------------------------------------------------------
//	HTTP handlers
//------------------------------------------------------

#define DECLARE_HTTP_FUNCS(name)    \
        static void WS_##name(AsyncWebServerRequest *request);\
        static Html::Element& get##name(Html::TextGeneratorCtx& ctx)

DECLARE_HTTP_FUNCS(Settings);
DECLARE_HTTP_FUNCS(SettingsStates);
DECLARE_HTTP_FUNCS(SettingsStatesNightly);
DECLARE_HTTP_FUNCS(SettingsStatesSunProtect);
DECLARE_HTTP_FUNCS(SettingsTimings);
DECLARE_HTTP_FUNCS(SettingsTimingsUp);
DECLARE_HTTP_FUNCS(SettingsTimingsDown);
//------------------------------------------------------
#if 1
#undef TRACE
#define TRACE( extra )  if(TRACING) ; else
#endif
//------------------------------------------------------

//------------------------------------------------------
static bool check_busy(AsyncWebServerRequest& request)
{
    State state = gbl_State;
    if (BusyUp == state || BusyDown == state)
    {
        SendText("Busy!!!", request, 200);
        return true;
    }

    return false;

#define CheckBusy()     if(check_busy(*request)) return; else
}
//------------------------------------------------------
static Html::Element& getSettings(Html::TextGeneratorCtx& ctx)
{
    SetRoot(Settings, "/settings");

    ctx.depth++;
    root.AddChild(getSettingsStates(ctx));
    root.AddChild(getSettingsTimings(ctx));
    ctx.depth--;

    return _root;
}
//------------------------------------------------------
static void WS_Settings(AsyncWebServerRequest *request)
{
    CheckBusy();
    SendHtml(Settings);
}
//------------------------------------------------------
const char* GetYesNo(const bool& b)
{
    return b ? "Yes" : "No";
}
//------------------------------------------------------
static Html::Element& getSettingsStates(Html::TextGeneratorCtx& ctx)
{
    SetRoot(States, "/settings/states");

    root.AddChild(CreateField("manual", GetYesNo(settings.states.manual), ctx));
    root.AddChild(CreateField("DST", GetYesNo(settings.internet_time.dst), ctx));

    ctx.depth++;
    root.AddChild(getSettingsStatesNightly(ctx));
    root.AddChild(getSettingsStatesSunProtect(ctx));
    ctx.depth--;

    return _root;
}
//------------------------------------------------------
static void WS_SettingsStates(AsyncWebServerRequest *request)
{
    CheckBusy();

    Settings temp = settings;
    bool b;

    GET_BOOL_PARAM(manual, b, states);
	if (GetBoolParam(*request, "DST", b))      temp.internet_time.dst = b;

    Settings::Store(temp);

    SendHtml(SettingsStates);
}
//------------------------------------------------------
static String per_day_minutes(uint16_t minutes)
{
    char text[13];
    sprintf(text, "%02d:%02d", minutes / MINUTES_PER_HOUR, minutes % MINUTES_PER_HOUR);
    return text;
}
//------------------------------------------------------
static void parse_per_day_minutes(const String& s, uint16_t& minutes)
{
    int h, m;
    char c;

    if (sscanf(s.c_str(), "%d%c%d", &h, &c, &m) != 3)
    {
        LOGGER << "parse_per_day_minutes #1" << NL;
        return;
    }

    if (h < 0 || h >= HOURS_PER_DAY)
    {
        LOGGER << "parse_per_day_minutes #2" << NL;
        return;
    }

    if (m < 0 || m >= MINUTES_PER_HOUR)
    {
        LOGGER << "parse_per_day_minutes #3" << NL;
        return;
    }

    minutes = (h * MINUTES_PER_HOUR) + m;
}
//------------------------------------------------------
static const char* GetText(const NightlyMode& mode)
{
    switch (mode)
    {
        #define TREATE_CASE(id) case NM_##id : return #id
        TREATE_CASE(DISABLED);
        TREATE_CASE(AIR);
        TREATE_CASE(ALL);
        #undef  TREATE_CASE
    }

    return "?";
}
//------------------------------------------------------
static Html::Element& getSettingsStatesNightly(Html::TextGeneratorCtx& ctx)
{
    SetRoot(Nightly, "/settings/states/nightly");

    root.AddChild(CreateField("mode", GetText(settings.states.nightly.mode), ctx));
    root.AddChild(CreateField("down", per_day_minutes(settings.states.nightly.down), ctx));
    root.AddChild(CreateField("up", (settings.states.nightly.up == Settings::SUNRISE) ? String("SUNRISE") : per_day_minutes(settings.states.nightly.up), ctx));

    return _root;
}
//------------------------------------------------------
static void WS_SettingsStatesNightly(AsyncWebServerRequest *request)
{
    CheckBusy();

    Settings temp = settings;

    String val;

    if (GetStringParam(*request, "mode", val))
    {
        val.toUpperCase();

        for(int nm = __min_NightlyMode__; nm <= __max_NightlyMode__; nm++)
            if (val == GetText((NightlyMode)nm))
            {
                temp.states.nightly.mode = (NightlyMode)nm;
                break;
            }
    }
    
    if (GetStringParam(*request, "down", val))
        parse_per_day_minutes(val, temp.states.nightly.down);

    if (GetStringParam(*request, "up", val))
    {
        val.toUpperCase();

        if (val == "SUNRISE")    temp.states.nightly.up = Settings::SUNRISE;
        else parse_per_day_minutes(val, temp.states.nightly.up);
    }

    Settings::Store(temp);

    SendHtml(SettingsStatesNightly);
}
//------------------------------------------------------
static Html::Element& getSettingsStatesSunProtect(Html::TextGeneratorCtx& ctx)
{
    SetRoot(SunProtect, "/settings/states/sun_protect");

    root.AddChild(CreateField("on", GetYesNo(settings.states.sun_protect.on), ctx));
    root.AddChild(CreateField("minutes_after_sun_rise", String((int)settings.states.sun_protect.minutes_after_sun_rise), ctx));
    root.AddChild(CreateField("duration_minutes", String((int)settings.states.sun_protect.duration_minutes), ctx));

    return _root;
}
//------------------------------------------------------
static void WS_SettingsStatesSunProtect(AsyncWebServerRequest *request)
{
    CheckBusy();

    Settings temp = settings;
    bool b;
    uint8_t ub;

    GET_BOOL_PARAM(on, b, states.sun_protect);
    GET_UNSIGNED_BYTE_PARAM(minutes_after_sun_rise, 0, 240, ub, states.sun_protect);
    GET_UNSIGNED_BYTE_PARAM(duration_minutes,       1, 240, ub, states.sun_protect);

    Settings::Store(temp);

    SendHtml(SettingsStatesSunProtect);
}
//------------------------------------------------------
static Html::Element& getSettingsTimings(Html::TextGeneratorCtx& ctx)
{
    SetRoot(Timings, "/settings/timings");

    ctx.depth++;
    root.AddChild(getSettingsTimingsUp(ctx));
    root.AddChild(getSettingsTimingsDown(ctx));
    ctx.depth--;

    return _root;
}
//------------------------------------------------------
static void WS_SettingsTimings(AsyncWebServerRequest *request)
{
    CheckBusy();

    SendHtml(SettingsTimings);
}
//------------------------------------------------------
static void addTimings(Html::Element& root, const Settings::Timings::Direction& direction, Html::TextGeneratorCtx& ctx)
{
 //   ctx.depth++;
    root.AddChild(CreateField("all", String(direction.all, 1), ctx));
    root.AddChild(CreateField("air", String(direction.air, 1), ctx));
    root.AddChild(CreateField("sun", String(direction.sun, 1), ctx));
//    ctx.depth--;
}
//------------------------------------------------------
static Html::Element& getSettingsTimingsUp(Html::TextGeneratorCtx& ctx)
{
    SetRoot(Up, "/settings/timings/up");

    addTimings(root, settings.timings.up, ctx);

    return _root;
}
//------------------------------------------------------
static void WS_SettingsTimingsUp(AsyncWebServerRequest *request)
{
    CheckBusy();

    Settings temp = settings;

    double d;
    GET_DOUBLE_PARAM(all, 15, 60, d, timings.up);
    GET_DOUBLE_PARAM(air, 2,  12, d, timings.up);
    GET_DOUBLE_PARAM(sun, 10, 30, d, timings.up);

    Settings::Store(temp);

    SendHtml(SettingsTimingsUp);
}
//------------------------------------------------------
static Html::Element& getSettingsTimingsDown(Html::TextGeneratorCtx& ctx)
{
    SetRoot(Down, "/settings/timings/down");

    addTimings(root, settings.timings.down, ctx);

    return _root;
}
//------------------------------------------------------
static void WS_SettingsTimingsDown(AsyncWebServerRequest *request)
{
    CheckBusy();

    Settings temp = settings;

    double d;
    GET_DOUBLE_PARAM(all, 15, 60, d, timings.down);
    GET_DOUBLE_PARAM(air, 18, 28, d, timings.down);
    GET_DOUBLE_PARAM(sun, 10, 30, d, timings.down);

    Settings::Store(temp);

    SendHtml(SettingsTimingsDown);
}
//------------------------------------------------------
static String processor(const String& var)
{
    if (var == "AUTO")
    {
        return !settings.states.manual ? "checked" : "";
    }

    if (var == "NIGHTLY")
    {
        return settings.states.nightly.mode != NM_DISABLED ? "checked" : "";
    }

    if (var == "AIRFUL")
    {
        return settings.states.nightly.mode == NM_AIR ? "checked" : "";
    }

    if (var == "SUN_PROTECT")
    {
        return settings.states.sun_protect.on ? "checked" : "";
    }

    if (var == "DST")
    {
        return settings.internet_time.dst ? "checked" : "";
    }

    return "";
}
//------------------------------------------------------
static void WS_SetShortSettings(AsyncWebServerRequest *request)
{
    LOGGER << request->url() << NL;
    Settings temp = settings;

    String arg;
    
    GetStringParam(*request, "AUTO", arg);
    temp.states.manual = arg != "1";

    GetStringParam(*request, "NIGHTLY", arg);
    if (arg == "1")
    {
        GetStringParam(*request, "AIRFUL", arg);
        temp.states.nightly.mode = arg == "1" ? NM_AIR : NM_ALL;
    }
    else
    {
        temp.states.nightly.mode = NM_DISABLED;
    }

    GetStringParam(*request, "SUN_PROTECT", arg);
    temp.states.sun_protect.on = arg == "1";

    GetStringParam(*request, "DST", arg);
    temp.internet_time.dst = arg == "1";

    Settings::Store(temp);
}
//------------------------------------------------------
void Settings::AddWebServices(AsyncWebServer& server)
{
    server.on("/settings/timings/up",         HTTP_GET, WS_SettingsTimingsUp);
    server.on("/settings/timings/down",       HTTP_GET, WS_SettingsTimingsDown);
    server.on("/settings/timings",            HTTP_GET, WS_SettingsTimings);
    server.on("/settings/states/nightly",     HTTP_GET, WS_SettingsStatesNightly);
    server.on("/settings/states/sun_protect", HTTP_GET, WS_SettingsStatesSunProtect);
    server.on("/settings/states",             HTTP_GET, WS_SettingsStates);
    server.on("/settings",                    HTTP_GET, WS_Settings);
    server.on("/short_settings",              HTTP_GET, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
        request->send(SPIFFS, "/short_settings.html", String(), false, processor);
        });
    server.on("/set_short_settings", HTTP_POST, WS_SetShortSettings);

}
//------------------------------------------------------
