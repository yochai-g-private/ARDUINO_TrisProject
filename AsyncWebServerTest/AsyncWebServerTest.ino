/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  https://randomnerdtutorials.com/esp8266-web-server-spiffs-nodemcu/
*/

// Import required libraries
#include <Hash.h>
#include <SPI.h>

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
//#include <Wire.h>
//#include <Adafruit_Sensor.h>
//#include <Adafruit_BME280.h>
#include <BME280I2C.h>
#include <Wire.h>

//Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI
BME280I2C bme;

// Replace with your network credentials
const char* ssid 		= "HomeNet";	//"ESP8266_Demo";
const char* password 	= "1357864200";	//"5708";

// Set LED GPIO
const int ledPin = 2;
// Stores LED state
String ledState;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

String getTemperature() {
// float temperature = bme.readTemperature();
  float temperature = bme.temp();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float t = dht.readTemperature(true);
  Serial.println(temperature);
  return String(temperature);
}
  
String getHumidity() {
//  float humidity = bme.readHumidity();
  float humidity = bme.hum();
  Serial.println(humidity);
  return String(humidity);
}

String getPressure() {
//  float pressure = bme.readPressure()/ 100.0F;
  float pressure = bme.pres(BME280::PresUnit_hPa);
  Serial.println(pressure);
  return String(pressure);
}

// Replaces placeholder with LED state value
String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if(digitalRead(ledPin)){
      ledState = "ON";
    }
    else{
      ledState = "OFF";
    }
    Serial.print(ledState);
    return ledState;
  }
  else if (var == "TEMPERATURE"){
    return getTemperature();
  }
  else if (var == "HUMIDITY"){
    return getHumidity();
  }
  else if (var == "PRESSURE"){
    return getPressure();
  }  
}
 
void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

#if 0
  #define FIND_ADDRESS false
  Wire.begin();
  while(FIND_ADDRESS)
  {
    Serial.printf("Searching for BME280 sensor address...\n");
    for (int address=1; address <= 127; address++) {
      Wire.beginTransmission(address); // Select address
      if (!Wire.endTransmission()) {
        Serial.printf("Found device at: %02Xh\n", address); 
        break;
      }

      delay(10);
    }

    delay(5000);
  }
  
  // Initialize the sensor
//  if (!bme.begin(0x76)) {
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
#else
  bme.begin();
#endif

  // Initialize SPIFFS
  if(!SPIFFS.begin()){
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

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to set GPIO to HIGH
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, HIGH);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to set GPIO to LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, LOW);    
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getTemperature().c_str());
  });
  
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getHumidity().c_str());
  });
  
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getPressure().c_str());
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
      request->send_P(400, "text/plain", "NOT....FOUND");
      });

  // Start server
  server.begin();
}
 
void loop()
{
    bool pips = digitalRead(D1);
}
