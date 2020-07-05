#include "WebServices.h"
#include "SmartHomeWiFiApp.h"
#include "MicroController.h"

#define _USE_DEMO_WEB_SERVICES  1

#if _USE_DEMO_WEB_SERVICES
static void add_demo_web_messages();
#endif //_USE_DEMO_WEB_SERVICES

// Replace with your network credentials
const char* ssid = "HomeNet";	//"ESP8266_Demo";
const char* password = "1357864200";	//"5708";
DEFINE_SMART_HOME_WIFI_APP(Tris, WIFI_STA);
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

static void WS_clock(AsyncWebServerRequest *request);

void InitializeWebServices()
{
    // Initialize SPIFFS
    if (!SPIFFS.begin()) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    wifi_app.ConnectToWiFi();

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", String(), false, NULL);
        });

    // Route to load style.css file
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/style.css", "text/css");
        });

    server.on("/Tris.gif", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/Tris.gif", "image/gif");
        });

    server.on("/icon.gif", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/icon.gif", "image/gif");
        });

#if _USE_DEMO_WEB_SERVICES
    add_demo_web_messages();
#endif //_USE_DEMO_WEB_SERVICES

    Settings::AddWebServices(server);
    Motor::AddWebServices(server);
    ErrorMgr::AddWebServices(server);

    server.on("/maintenance", HTTP_GET, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
        request->send(SPIFFS, "/maintenance.html", String(), false, NULL);
        });

    server.on("/clock", HTTP_GET, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
        request->send(SPIFFS, "/clock.html", String(), false, NULL);
        });

    server.on("/set_clock", HTTP_POST, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
        String arg;
        Times  times;
        if (GetStringParam(*request, "now", arg))
        {
            const char* s = arg.c_str();
            if (times.ParseDateAndTime(s))
            {
                DstTime now = times;
                RTC::Set(FixTime(now));

                LOGGER << "RTC set to " << s << NL;

                Motor::Schedule();
            }
            else
            {
                LOGGER << "Invalid date&time " << s << NL;
            }
        }
        else
        {
            LOGGER << "Argument 'now' not found" << NL;
        } } );

    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
        LOGGER << request->url() << NL;
        Html::h1 root("Restarting in 10 seconds...");
        SendElement(root, *request);
        Scheduler::AddInSeconds([](void* ctx) { MicroController::Restart(); }, NULL, "Shutdown", 10);
        });

    server.onNotFound([](AsyncWebServerRequest *request) {
        if (request->url() == "/favicon.ico")    return;
        LOGGER << "URL NOT FOUND: " << request->url() << NL;
        SendText("Page not found", *request, 400);
        //request->send_P(400, "text/plain", );
        });

    // Start server
    server.begin();
}
//--------------------------------------------------------------------
static void WS_clock(AsyncWebServerRequest *request)
{
    String arg;
    Times  times = DstTime::Now();

    bool set_time = false;

    if (GetStringParam(*request, "curr", arg))
    {
        const char* s = arg.c_str();

        set_time = times.ParseDateAndTime(s)    ||
                   times.ParseDate(s)           ||
                   times.ParseTime(s);

        if(!set_time)
        {
            SendText("Invalid date&time", *request);
            return;
        }
    }
    else
    {
        if (GetStringParam(*request, "date", arg))
        {
            if (!times.ParseDate(arg.c_str()))
            {
                SendText("Invalid date", *request);
                return;
            }

            set_time = true;
        }

        if (GetStringParam(*request, "time", arg))
        {
            if (!times.ParseTime(arg.c_str()))
            {
                SendText("Invalid time", *request);
                return;
            }

            set_time = true;
        }
    }

    if (set_time)
    {
        DstTime now = times;
        RTC::Set(FixTime(now));
    }

    Settings temp = settings;
    bool b;

    GET_BOOL_PARAM(DST, b, states);

    Settings::Store(temp);

    String text = "Current time: ";
    text += DstTime::NowToText();
    Html::h1 root(text);

    root.AddChild(*new Html::h2(String("Daylight saving time: ") + GetYesNo(settings.states.DST)));

    SendElement(root, *request);
}
//--------------------------------------------------------------------
String GetElementValue(const char* field, const String& value)
{
    char retval[64];
    sprintf(retval, "%s = %s", field, value.c_str());
    return retval;
}
//------------------------------------------------------
Html::Element& CreateField(const char* field, const String& value, Html::TextGeneratorCtx& ctx)
{
    Html::Element& retval = *new Html::h3(GetElementValue(field, value).c_str());
    retval.AddAttribute(FieldIndentAttribute);
    return retval;
}
//------------------------------------------------------
void SendInstantHtml(AsyncWebServerRequest& request, Html::Element& e)
{
    Html::html html;
    //    html.SetTitle(#func_id);
    html.SetMeta();
    html.AddIcon("icon.gif", "image/gif", "32x32");
    html.AddStyleSheet("style.css");
    html.Body().AddChild(e);
    SendElement(html, request);
}
//------------------------------------------------------


#if _USE_DEMO_WEB_SERVICES

String getTemperature() {
    float temperature = 7.7;// bme.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //float t = dht.readTemperature(true);
    Serial.println(temperature);
    return String(temperature);
}

String getHumidity() {
    float humidity = 66.8;// bme.readHumidity();
    Serial.println(humidity);
    return String(humidity);
}

String getPressure() {
    float pressure = 1024;// bme.readPressure() / 100.0F;
    Serial.println(pressure);
    return String(pressure);
}

static String ledState;
static bool led_ON = false;
// Replaces placeholder with LED state value
static String processor(const String& var) {
    Serial.println(var);
    if (var == "STATE") {
        ledState = led_ON ? "ON" : "OFF";
        Serial.print(ledState);
        return ledState;
    }
    else if (var == "TEMPERATURE") {
        return getTemperature();
    }
    else if (var == "HUMIDITY") {
        return getHumidity();
    }
    else if (var == "PRESSURE") {
        return getPressure();
    }
}

static void add_demo_web_messages()
{
    // Route for root / web page
    server.on("/demo", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/demo.html", String(), false, processor);
        });

    // Route to set GPIO to HIGH
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
        //gitalWrite(ledPin, HIGH);
        led_ON = true;
        request->send(SPIFFS, "/demo.html", String(), false, processor);
        });

    // Route to set GPIO to LOW
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
        //gitalWrite(ledPin, LOW);
        led_ON = false;
        request->send(SPIFFS, "/demo.html", String(), false, processor);
        });

    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/plain", getTemperature().c_str());
        });

    server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/plain", getHumidity().c_str());
        });

    server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/plain", getPressure().c_str());
        });
}
#endif //_USE_DEMO_WEB_SERVICES

//------------------------------------------------------
