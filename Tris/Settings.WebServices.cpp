#include "WebServices.h"
#include "Settings.h"

#define _IndentAttribute(mul, add) (*new Html::StyleAttribute(mul * (ctx.depth + add)))
#define FieldIndentAttribute  _IndentAttribute(4, 1)
#define RootIndentAttribute   _IndentAttribute(2, 0)

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
    retval.AddAttribute(FieldIndentAttribute);
    return retval;
}

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

#define GET_NUMERIC_PARAM(type, fld, min, max, val, parent_fld)    if (Get##type##Param(*request, #fld, min, max, true, val))   { temp.parent_fld.fld = val; } else;

#define GET_LONG_PARAM(fld, min, max, val, parent_fld)    GET_NUMERIC_PARAM(Long, fld, min, max, val, parent_fld)
#define GET_BOOL_PARAM(fld, val, parent_fld)              if (GetBoolParam(*request, #fld, val))      temp.parent_fld.fld = val; else;

#if 1
#undef TRACE
#define TRACE( extra )  if(TRACING) ; else
#endif

#define SendSettings(func_id)  \
    Html::TextGeneratorCtx ctx;\
    TRACE("before get" #func_id);\
    Html::Element& e = get##func_id(ctx);\
    TRACE("before SendElement");\
    SendElement( e, *request );\
    TRACE("before delete &e");\
    delete &e

#define SetRoot(name, url)    \
    Html::Element& root = *new Html::Paragraph();\
    const char* element_name = #name ":";\
    if (ctx.depth)    {\
        Html::h1& h1 = *new Html::h1();\
        h1.AddAttribute(RootIndentAttribute);\
        root.AddChild(h1);\
        h1.AddChild(*new Html::Anchor(element_name, url)); }\
    else { root.AddChild(*new Html::h1(element_name)); }

//------------------------------------------------------
static Html::Element& getSettings(Html::TextGeneratorCtx& ctx)
{
    SetRoot(Settings, "/settings");

    ctx.depth++;
    root.AddChild(getSettingsStates(ctx));
    root.AddChild(getSettingsTimings(ctx));
    ctx.depth--;

    return root;
}
//------------------------------------------------------
static void WS_Settings(AsyncWebServerRequest *request)
{
    SendSettings(Settings);
}
//------------------------------------------------------
static Html::Element& getSettingsStates(Html::TextGeneratorCtx& ctx)
{
    SetRoot(States, "/settings/states");

    root.AddChild(create_field("manual", String((int)settings.states.manual), ctx));
    root.AddChild(create_field("DST", String((int)settings.states.DST), ctx));

    ctx.depth++;
    root.AddChild(getSettingsStatesNightly(ctx));
    root.AddChild(getSettingsStatesSunProtect(ctx));
    ctx.depth--;

    return root;
}
//------------------------------------------------------
static void WS_SettingsStates(AsyncWebServerRequest *request)
{
    Settings temp = settings;
    bool b;

//    GET_BOOL_PARAM(manual, b, states);
//    GET_BOOL_PARAM(DST, b, states);

    Settings::Store(temp);

    SendSettings(SettingsStates);
}
//------------------------------------------------------
static String per_day_minutes(uint16_t minutes)
{
    char text[13];
    sprintf(text, "%02d:%02d", minutes / MINUTES_PER_HOUR, minutes % MINUTES_PER_HOUR);
    return text;
}
//------------------------------------------------------
static const char* GetText(NightlyMode mode)
{
    switch (mode)
    {
#define TREATE_CASE(id) case NM_##id : return #id
        TREATE_CASE(DISABLED);
        TREATE_CASE(AIR);
        TREATE_CASE(ALL);
#undef  TREATE_CASE
        return "?";
    }
}
//------------------------------------------------------
static Html::Element& getSettingsStatesNightly(Html::TextGeneratorCtx& ctx)
{
    SetRoot(Nightly, "/settings/states/nightly");

    root.AddChild(create_field("mode", GetText(settings.states.nightly.mode), ctx));
    root.AddChild(create_field("down", per_day_minutes(settings.states.nightly.down), ctx));
    root.AddChild(create_field("up", (settings.states.nightly.up == 0xFFFF) ? String("SUNRISE") : per_day_minutes(settings.states.nightly.up), ctx));

    return root;
}
//------------------------------------------------------
static void WS_SettingsStatesNightly(AsyncWebServerRequest *request)
{
    SendSettings(SettingsStatesNightly);
}
//------------------------------------------------------
static Html::Element& getSettingsStatesSunProtect(Html::TextGeneratorCtx& ctx)
{
    SetRoot(SunProtect, "/settings/states/sun_protect");

    root.AddChild(create_field("on", String((int)settings.states.sun_protect.on), ctx));
    root.AddChild(create_field("minutes_after_sun_rise", String((int)settings.states.sun_protect.minutes_after_sun_rise), ctx));
    root.AddChild(create_field("duration_minutes", String((int)settings.states.sun_protect.duration_minutes), ctx));

    return root;
}
//------------------------------------------------------
static void WS_SettingsStatesSunProtect(AsyncWebServerRequest *request)
{
    SendSettings(SettingsStatesSunProtect);
}
//------------------------------------------------------
static Html::Element& getSettingsTimings(Html::TextGeneratorCtx& ctx)
{
    SetRoot(Timings, "/settings/timings");

    ctx.depth++;
    root.AddChild(getSettingsTimingsUp(ctx));
    root.AddChild(getSettingsTimingsDown(ctx));
    ctx.depth--;

    return root;
}
//------------------------------------------------------
static void WS_SettingsTimings(AsyncWebServerRequest *request)
{
    SendSettings(SettingsTimings);
}
//------------------------------------------------------
static void addTimings(Html::Element& root, const Settings::Timings::Direction& direction, Html::TextGeneratorCtx& ctx)
{
 //   ctx.depth++;
    root.AddChild(create_field("all", String(direction.all, 1), ctx));
    root.AddChild(create_field("air", String(direction.air, 1), ctx));
    root.AddChild(create_field("sun", String(direction.sun, 1), ctx));
//    ctx.depth--;
}
//------------------------------------------------------
static Html::Element& getSettingsTimingsUp(Html::TextGeneratorCtx& ctx)
{
    SetRoot(Up, "/settings/timings/up");

    addTimings(root, settings.timings.up, ctx);

    return root;
}
//------------------------------------------------------
static void WS_SettingsTimingsUp(AsyncWebServerRequest *request)
{
    SendSettings(SettingsTimingsUp);
}
//------------------------------------------------------
static Html::Element& getSettingsTimingsDown(Html::TextGeneratorCtx& ctx)
{
    SetRoot(Down, "/settings/timings/down");

    addTimings(root, settings.timings.down, ctx);

    return root;
}
//------------------------------------------------------
static void WS_SettingsTimingsDown(AsyncWebServerRequest *request)
{
    SendSettings(SettingsTimingsDown);
}
//------------------------------------------------------

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
}
//------------------------------------------------------
