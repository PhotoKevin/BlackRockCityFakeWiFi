#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Note: https://github.com/esp8266/Arduino/commit/9e82bf7c8db6274afa297262e1e16e685b1d56f9

#include "BMWifi.h"
long long clientAddress ();
static int isBanned (long long address);

static void send (const char *type, const char *txt)
{
   Serial.println (server.uri());
   server.setContentLength (CONTENT_LENGTH_UNKNOWN);
   server.send (200, type, "");
   if (!isBanned (clientAddress ()) || (NULL == strstr (type, "html")))
      server.sendContent (txt);
   else
      server.sendContent (blocked_html);
   yield ();
   
}

// https://www.esp8266.com/viewtopic.php?f=8&t=4307


#include <U8x8lib.h>

long long mac2ll (uint8 *mac)
{
   long long address = 0;

   for (int i=0; i<6; i++)
   {
      address << 8;
      address += mac[i];
   }

   return address;
}

long long ip2ll (IPAddress ip)
{
   long long address = 0;

   for (int i=0; i<4; i++)
   {
      Serial.printf ("  ... %llx", address);
      address << 8;
      address += ip[i];
   }

   return address;   
}

long long clientAddress ()
{
   long long address = 0;
   WiFiClient cli = server.client ();
   IPAddress clientIP = server.client().remoteIP();
   Serial.printf ("Client %s\n", clientIP.toString().c_str());

   auto client_count = wifi_softap_get_station_num();
   auto i = 1;
   struct station_info *station_list = wifi_softap_get_station_info();
   while (station_list != NULL) 
   {
      IPAddress station = IPAddress ((&station_list->ip)->addr);
      Serial.printf ("  Possible: %s\n", station.toString().c_str());
      if (clientIP == station)
      {
         Serial.println ("  Match");
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
   Serial.printf (" --> %llx\n", address);
      
   return address;
}


static void handlebrccss (void){   send ("text/css", brc_css);}
static void handleCheckboxCSS (void) {   send ("text/css", checkbox_css);}
static void handleQuestionJson (void) {    send ("application/javascript", questions_json); }
static void handleQuestion (void) {    send ("text/html", question_html);}
static void notFound (void) {  server.send (404, "text/html", "");}
static void handleLegal (void) {    send ("text/html", legal_html);}
static void handlequestionsjs (void) {   send ("application/javascript", questions_js);}
static void handleRadioCSS (void) {   send ("text/css", radio2_css);}
static void handleDebugData (void) {    send ("application/javascript", debugdata_js);}

static void expireBanned ()
{
   time_t currentTime = time (NULL);
   for (int i=0; i<NUM_BANNED; i++)
   {
      if (EEData.banned[i].timestamp != (time_t) 0)
      {
         float secs = difftime (currentTime, EEData.banned[i].timestamp);
         if (secs > 20)
         {
            Serial.printf ("Removing %llx\n", EEData.banned[i].address);
            EEData.banned[i].timestamp = 0;
         }
      }
   }
}

static void banDevice (long long address)
{
   Serial.printf ("Banning %llx\n", address);
   time_t currentTime = time (NULL);
   for (int i=0; i<NUM_BANNED; i++)
   {
      if (EEData.banned[i].timestamp == (time_t) 0)
      {
         EEData.banned[i].address = address;
         EEData.banned[i].timestamp = currentTime;
         EEChanged = 1;
         break;
      }
   }
   /* If there were no empty slots, they get a free pass */
}

static int isBanned (long long address)
{
   expireBanned ();
   for (int i=0; i<NUM_BANNED; i++)
   {
      if (EEData.banned[i].address == address)
         return 1;
   }

   return 0;   
}

static void handleBlocked (void) 
{
   long long device = clientAddress ();
   banDevice (device);
   send ("text/html", blocked_html); 

   if (isBanned (device))
      Serial.printf ("Banned\n");
}


   
// https://techtutorialsx.com/2018/07/22/esp32-arduino-http-server-template-processing/

// https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html

extern U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8;

static void redirectPage (void)
{
   client_status ();
   Serial.print ("Redirecting from ");
   Serial.println (server.uri());

   WiFiClient cli = server.client ();
   IPAddress clientIP = server.client().remoteIP();
   Serial.print ("Client IP: ");
   Serial.println (clientIP.toString ());

   char ip[30];
   clientIP.toString().toCharArray (ip, 16);
   u8x8.drawString (0, 1, ip);
//   u8x8.refreshDisplay();    // only required for SSD1606/7  

   //         auto station_ip = IPAddress((&station_list->ip)->addr).toString().c_str();
   
  server.setContentLength (CONTENT_LENGTH_UNKNOWN);
  server.sendHeader ("Location", "/legal.html");
  server.send (301);
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
   server.onNotFound (redirectPage);
   
   server.on ("/checkbox.css", HTTP_GET,  handleCheckboxCSS);
   server.on ("/question.html", HTTP_GET,  handleQuestion);
   server.on ("/question.html", HTTP_POST,  handleQuestion);
   server.on ("/legal.html", HTTP_GET,  handleLegal);
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
