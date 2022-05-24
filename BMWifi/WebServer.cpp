#include <Arduino.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Note: https://github.com/esp8266/Arduino/commit/9e82bf7c8db6274afa297262e1e16e685b1d56f9

#include "BMWifi.h"
long long clientAddress ();

static bool allowSend (const char *type, String page)
{
   bool rc = false;
   Serial.printf ("Checking allowed: %s(%s)\n",  page.c_str(), type);

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
   server.sendHeader ("Cache-Control", "no-cache, no-store, must-revalidate");
   server.sendHeader ("Pragma", "no-cache");
   server.sendHeader ("Expires", "-1");

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

long long clientAddress (void)
{
   long long address = 0;
   WiFiClient cli = server.client ();
   IPAddress clientIP = server.client().remoteIP();
   
   uint8_t client_count = wifi_softap_get_station_num();
//   int i = 1;
   struct station_info *station_list = wifi_softap_get_station_info();
   while (station_list != NULL) 
   {
      IPAddress station = IPAddress ((&station_list->ip)->addr);
      if (clientIP == station)
      {
         address = mac2ll (station_list->bssid);
         
         String station_ip = station.toString();
//         char station_mac[18] = {0};
//         sprintf(station_mac, "%02X:%02X:%02X:%02X:%02X:%02X", MAC2STR(station_list->bssid));         
//         Serial.printf ("%d. IP: %s MAC: %s\n", i++, station_ip.c_str(), station_mac);
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
static void handleQuestion (void)    
{
   if (server.hasArg ("timeStamp"))
   {
      String request = server.arg ("timeStamp");
      if (request != NULL)
      {
         Serial.printf ("TimeStamp: %s\n", request.c_str ());
         int year, month, day;
         int hour, minute, second;

         int n = sscanf (request.c_str(), "%04d-0%02d-%02dT%02d:%02d:%02d", &year, &month, &day, &hour, &minute, &second);
         if (n == 6)
         {
            struct tm newtime;
            memset (&newtime, 0, sizeof newtime);
            newtime.tm_year = year - 1900;
            newtime.tm_mon = month - 1;
            newtime.tm_mday = day;
            newtime.tm_hour = hour;
            newtime.tm_min = minute;
            newtime.tm_sec = second;
            newtime.tm_isdst = -1;
            time_t nowtime = mktime (&newtime);
            Serial.printf ("good: %d:%d:%d\n", hour, minute, second);
            struct timeval tv;
            memset (&tv, 0, sizeof tv);
            tv.tv_sec = nowtime;
            settimeofday (&tv, NULL);
            EEData.lastActivity = nowtime;

           // set_system_time (2000);
         }
      }
   }
   send ("text/html", question_html);
}

static void handleRadioCSS (void)    {send ("text/css", radio2_css);}
static void handlebrccss (void)      {send ("text/css", brc_css);}
static void handleCheckboxCSS (void) {send ("text/css", checkbox_css);}

static void handlequestionsjs (void) {send ("application/javascript", questions_js);}
static void handleBannedJs (void)    {send ("application/javascript", banned_js);}
static void handleDebugData (void)   {send ("application/javascript", debugdata_js);}
static void handleStatusJs (void)    {send ("text/javascript", status_js);}
static void handleStatus (void)      {send ("text/html", status_html);}


static void notFound (void)          
{
   Serial.printf ("Not found: %s\n", server.uri ().c_str());
   server.send (404, "text/html", "");
}

static void handleQuestionJson (void) 
{
   Serial.println ("handle Ajax request");
   if (!server.hasArg ("request"))
      Serial.println ("Bad request");
   else
   {
      String request = server.arg ("request");
      Serial.printf ("  Ajax reques of %s\n", request.c_str());
      if (request == "question")
         send ("application/javascript", questions_json);
      else if (request == "expire")
      {
         char json[44];
         long long device = clientAddress ();
         int expires = banExpires (device);

         snprintf (json, sizeof json, "{\"expire\" : \"%d\"}", expires);
         send ("application/javascript", json);
      }
      else if (request == "status")
      {
         Serial.println ("call getSysInfo");
         send ("application/javascript", getSystemInformation ().c_str());
      }
      else
         Serial.printf ("Unknown request: %s\n", request.c_str());
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


// https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html

void handlePortalCheck () 
{
   Serial.printf ("handlePortalCheck: %s%s\n", server.hostHeader ().c_str(), server.uri ().c_str());

   Serial.println ("Request redirected to captive portal");
   server.sendHeader ("Cache-Control", "no-cache, no-store, must-revalidate");
   server.sendHeader ("Pragma", "no-cache");
   server.sendHeader ("Expires", "-1");

   server.sendHeader ("Location", String("http://") + server.client().localIP().toString(), true);
   String content = redirect_html;
   content.replace ("login.example.com", server.client().localIP().toString());
   Serial.println ("send content");
   Serial.println (content);
   server.send (302, "text/html", content);
   Serial.println ("content sent");
}


void setupWebServer (void)
{
   IPAddress ip = WiFi.localIP ();
   
   if (ip[0] == 0)
      ip = WiFi.softAPIP ();

   char correctURL[33];
   sprintf (correctURL, "http://%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

  // Set up the endpoints for HTTP server
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
   server.on ("/status", HTTP_GET, handleStatus);
   server.on ("/status.js", HTTP_GET, handleStatusJs);

   server.on("/generate_204", handlePortalCheck);  //Android captive portal check.
   server.on("/fwlink", handlePortalCheck);  //Microsoft captive portal check. 

   server.collectHeaders ("Request-Time", "");

   server.begin ();
   yield ();

   Serial.printf ("Listening at: %s\n", ip.toString ().c_str());
}
