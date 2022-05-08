/**
   BasicHTTPClient.ino

    Created on: 24.05.2015

*/

#include <Arduino.h>
#include <ArduinoJson.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

#include <U8g2lib.h>
#include "weather_constants.h"

ESP8266WiFiMulti WiFiMulti;
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ 16, /* clock=*/ 5, /* data=*/ 4);

u8g2_uint_t offset;     // current offset for the scrolling text
u8g2_uint_t width;      // pixel width of the scrolling text (must be lesser than 128 unless U8G2_16BIT is defined

char buf[30];
char outstr[4];
unsigned long int weatherMillis = 0;
unsigned long int displayMillis = 0;
String weatherurl = String("http://api.openweathermap.org/data/2.5/weather?zip=") + myZIP + String(",") + myCOUNTRY + String("&APPID=") + myopenweatherapi + String("&units=metric");

DynamicJsonDocument docOut(4096);

DynamicJsonDocument fetch_weather() {
  DynamicJsonDocument doc(4096);
  WiFiClient client;
  HTTPClient http;
  int httpCode;
  http.useHTTP10(true);
  http.begin(client, weatherurl);  // HTTP
  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  httpCode = http.GET();
  
  // httpCode will be negative on error
  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);
  
    // file found at server
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
      //char* payload = http.getString();
      //Serial.println(payload);
      deserializeJson(doc, http.getStream());
    }
  }
  http.end();
  return doc;
}


void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);
  u8g2.begin();

  u8g2.setFont(u8g2_font_logisoso24_tf); // set the target font to calculate the pixel width
  //width = u8g2.getUTF8Width(text);    // calculate the pixel width of the text

  u8g2.setFontMode(0);    // enable transparent mode, which is faster

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(mySSID, myPASSWORD);

  do {
    delay(1000);
  } while (WiFiMulti.run() != WL_CONNECTED);

  docOut = fetch_weather();
}

void loop() {
  // wait for WiFi connection
  const char* weather_0_main;
  float main_temp;
  unsigned long int time_diff;
  
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    time_diff = millis() - weatherMillis;
    //Serial.println(time_diff);
    if ( time_diff > 600000) { //fetch new weather info every 10mins
      weatherMillis = millis();
      Serial.print("[HTTP] begin...\n");
      docOut = fetch_weather();
    }

    if (millis() - displayMillis > 6*1000) {

      JsonObject weather_0 = docOut["weather"][0];
      int weather_0_id = weather_0["id"]; // 501
      weather_0_main = weather_0["main"]; // "Rain"
      const char* weather_0_description = weather_0["description"]; // "moderate rain"
      Serial.println(weather_0_description);

      JsonObject main = docOut["main"];
      main_temp = main["temp"]; // 5.43
      dtostrf(main_temp, 2, 0, outstr);
      //int main_feels_like = main["feels_like"]; // 1
      Serial.println(main_temp);
      
      displayMillis = millis();
      const char* spaces = " ";
      const char* cel = "C";
      strcpy(buf,weather_0_main);
      //strcat(buf,spaces);
      //strcat(buf,outstr);
      //strcat(buf,cel);

      u8g2.clear();
      u8g2.drawStr(0,30,buf);  // write something to the internal memory
      u8g2.sendBuffer();          // transfer internal memory to the display

      delay(3*1000);

      strcpy(buf,outstr);
      //strcat(buf,spaces);
      //strcat(buf,outstr);
      strcat(buf,cel);

      u8g2.clear();
      u8g2.drawStr(0,30,buf);  // write something to the internal memory
      u8g2.sendBuffer();          // transfer internal memory to the display
    }
  } else {
    Serial.println("Waiting for WiFi...");
    delay(3000);
  }
}
