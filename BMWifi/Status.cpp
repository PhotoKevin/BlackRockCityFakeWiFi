#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Note: https://github.com/esp8266/Arduino/commit/9e82bf7c8db6274afa297262e1e16e685b1d56f9

#include "BMWifi.h"

String getSystemInformation (void)
{
   String json = "";
   StaticJsonDocument<1024> jsonBuffer;
   //DynamicJsonDocument jsonBuffer(1280);
 //  JsonObject root = jsonBuffer.as<JsonObject>();

   const int capacity = JSON_OBJECT_SIZE(19);
   StaticJsonDocument<capacity> root;

   char buffer[32];
   time_t now;
   now = time (NULL);
   strftime (buffer, sizeof buffer, "%FT%T", gmtime (&now));


   root["banned"] = EEData.totalBanned;
   root["redirects"] = EEData.totalRedirects;
   root["lastActivity"] = buffer;
   root["sdkVersion"] = ESP.getSdkVersion();
   root["bootVersion"] = ESP.getBootVersion();
   root["bootMode"] = ESP.getBootMode();
   root["chipID"] = ESP.getChipId();
   root["cpuFreq"] = ESP.getCpuFreqMHz();
      
   root["voltage"] = ESP.getVcc();
   
   root["memoryFree"] = ESP.getFreeHeap();
   root["sketchSize"] = ESP.getSketchSize();
   root["sketchFree"] = ESP.getFreeSketchSpace();
   
   root["flashRealSize"] = ESP.getFlashChipRealSize();
   root["flashSize"] = ESP.getFlashChipSize();
//   root["flashSpeed"] = ( ESP.getFlashChipSpeed() / 1000000 );
   
   root["softApMac"] = WiFi.softAPmacAddress();
   root["softApIP"] = WiFi.softAPIP().toString ();
   
   root["station_mac"] = WiFi.macAddress();
   root["station_ip"] = WiFi.localIP().toString();
   serializeJson ( root, json);

   Serial.println (json);   
   return json;
}
