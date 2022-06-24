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

   const int capacity = JSON_OBJECT_SIZE(32);
   StaticJsonDocument<capacity> root;

   char buffer[32];
   time_t now;
   now = EEData.lastActivity;

   root["eepromDataSize"]  = EEData.eepromDataSize;
   root["totalBanned"]     = EEData.totalBanned;
   root["legalShown"]      = EEData.legalShown;
   root["legalAccepted"]   = EEData.legalAccepted;

   strftime (buffer, sizeof buffer, "%FT%T", gmtime (&now));
   root["lastActivity"]    = buffer;
   root["androidCount"]    = EEData.androidCount;
   root["iPhoneCount"]     = EEData.iPhoneCount;
   root["SSID"]            = EEData.SSID;
   root["hostname"]        = EEData.hostname;
//   root["username"]        = EEData.username;
//   root["password"]        = EEData.password;

   snprintf (buffer, sizeof buffer, "%02x:%02x:%02x:%02x:%02x:%02x", EEData.masterDevice[0], EEData.masterDevice[1], EEData.masterDevice[2], EEData.masterDevice[3], EEData.masterDevice[4], EEData.masterDevice[5]);
   root["masterDevice"]    = buffer;

   snprintf (buffer, sizeof buffer, "%d.%d.%d.%d", EEData.ipAddress[0], EEData.ipAddress[1], EEData.ipAddress[2], EEData.ipAddress[3]);
   root["ipAddress"]       = buffer;
   snprintf (buffer, sizeof buffer, "%d.%d.%d.%d", EEData.netmask[0], EEData.netmask[1], EEData.netmask[2], EEData.netmask[3]);
   root["netmask"]         = buffer;

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
   serializeJson (root, json);

   return json;
}


String getSettings (void)
{
   String json = "";
   StaticJsonDocument<1024> jsonBuffer;
   //DynamicJsonDocument jsonBuffer(1280);
 //  JsonObject root = jsonBuffer.as<JsonObject>();

   const int capacity = JSON_OBJECT_SIZE(32);
   StaticJsonDocument<capacity> root;

   char buffer[32];

   root["ssid"]            = EEData.SSID;
   root["hostname"]        = EEData.hostname;
   root["username"]        = EEData.username;
   //root["password"]        = EEData.password;

   snprintf (buffer, sizeof buffer, "%02x:%02x:%02x:%02x:%02x:%02x", EEData.masterDevice[0], EEData.masterDevice[1], EEData.masterDevice[2], EEData.masterDevice[3], EEData.masterDevice[4], EEData.masterDevice[5]);
   root["masterDevice"]    = buffer;

   snprintf (buffer, sizeof buffer, "%d.%d.%d.%d", EEData.ipAddress[0], EEData.ipAddress[1], EEData.ipAddress[2], EEData.ipAddress[3]);
   root["ipAddress"]       = buffer;
   snprintf (buffer, sizeof buffer, "%d.%d.%d.%d", EEData.netmask[0], EEData.netmask[1], EEData.netmask[2], EEData.netmask[3]);
   root["netmask"]         = buffer;

   long long currentDeviceMac = clientAddress ();
   uint8_t *bytes = (uint8_t *) &currentDeviceMac;
   snprintf (buffer, sizeof buffer, "%02x:%02x:%02x:%02x:%02x:%02x", bytes[5], bytes[4], bytes[3], bytes[2], bytes[1], bytes[0]);
   root["currentDevice"] = buffer;

   root["softApMac"] = WiFi.softAPmacAddress();
   root["softApIP"] = WiFi.softAPIP().toString ();
   
   root["station_mac"] = WiFi.macAddress();
   root["station_ip"] = WiFi.localIP().toString();
   serializeJson ( root, json);

   return json;
}
