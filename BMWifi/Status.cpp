#include <Arduino.h>
#include <ArduinoJson.h>
#include <time.h>


#if defined (ESP32)
   #include "WiFi.h"
   #include <ESPAsyncWebSrv.h>
   #include <esp_system.h>
   #include "esp_chip_info.h"

#elif defined (ESP8266)
   #include <ESP8266WiFi.h>
   #include <ESPAsyncWebSrv.h>
#else
   #error Change your board type to an ESP32
#endif


// Note: https://github.com/esp8266/Arduino/commit/9e82bf7c8db6274afa297262e1e16e685b1d56f9

#include "BMWifi.h"


#if defined (ESP32)
String getChipModel (esp_chip_model_t chip)
{
   switch (chip)
   {
   case CHIP_ESP32: return "ESP32";
   case CHIP_ESP32S2: return "ESP32S2";
   case CHIP_ESP32S3: return "ESP32S3";
   case CHIP_ESP32C3: return "ESP32C3";
   case CHIP_ESP32C2: return "ESP32C2";
   case CHIP_ESP32C6: return "ESP32C6";
   case CHIP_ESP32H2: return "ESP32H2";
   case CHIP_POSIX_LINUX: return "Linus";
   default: return "Unknown chip";
   }
}
#endif


String getTitle (void)
{

   String json = "";

   JsonDocument root;

   root["title"]            = EEData.SSID;
   serializeJson (root, json);
   Serial.println (json);

   return json;
}

String getSystemInformation (void)
{
   String json = "";

   JsonDocument root;

   char buffer[32];
   time_t now;
   now = EEData.lastActivity;

   root["eepromDataSize"]  = EEData.eepromDataSize;
   root["totalBanned"]     = EEData.totalBanned;
   root["legalShown"]      = EEData.legalShown;
   root["legalAccepted"]   = EEData.legalAccepted;

   strftime (buffer, sizeof buffer, "%F %T", gmtime (&now));
   root["lastActivity"]    = buffer;
   root["androidCount"]    = EEData.androidCount;
   root["iPhoneCount"]     = EEData.iPhoneCount;
   root["unknownCount"]    = EEData.unknownCount;
   root["SSID"]            = EEData.SSID;
   root["hostname"]        = EEData.hostname;

   snprintf (buffer, sizeof buffer, "%02x:%02x:%02x:%02x:%02x:%02x", EEData.masterDevice[0], EEData.masterDevice[1], EEData.masterDevice[2], EEData.masterDevice[3], EEData.masterDevice[4], EEData.masterDevice[5]);
   root["masterDevice"]    = buffer;

   snprintf (buffer, sizeof buffer, "%d.%d.%d.%d", EEData.ipAddress[0], EEData.ipAddress[1], EEData.ipAddress[2], EEData.ipAddress[3]);
   root["ipAddress"]       = buffer;
   snprintf (buffer, sizeof buffer, "%d.%d.%d.%d", EEData.netmask[0], EEData.netmask[1], EEData.netmask[2], EEData.netmask[3]);
   root["netmask"]         = buffer;

   root["sdkVersion"] = ESP.getSdkVersion();
//   root["bootVersion"] = ESP.getBootVersion();
//   root["bootMode"] = ESP.getBootMode();
//   root["chipID"] = ESP.getChipModel();
   root["cpuFreq"] = ESP.getCpuFreqMHz();

//   root["voltage"] = ESP.getVcc();
   
   root["memoryFree"] = ESP.getFreeHeap();
   root["sketchSize"] = ESP.getSketchSize();
   root["sketchFree"] = ESP.getFreeSketchSpace();
   
//   root["flashRealSize"] = ESP.getFlashChipRealSize();
   root["flashSize"] = ESP.getFlashChipSize();
//   root["flashSpeed"] = ( ESP.getFlashChipSpeed() / 1000000 );
   
   root["softApMac"] = WiFi.softAPmacAddress();
   root["softApIP"] = WiFi.softAPIP().toString ();
   
   root["station_mac"] = WiFi.macAddress();
   root["station_ip"] = WiFi.localIP().toString();

   root["lastUnknownUserAgent"] = String (EEData.lastUnknownUserAgent);

   #if defined (ESP32)
      esp_chip_info_t chip;
      esp_chip_info (&chip);
      char s[22];
      snprintf (s, sizeof s, "%d.%02d", chip.revision/100, chip.revision%100);
      root["chip"] = getChipModel (chip.model);
      root["revision"] =  s;
      root["cores"] = chip.cores;
   #endif

   serializeJson (root, json);
   return json;
}

String getSettings (AsyncWebServerRequest *req)
{
   String json = "";

   JsonDocument root;

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

   uint64_t currentDeviceMac = clientAddress (req);
   uint8_t *bytes = (uint8_t *) &currentDeviceMac;
   snprintf (buffer, sizeof buffer, "%02x:%02x:%02x:%02x:%02x:%02x", bytes[5], bytes[4], bytes[3], bytes[2], bytes[1], bytes[0]);
   root["currentDevice"] = buffer;

   root["softApMac"] = WiFi.softAPmacAddress();
   root["softApIP"] = WiFi.softAPIP().toString ();
   
   root["station_mac"] = WiFi.macAddress();
   root["station_ip"] = WiFi.localIP().toString();
   serializeJson (root, json);

   return json;
}
