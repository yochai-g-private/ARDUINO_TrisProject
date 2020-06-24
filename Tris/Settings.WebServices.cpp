#include "WebServices.h"
#include "Settings.h"



static String get_element_value(const char* field, const String& value)
{
    char retval[64];
    sprintf(retval, "%s = %s", field, value.c_str());
    return retval;
}
//------------------------------------------------------
Html::Element& create_field(const char* field, const String& value, Html::TextGeneratorCtx& ctx)
{
    Html::Element& retval = *new Html::h3(get_element_value(field, value).c_str());
    retval.AddAttribute(*new Html::StyleAttribute(4 * (ctx.depth + 1)));
    return retval;
}

//------------------------------------------------------
//	HTTP handlers
//------------------------------------------------------

#define DECLARE_HTTP_FUNCS(name)    \
        static void HTTP_handle##name(AsyncWebServerRequest *request);\
        static Html::Element& get##name(Html::TextGeneratorCtx& ctx)

DECLARE_HTTP_FUNCS(Settings);
DECLARE_HTTP_FUNCS(SettingsStates);
DECLARE_HTTP_FUNCS(SettingsStatesNightly);
DECLARE_HTTP_FUNCS(SettingsStatesSunProtect);
DECLARE_HTTP_FUNCS(SettingsTimings);
DECLARE_HTTP_FUNCS(SettingsTimingsUp);
DECLARE_HTTP_FUNCS(SettingsTimingsDown);

#define GET_NUMERIC_PARAM(type, fld, min, max, val, parent_fld)    if (Get##type##Param(*request, #fld, min, max, true, val))   { temp.parent_fld.fld = val; } else;

#define GET_LONG_PARAM(fld, min, max, val, parent_fld)    GET_NUMERIC_PARAM(Long, fld, min, max, val, parent_fld)
#define GET_BOOL_PARAM(fld, val, parent_fld)              if (GetBoolParam(*request, #fld, val))      temp.parent_fld.fld = val; else;

#define SendSettings(func_id)  \
    Html::TextGeneratorCtx ctx;\
    Html::Element& e = getSettings##func_id(ctx);\
    SendElement( e, *request );\
    delete &e

//------------------------------------------------------
static void HTTP_handleSettings(AsyncWebServerRequest *request)
{
    // TODO
}
//------------------------------------------------------
static Html::Element& getSettingsStates(Html::TextGeneratorCtx& ctx)
{
    Html::Element& root = *new Html::h1("States:");

    if (ctx.depth)
    {
        root.AddAttribute(*new Html::StyleAttribute(4 * ctx.depth));
    }

    root.AddChild(create_field("manual", String((int)settings.states.manual), ctx));

    ctx.depth++;
    root.AddChild(getSettingsStatesNightly(ctx));
    root.AddChild(getSettingsStatesSunProtect(ctx));
    ctx.depth--;

    root.AddChild(create_field("DST", String((int)settings.states.DST), ctx));

    return root;
}
//------------------------------------------------------
static void HTTP_handleSettingsStates(AsyncWebServerRequest *request)
{
    Settings temp = settings;
    bool b;

    GET_BOOL_PARAM(manual, b, states);
    GET_BOOL_PARAM(DST, b, states);

    Settings::Store(temp);

    SendSettings(States);
}
//------------------------------------------------------
static String per_day_minutes(uint16_t minutes)
{
    char text[13];
    sprintf(text, "%02d:%02d", minutes / MINUTES_PER_HOUR, minutes % MINUTES_PER_HOUR);
    return text;
}
//------------------------------------------------------
static Html::Element& getSettingsStatesNightly(Html::TextGeneratorCtx& ctx)
{
    Html::Element& root = *new Html::h1("Nightly:");

    if (ctx.depth)
    {
        root.AddAttribute(*new Html::StyleAttribute(4 * ctx.depth));
    }

    root.AddChild(create_field("mode", String((int)settings.states.nightly.mode), ctx));
    root.AddChild(create_field("down", per_day_minutes(settings.states.nightly.down), ctx));
    root.AddChild(create_field("up", (settings.states.nightly.up == 0xFFFF) ? String("SUNRISE") : per_day_minutes(settings.states.nightly.up), ctx));

    return root;
}
//------------------------------------------------------
static void HTTP_handleSettingsStatesNightly(AsyncWebServerRequest *request)
{
    SendSettings(StatesNightly);
}
//------------------------------------------------------
static Html::Element& getSettingsStatesSunProtect(Html::TextGeneratorCtx& ctx)
{
    Html::Element& root = *new Html::h1("SunProtect:");

    if (ctx.depth)
    {
        root.AddAttribute(*new Html::StyleAttribute(4 * ctx.depth));
    }

    root.AddChild(create_field("on", String((int)settings.states.sun_protect.on), ctx));
    root.AddChild(create_field("minutes_after_sun_rise", String((int)settings.states.sun_protect.minutes_after_sun_rise), ctx));
    root.AddChild(create_field("duration_minutes", String((int)settings.states.sun_protect.duration_minutes), ctx));

    return root;
}
//------------------------------------------------------
static void HTTP_handleSettingsStatesSunProtect(AsyncWebServerRequest *request)
{
    SendSettings(StatesSunProtect);
}
//------------------------------------------------------
static void HTTP_handleSettingsTimings(AsyncWebServerRequest *request)
{
    // TODO
}
//------------------------------------------------------
//static void update_SettingsTimings(AsyncWebServerRequest *request, Settings::Timing::Direction& d)
//{
//    //Settings temp = settings;
//    //String   val;
//    //
//    //IF_ARG(all)
//    //{
//    //    d.
//    //}
//
//    //update_settings(hold);
//}
//------------------------------------------------------
static void HTTP_handleSettingsTimingsUp(AsyncWebServerRequest *request)
{
    // TODO
    //    update_SettingsTimings(request, settings.timings.up);
}
//------------------------------------------------------
static void HTTP_handleSettingsTimingsDown(AsyncWebServerRequest *request)
{
    // TODO
    //    update_SettingsTimings(request, settings.timings.down);
}
//------------------------------------------------------

//------------------------------------------------------
void Settings::AddWebServices(AsyncWebServer& server)
{
    //server.on("/settings/timings/up",         HTTP_GET, HTTP_handleSettingsTimingsUp);
    //server.on("/settings/timings/down",       HTTP_GET, HTTP_handleSettingsTimingsDown);
    //server.on("/settings/timings",            HTTP_GET, HTTP_handleSettingsTimings);
      server.on("/settings/states/nightly",     HTTP_GET, HTTP_handleSettingsStatesNightly);
    //server.on("/settings/states/sun_protect", HTTP_GET, HTTP_handleSettingsStatesSunProtect);
    //server.on("/settings/states",             HTTP_GET, HTTP_handleSettingsStates);
    //server.on("/settings",                    HTTP_GET, HTTP_handleSettings);
}
//------------------------------------------------------
