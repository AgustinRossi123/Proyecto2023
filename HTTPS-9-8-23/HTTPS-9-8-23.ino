//-----wifi---------
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#ifndef STASSID
#define STASSID "Galaxy A113976"
#define STAPSK "apjj5435"
#endif
//-----sensores-------
#include "RTClib.h"
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include "DHT.h"
#define DHTPIN 2
#define DHTTYPE DHT11
#include "HX711.h"
//------------ds1307----------
RTC_DS1307 rtc;

//-----------Neo6M------------
static const int RXPin = 14, TXPin = 12;
static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
SoftwareSerial ss(RXPin, TXPin);
float latitud;
float longitud;
//-----------DHT-----------
DHT dht(DHTPIN, DHTTYPE);

char buf[256];
String tiempoMedicion;
const char* URL = "https://next-app-api.vercel.app/api/camiones/caba-cor/sensores";
ESP8266WiFiMulti WiFiMulti;

//-----------Hx711-----------------
const int LOADCELL_DOUT_PIN = 0;
const int LOADCELL_SCK_PIN = 13;

float peso;
float pesoreal;
long reading;
HX711 scale;

void setup() {
//-------------conexion-------------------------------------------------
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(STASSID, STAPSK);
  Serial.println("setup() done connecting to ssid '" STASSID "'");
  
//------------------Ds1307-------------------------------
#ifndef ESP8266
  while (!Serial); // wait for serial port to connect. Needed for native USB
#endif

  while (! rtc.begin()) {
    
    Serial.println("Couldn't find RTC");
    delay(700);
    //Serial.flush();
  }

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

//-----------------DHT---------------

  delay(500);
 
  dht.begin();

//-------------------Neo6M-----------
  ss.begin(GPSBaud);
//-----------------hx711------------------------
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
}

void loop() {
  //----------------------DHT-------------------------------------------------
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  //----------------Ds1307----------------------
  DateTime now = rtc.now();
  tiempoMedicion = now.timestamp();

//----------------------Neo6MGPS-------------------------------------------
  while (ss.available() > 0){
    if (gps.encode(ss.read())){
      if(gps.location.isValid()){
        latitud = gps.location.lat();
        longitud = gps.location.lng();
//----------------------hx711----------------------------------------------
        if (scale.is_ready()) {
            reading = scale.read();
            peso = scale.read();
            pesoreal = (peso - 92600)/100;
            Serial.print(pesoreal);
            Serial.println(" gramos");
        }
        else {
          Serial.println("HX711 not found.");
          return;
        }
//----------------------WIFI-----------------------------------------------------
        if ((WiFiMulti.run() == WL_CONNECTED)) {
          std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
          client->setInsecure();
          HTTPClient https;
          Serial.print("[HTTPS] begin...\n");
          if (https.begin(*client, URL)) {
            Serial.print("[HTTPS] POST...\n");      
            sprintf(buf, "{\"humedad\": %.2f, \"temperatura\": %.2f, \"latitud\": %.6f, \"longitud\": %.6f, \"tiempoMedicion\": \"%sZ\", \"peso\": %.2f}", h, t, latitud, longitud, tiempoMedicion.c_str(), pesoreal);
            Serial.println(buf);
            https.addHeader("Content-Type", "application/json");
            int httpCode = https.POST(buf);
      
            if (httpCode > 0) {
              // HTTP header has been send and Server response header has been handled
              Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
      
              // file found at server
              if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                String payload = https.getString();
                Serial.println(payload);
              }
            } else {
              Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
            }
      
            https.end();
          } else {
            Serial.printf("[HTTPS] Unable to connect\n");
          }
        }
        smartDelay(5000);
      }
      else{
         Serial.print(F("INVALID"));
         return;
      }
    }
  }
}

////----------Neo6m--------------------------------------------------------------

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}
