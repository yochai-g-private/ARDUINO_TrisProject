
#include "WebServices.h"

#if 0
const char* ssid     = "HomeNet";
const char* password = "1357864200";

#if 1

class String;
typedef unsigned char __uint8_t;
typedef __uint8_t uint8_t;
typedef unsigned int __uint32_t;
typedef __uint32_t uint32_t;

#include <Hash.h>
#include <SPI.h>

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <Wire.h>

#include "Logger.h"

AsyncWebServer server(80);

void InitializeWebServices()
{
    // Initialize SPIFFS
    if (!SPIFFS.begin()) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi..");
    }

    // Print ESP32 Local IP Address
    Serial.println(WiFi.localIP());

    server.onNotFound([](AsyncWebServerRequest *request) {
        LOGGER << "Not found" << NL;
        request->send_P(400, "text/plain", "blablabla");
        });

    // Start server
    server.begin();
}

#else
#include "NYG.h"
#include "SmartHomeWiFiApp.h"

#include <Hash.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
//C:\Program Files (x86)\Arduino\libraries\ESPAsyncWebServer\src\ESPAsyncWebServer.h
#include <ESPAsyncWebServer.h>
#include <FS.h>

#include "Logger.h"
#include "Html.h"
#include "Settings.h"
#include "TimeEx.h"

using namespace NYG;

#define _USE_SMART_HOME_WIFI_APP    0

#if _USE_SMART_HOME_WIFI_APP
DEFINE_SMART_HOME_WIFI_APP(Tris, WIFI_AP);
#else
const char* ssid = "HomeNet";
const char* password = "1357864200";
#endif //_USE_SMART_HOME_WIFI_APP

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

//------------------------------------------------------
//	HTTP handlers
//------------------------------------------------------

static void HTTP_notFound(AsyncWebServerRequest *request);

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

static void HTTP_handleClock(AsyncWebServerRequest *request);
static void HTTP_handleRestart(AsyncWebServerRequest *request);

static const String contentType = "text/html";

#define SendElement( e )    \
    String __text = e.ToString(ctx); delete &e; request->send(request->beginResponse(200, contentType, __text))

#define SendSettings(func_id)  \
    Html::TextGeneratorCtx ctx;\
    Html::Element& e = getSettings##func_id(ctx);\
    SendElement( e )

void InitializeWebServices()
{
    static bool first_time = true;

    if (!first_time)
        return;

    first_time = true;

    // Initialize SPIFFS
    if (!SPIFFS.begin())
    {
        //LOGGER << "An Error has occurred while mounting SPIFFS" << NL;
        return;
    }

    // Connect to Wi-Fi
#if _USE_SMART_HOME_WIFI_APP
    wifi_app.ConnectToWiFi();
#else
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
    }

    LOGGER << "IP: " << WiFi.localIP().toString() << NL;
#endif //_USE_SMART_HOME_WIFI_APP

    // Route for root / web page

 /*   server.on("/settings/timings/up",               HTTP_GET, HTTP_handleSettingsTimingsUp);
    server.on("/settings/timings/down",             HTTP_GET, HTTP_handleSettingsTimingsDown);
    server.on("/settings/timings",                  HTTP_GET, HTTP_handleSettingsTimings);
    server.on("/settings/states/nightly",           HTTP_GET, HTTP_handleSettingsStatesNightly);
    server.on("/settings/states/sun_protect",       HTTP_GET, HTTP_handleSettingsStatesSunProtect);
    server.on("/settings/states",                   HTTP_GET, HTTP_handleSettingsStates);
    server.on("/settings",                          HTTP_GET, HTTP_handleSettings);
    server.on("/clock",                             HTTP_GET, HTTP_handleClock);
    server.on("/restart",                           HTTP_GET, HTTP_handleRestart);*/

    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send_P(400, "text/plain", "hahahhahahahhahahah!!!!");
        });
//    server.onNotFound(HTTP_notFound);

    // Start server
    server.begin();
}
//------------------------------------------------------
//#define IF_ARG(fld)    val = request->arg(#fld); if (*val.c_str())

static bool GetLongParam(AsyncWebServerRequest *request, const char* fld, long& val)
{
    const String& arg = request->arg(fld);
    if (!*arg.c_str())
        return false;

    val = arg.toInt();
    LOGGER << arg << " = " << (uint32_t)val << NL;

    return true;
}

static bool GetBoolParam(AsyncWebServerRequest *request, const char* fld, bool& val)
{
    String arg = request->arg(fld);
    if (!*arg.c_str())
        return false;

    arg.toUpperCase();

    if      (arg == "YES" || arg == "ON"  || arg == "TRUE"  || arg == "1")  val = true;
    else if (arg == "NO"  || arg == "OFF" || arg == "FALSE" || arg == "0")  val = false;
    else    return false;

    return true;
}

#define GET_NUMERIC_PARAM(type, fld, min, max, val, parent_fld)    {\
         if (Get##type##Param(request, #fld, val))   {\
            if (val < min)          val = min;\
            else if (val > max)     val = max;\
            temp.parent_fld.fld = val;    } }

#define GET_LONG_PARAM(fld, min, max, val, parent_fld)    GET_NUMERIC_PARAM(Long, fld, min, max, val, parent_fld)
#define GET_BOOL_PARAM(fld, val, parent_fld)              if (GetBoolParam(request, #fld, val))      temp.parent_fld.fld = val;

//------------------------------------------------------
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

static void HTTP_notFound(AsyncWebServerRequest *request)
{
    request->send(request->beginResponse(400, contentType, "Not found!!!"));
}
//------------------------------------------------------
static void HTTP_handleSettings(AsyncWebServerRequest *request)
{
    //LOGGER << __FUNCTION__ << NL;
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
    GET_BOOL_PARAM(DST,    b, states);

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

    root.AddChild(create_field("mode",          String((int)settings.states.nightly.mode),      ctx));
    root.AddChild(create_field("down",          per_day_minutes(settings.states.nightly.down),  ctx));
    root.AddChild(create_field("up",            (settings.states.nightly.up == 0xFFFF) ? String("SUNRISE") : per_day_minutes(settings.states.nightly.up),    ctx));

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

    root.AddChild(create_field("on",                         String((int)settings.states.sun_protect.on),                        ctx));
    root.AddChild(create_field("minutes_after_sun_rise",     String((int)settings.states.sun_protect.minutes_after_sun_rise),    ctx));
    root.AddChild(create_field("duration_minutes",           String((int)settings.states.sun_protect.duration_minutes),          ctx));

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
    //LOGGER << __FUNCTION__ << NL;

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
    //LOGGER << __FUNCTION__ << NL;
    //    update_SettingsTimings(request, settings.timings.up);
}
//------------------------------------------------------
static void HTTP_handleSettingsTimingsDown(AsyncWebServerRequest *request)
{
    //LOGGER << __FUNCTION__ << NL;
    //    update_SettingsTimings(request, settings.timings.up);
}
//------------------------------------------------------
static void HTTP_handleClock(AsyncWebServerRequest *request)
{
    String text = "Current time: ";
    text += DstTime::NowToText();
    Html::Element& root = *new Html::h1(text.c_str());

    Html::TextGeneratorCtx ctx;
    SendElement(root);
}
//------------------------------------------------------
static void HTTP_handleRestart(AsyncWebServerRequest *request)
{

}
//------------------------------------------------------


#endif 
#endif