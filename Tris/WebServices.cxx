#include "WebServices.h"
#include "SmartHomeWiFiApp.h"
#include "MicroController.h"

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
String processor(const String& var) {
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

// Replace with your network credentials
const char* ssid = "HomeNet";	//"ESP8266_Demo";
const char* password = "1357864200";	//"5708";
DEFINE_SMART_HOME_WIFI_APP(Tris, WIFI_STA);
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

static void add_test_web_messages();
static void WS_clock(AsyncWebServerRequest *request);

void InitializeWebServices()
{
    // Initialize SPIFFS
    if (!SPIFFS.begin()) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    wifi_app.ConnectToWiFi();

    add_test_web_messages();
    Settings::AddWebServices(server);

    server.on("/clock", HTTP_GET, WS_clock);

    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
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

    if (GetStringParam(*request, "date", arg))
    {
        if (!times.ParseDate(arg.c_str()))
        {
            SendText("Invalid date", *request);
            return;
        }

        DstTime now = times;
        FixTime::Set(FixTime(now));
    }
    else if (GetStringParam(*request, "time", arg))
    {
        if (!times.ParseTime(arg.c_str()))
        {
            SendText("Invalid time", *request);
            return;
        }

        DstTime now = times;
        FixTime::Set(FixTime(now));
    }

    String text = "Current time: ";
    text += DstTime::NowToText();
    Html::h1 root(text);

    SendElement(root, *request);
}
//--------------------------------------------------------------------
static void add_test_web_messages()
{
    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/index.html", String(), false, processor);
        });

    // Route to load style.css file
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/style.css", "text/css");
        });

    // Route to set GPIO to HIGH
    server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request) {
        //gitalWrite(ledPin, HIGH);
        led_ON = true;
        request->send(SPIFFS, "/index.html", String(), false, processor);
        });

    // Route to set GPIO to LOW
    server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
        //gitalWrite(ledPin, LOW);
        led_ON = false;
        request->send(SPIFFS, "/index.html", String(), false, processor);
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

