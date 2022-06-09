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

   bool banned = isBanned (clientAddress());
   yield ();
   Serial.printf (" isBanned? -> %d\n", banned);

   if (NULL == strstr (type, "html"))
      rc = true;
   else if (! banned)
      rc = true;
   else if (server.uri() == "/blocked.hmtl")
      rc = true;
   else if (server.uri() == "/banned.hmtl")
      rc = true;

   Serial.printf (" --> %s\n", rc ? "OK" : "NOK");
   return rc;
}

static void sendHeaders (void)
{
   server.sendHeader ("Cache-Control", "no-cache, no-store, must-revalidate");
   server.sendHeader ("Pragma", "no-cache");
   server.sendHeader ("Expires", "-1");

   char title[200];
   snprintf (title, sizeof title, "title=%s", EEData.SSID);
   server.sendHeader ("Set-Cookie", title);
}

static void send (const char *type, const char *txt)
{
   IPAddress clientIP = server.client().remoteIP();
   Serial.printf ("%s%s ", server.hostHeader ().c_str (), server.uri ().c_str());
   Serial.printf ("(%s) -> %s\n", type, clientIP.toString ().c_str ());
   sendHeaders ();
//   server.setContentLength (CONTENT_LENGTH_UNKNOWN);
   
   if (allowSend (type, server.uri ()))
      server.send (200, type, txt);
   else
      server.send (200, type, banned_html);

   yield ();
//   server.stop ();
}



static void handleBanned (void)      {send ("text/html", banned_html);}
static void handleRadioCSS (void)    {send ("text/css", radio2_css);}
static void handlebrccss (void)      {send ("text/css", brc_css);}
static void handleCheckboxCSS (void) {send ("text/css", checkbox_css);}

static void handlequestionsjs (void) {send ("application/javascript", questions_js);}
static void handleBannedJs (void)    {send ("application/javascript", banned_js);}
static void handleBMWifiJs (void)    {send ("application/javascript", bmwifi_js);}
static void handleDebugData (void)   {send ("application/javascript", debugdata_js);}
static void handleStatusJs (void)    {send ("text/javascript", status_js);}
static void handleStatus (void)      {send ("text/html", status_html);}

static void handleLegal (void)       
{
   String userAgent = server.header("User-Agent");

   if (! isMasterDevice ())
   {
      if (userAgent.indexOf ("Android") >= 0)
         EEData.androidCount++;
      else if (userAgent.indexOf ("iPhone") >= 0)
         EEData.iPhoneCount++;
      
      EEData.totalRedirects += 1;
   }

   if (clockSet)
      EEData.lastActivity = time (NULL);
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
            clockSet = true;
            EEChanged = 1;
         }
      }
   }

   send ("text/html", question_html);
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

   if (!isMasterDevice ())
      EEData.totalBanned += 1;
      
   if (clockSet)
      EEData.lastActivity = time (NULL);
   EEChanged = 1;

   banDevice (device);
   if (isBanned (device))
      Serial.printf ("Device %llx is Banned\n", device);
}


// https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html

void handlePortalCheck () 
{
   // delay (10); // Without the delay, the printf will crash. I think it's because hostHeader is returning a garbage value.
   // Serial.printf ("handlePortalCheck: %s%s\n", server.hostHeader ().c_str(), server.uri ().c_str());
   Serial.println ("");
   if (isMasterDevice ())
   {
      sendHeaders ();

      server.sendHeader ("Location", String ("http://") + server.client().localIP().toString() + "/status", true);
      server.send (302, "text/html", "");
   }
   else
   {
      Serial.println ("Request redirected to captive portal");
      sendHeaders ();

      server.sendHeader ("Location", String ("http://") + server.client().localIP().toString(), true);
      String content = redirect_html;
      content.replace ("login.example.com", server.client().localIP().toString());
      // Serial.println ("send content");
      // Serial.println (content);
      server.send (302, "text/html", content);
      Serial.println ("content sent");
   }
}

static void notFound (void)          
{
   Serial.printf ("Not found: %s%s\n", server.hostHeader ().c_str (), server.uri ().c_str());
   if (server.hostHeader() == server.client().localIP().toString())
   {
      Serial.println ("Send 404");
      server.send (404, "text/html", "");
   }
   else
      handlePortalCheck ();
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
   server.on ("/bmwifi.js", HTTP_GET, handleBMWifiJs);

   server.on("/generate_204", handlePortalCheck);  //Android captive portal check.
   server.on("/fwlink", handlePortalCheck);  //Microsoft captive portal check. 
   server.on("/connecttest.txt", handlePortalCheck);  // Windows 10 captive portal check.
   server.on("/redirect", handlePortalCheck);  // Windows 10 captive portal check.

   server.collectHeaders ("Request-Time", "User-Agent", "");

   server.begin ();
   yield ();

   Serial.printf ("Listening at: %s\n", ip.toString ().c_str());
}
