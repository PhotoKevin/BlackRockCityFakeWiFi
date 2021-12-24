#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Note: https://github.com/esp8266/Arduino/commit/9e82bf7c8db6274afa297262e1e16e685b1d56f9

#include "BMWifi.h"
long long clientAddress ();

static bool allowSend (const char *type, String page)
{
   bool rc = false;
   Serial.printf ("Checking allowed: %s(%s)",  page.c_str(), type);

   Serial.printf (" isBanned? -> %d\n", isBanned (clientAddress()));

   if (NULL == strstr (type, "html"))
      rc = true;
   else if (!isBanned (clientAddress ()))
      rc = true;
   else if (server.uri() == "/blocked.hmtl")
      rc = true;
   else if (server.uri() == "/banned.hmtl")
      rc = true;

   Serial.printf (" --> %s\n", rc ? "OK" : "NOK");
   return rc;
}

static void send (const char *type, const char *txt)
{
   IPAddress clientIP = server.client().remoteIP();
   Serial.print (server.uri ());
   Serial.printf ("(%s) -> %s\n", type, clientIP.toString ().c_str ());
   
   server.setContentLength (CONTENT_LENGTH_UNKNOWN);
   server.sendHeader ("Cache-Control", "no-store");
   server.send (200, type, "");
   if (allowSend (type, server.uri ()))
   {
      server.sendContent (txt);
   }
   else
   {
      server.sendContent (banned_html);
   }
   yield ();
   SaveEEDataIfNeeded (EEDataAddr, &EEData, sizeof EEData);
}

// https://www.esp8266.com/viewtopic.php?f=8&t=4307


#include <U8x8lib.h>

long long mac2ll (uint8 *mac)
{
   long long address = 0;

   for (int i=0; i<6; i++)
   {
      address <<= 8;
      address += mac[i];
   }

   return address;
}

long long ip2ll (IPAddress ip)
{
   long long address = 0;

   for (int i=0; i<4; i++)
   {
      address <<= 8;
      address += ip[i];
   }

   return address;   
}

long long clientAddress ()
{
   long long address = 0;
   WiFiClient cli = server.client ();
   IPAddress clientIP = server.client().remoteIP();
   
   auto client_count = wifi_softap_get_station_num();
   auto i = 1;
   struct station_info *station_list = wifi_softap_get_station_info();
   while (station_list != NULL) 
   {
      IPAddress station = IPAddress ((&station_list->ip)->addr);
      if (clientIP == station)
      {
         address = mac2ll (station_list->bssid);
         
         auto station_ip = station.toString().c_str();
         char station_mac[18] = {0};
         sprintf(station_mac, "%02X:%02X:%02X:%02X:%02X:%02X", MAC2STR(station_list->bssid));         
         Serial.printf("%d. %s %s", i++, station_ip, station_mac);
         station_list = STAILQ_NEXT(station_list, next);
      }   
   }
   wifi_softap_free_station_info();

   if (address == 0)
      address = ip2ll (clientIP);
      
   return address;
}

static void handleBanned (void)      {send ("text/html", banned_html);}
static void handleLegal (void)       
{
   EEData.totalRedirects += 1;
   EEChanged = 1;
   send ("text/html", legal_html);
}
static void handleQuestion (void)    {send ("text/html", question_html);}

static void handleRadioCSS (void)    {send ("text/css", radio2_css);}
static void handlebrccss (void)      {send ("text/css", brc_css);}
static void handleCheckboxCSS (void) {send ("text/css", checkbox_css);}

static void handlequestionsjs (void) {send ("application/javascript", questions_js);}
static void handleBannedJs (void)    {send ("application/javascript", banned_js);}
static void handleDebugData (void)   {send ("application/javascript", debugdata_js);}

static void notFound (void)          
{
   Serial.printf ("Not found: %s\n", server.uri ());
   server.send (404, "text/html", "");
}

static void handleQuestionJson (void) 
{
   if (!server.hasArg ("request"))
      Serial.println ("Bad request");
   else
   {
      String request = server.arg ("request");
      if (request == "question")
         send ("application/javascript", questions_json);
      else if (request = "expire")
      {
         char json[44];
         long long device = clientAddress ();
         int expires = banExpires (device);

         snprintf (json, sizeof json, "{\"expire\" : \"%d\"}", expires);
         send ("application/javascript", json);
      }
      else
         Serial.printf ("Unknown request: %s\n", request);
   }
}


void handleBlocked (void) 
{
   long long device = clientAddress ();
   send ("text/html", blocked_html); 

   banDevice (device);
   if (isBanned (device))
      Serial.printf ("Device %llx is Banned\n", device);
}

   
// https://techtutorialsx.com/2018/07/22/esp32-arduino-http-server-template-processing/

// https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html

extern U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8;

static void XXredirectPage (void)
{
   EEData.totalRedirects += 1;
   server.setContentLength (CONTENT_LENGTH_UNKNOWN);
   server.sendHeader ("Cache-Control", "no-store");
   server.sendHeader ("Location", "/legal.html");
   server.send (301);

   Serial.printf ("Redirects %d\n", EEData.totalRedirects);
}

void setupWebServer (void)
{
   IPAddress ip = WiFi.localIP ();
   
   if (ip[0] == 0)
      ip = WiFi.softAPIP ();

   char correctURL[33];
   sprintf (correctURL, "http://%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

  // Set up the endpoints for HTTP server
//  server.on ("/donation", HTTP_POST, handlePage);
//  server.on ("/donation", HTTP_GET,  handlePage);
   server.onNotFound (notFound);

   server.on ("/", HTTP_GET, handleLegal);
   server.on ("/checkbox.css", HTTP_GET,  handleCheckboxCSS);
   server.on ("/question.html", HTTP_GET,  handleQuestion);
   server.on ("/question.html", HTTP_POST,  handleQuestion);
   server.on ("/legal.html", HTTP_GET,  handleLegal);
   server.on ("/banned.html", HTTP_GET,  handleBanned);
   server.on ("/banned.js", HTTP_GET,  handleBannedJs);
   server.on ("/blocked.html", HTTP_GET,  handleBlocked);
   server.on ("/debugdata.js", HTTP_GET,  handleDebugData);
   server.on ("/brc.css", HTTP_GET,  handlebrccss);
   server.on ("/getJson", HTTP_POST,  handleQuestionJson);
   server.on ("/questions.js", HTTP_GET,  handlequestionsjs);
   server.on ("/radio2.css", HTTP_GET,  handleRadioCSS);
   server.on ("/favicon.ico", HTTP_GET, notFound);
   
   server.begin ();
   yield ();

   Serial.printf ("Listening at: %s\n", correctURL);
}
