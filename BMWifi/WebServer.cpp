#include <Arduino.h>
#include <time.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Note: https://github.com/esp8266/Arduino/commit/9e82bf7c8db6274afa297262e1e16e685b1d56f9

#include "BMWifi.h"
long long clientAddress ();

static String getAuthToken ()
{
   if (server.hasHeader("Cookie")) 
   {
      String cookie = server.header ("Cookie");
      Serial.printf ("Found cookie: '%s'\n", cookie.c_str());
      // !Found cookie: title=BRC Open WiFi; auth=18
      //cookie.split ();    
      return cookie;  
  }

  return String ("");
}

static void setClock (void)
{
   if (server.hasArg ("timeStamp"))
   {
      String request = server.arg ("timeStamp");
      if (request != NULL)
      {
//         Serial.printf ("TimeStamp: %s\n", request.c_str ());
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
}

static bool allowSend (String page)
{
   bool rc = false;

   bool banned = isBanned (clientAddress());
   yield ();

   if (! banned)
      rc = true;
   else if (page == "/blocked.hmtl")
      rc = true;
   else if (page == "/banned.hmtl")
      rc = true;
   else if (page == "/login.hmtl")
      rc = true;

//   Serial.printf ("Checking allowed: %s(%s) --> %s\n",  page.c_str(), type, rc ? "OK" : "NOK");
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

static void sendHtml (const char *txt)
{
   IPAddress clientIP = server.client().remoteIP();
   bool allowed = allowSend (server.uri ());
   Serial.printf ("Send %s%s -> %s (%s)\n", server.hostHeader ().c_str (), server.uri ().c_str(), clientIP.toString ().c_str (), allowed ? "allowed" : "not allowed");
   sendHeaders ();
   
   if (allowed)
      server.send (200, "text/html", txt);
   else
      server.send (200, "text/html", banned_html);

   yield ();
}

static void sendJs (const char *txt)
{
   Serial.printf ("SendJs %s%s\n", server.hostHeader ().c_str (), server.uri ().c_str());
   sendHeaders ();
   
   server.send (200, "application/javascript", txt);

   yield ();
}

static void sendCss (const char *txt)
{
   Serial.printf ("SendCss %s%s\n", server.hostHeader ().c_str (), server.uri ().c_str());
   sendHeaders ();
   
   server.send (200, "text/css", txt);

   yield ();
}


static void handleRestrictedPage (const char *requestedPage)
{
   if (isMasterDevice () || isLoggedIn (clientAddress (), getAuthToken ()))
      sendHtml (requestedPage);
   else
   {
      sendHeaders ();

      server.sendHeader ("Location", String ("http://") + server.client().localIP().toString() + "/login.html", true);
      server.send (302, "text/html", "");
   }
}


static void handleBanned (void)      {sendHtml (banned_html);}
static void handleRadioCSS (void)    {sendCss (radio2_css);}
static void handlebrccss (void)      {sendCss (brc_css);}
static void handleCheckboxCSS (void) {sendCss (checkbox_css);}

static void handlequestionsjs (void) {sendJs (questions_js);}
static void handleBannedJs (void)    {sendJs (banned_js);}
static void handleBMWifiJs (void)    {sendJs (bmwifi_js);}
static void handleDebugData (void)   {sendJs (debugdata_js);}
static void handleStatusJs (void)    {sendJs (status_js);}
static void handleSettingsJs (void)  {sendJs (settings_js);}
static void handleStatus (void)      {handleRestrictedPage (status_html);}
static void handleSettings (void)    {handleRestrictedPage (settings_html);}

static void handleLegal (void)       
{
   String userAgent = server.header("User-Agent");

   if (! isMasterDevice ())
   {
      if (userAgent.indexOf ("Android") >= 0)
         EEData.androidCount++;
      else if (userAgent.indexOf ("iPhone") >= 0)
         EEData.iPhoneCount++;
      
      EEData.legalShown += 1;
   }

   if (clockSet)
      EEData.lastActivity = time (NULL);
   EEChanged = 1;
   sendHtml (legal_html);
}

static void handleLogin (void)
{
   Serial.println ("handleLogin");   
   String username = server.arg ("username");
   String password = server.arg ("password");
   String cookie;
   bool loggedin = false;
   if (username != NULL && password != NULL)
   {
      setClock ();
      loggedin = Login (username, password, clientAddress (), cookie);
   }   

   if (loggedin)
   {
      char title[200];
      snprintf (title, sizeof title, "auth=%s", cookie.c_str ());
      server.sendHeader ("Set-Cookie", title);
      sendHeaders ();

      server.sendHeader ("Location", String ("http://") + server.client().localIP().toString() + "/status.html", true);
      server.send (302, "text/html", "");
   }
   else
      sendHtml (login_html);
}

static void handleQuestion (void)    
{
   setClock ();
   sendHtml (question_html);
}


static void handleJsonRequest (void) 
{
   if (!server.hasArg ("request"))
      Serial.println ("Empty Ajax request");
   else
   {
      String request = server.arg ("request");
      Serial.printf ("Ajax request of %s\n", request.c_str());
      if (request == "question")
         sendJs (questions_json);
      else if (request == "expire")
      {
         char json[44];
         long long device = clientAddress ();
         int expires = banExpires (device);

         snprintf (json, sizeof json, "{\"expire\" : \"%d\"}", expires);
         sendJs (json);
      }
      else if (request == "status")
      {
         if (!clockSet)
            setClock ();

         if (isMasterDevice () || isLoggedIn (clientAddress (), getAuthToken ()))
            sendJs (getSystemInformation ().c_str());
      }
      else if (request == "settings")
      {
         if (isMasterDevice () || isLoggedIn (clientAddress (), getAuthToken ()))
            sendJs (getSettings ().c_str());
      }
      else if (request == "resetCounts")
      {
         if (isMasterDevice () || isLoggedIn (clientAddress (), getAuthToken ()))
         {
            EEData.totalBanned = 0;
            EEData.legalShown = 0;
            EEData.legalAccepted = 0;
            EEData.androidCount = 0;
            EEData.iPhoneCount = 0;
            EEData.lastActivity = time (NULL);
         }
      }
      else
         Serial.printf ("Unknown AJAX request: %s\n", request.c_str());

      Serial.printf ("   Ajax request complete");
   }
     
}


void parseAddress (uint8_t *addressBytes, const char *address, int base)
{
   char s[40];

   strncpy (s, address, sizeof s);
   s[sizeof s - 1] = '\0';

   Serial.printf ("Parsing %s\n", s);

   int i = 0;
   char *p = s;
   while (p != NULL && strlen (p) > 0)
   {
      Serial.printf ("... %s\n", p);
      addressBytes[i++] = (uint8_t) strtol (p, &p, base);
      Serial.printf ("....%d\n", addressBytes[i-1]);
      if (*p != '\0')
         p++;
   }
}

void handleSettingsPost (void)
{
   Serial.println ("handleSettings");
   if (isMasterDevice () || isLoggedIn (clientAddress (), getAuthToken ()))
   {
      Serial.println ("  make changes");
      if (server.hasArg ("ssid"))
         strncpy (EEData.SSID, server.arg ("ssid").c_str(), sizeof EEData.SSID); 

      if (server.hasArg ("username"))
      {
         String u = server.arg ("username");
         u.trim ();
         strncpy (EEData.username, u.c_str(), sizeof EEData.username);
      }

      if (server.hasArg ("password") and server.arg ("passowrd").length () > 0)
      {
         String p = server.arg ("password");
         p.trim ();
         strncpy (EEData.password, p.c_str(), sizeof EEData.password);
      }

      if (server.hasArg ("hostname"))
         strncpy (EEData.hostname, server.arg ("hostname").c_str(), sizeof EEData.hostname);

      if (server.hasArg ("masterDevice"))
         parseAddress (EEData.masterDevice, server.arg ("masterDevice").c_str(), 16);

      if (server.hasArg ("ipAddress"))
         parseAddress (EEData.ipAddress, server.arg ("ipAddress").c_str(), 10);

      if (server.hasArg ("netmask"))
         parseAddress (EEData.netmask, server.arg ("netmask").c_str(), 10);

      if (server.hasArg ("playSound"))
         EEData.playSound = atoi (server.arg ("playSound").c_str ());

      EEChanged = 1;

//         ESP.restart ();
//         while (1);

   }

   handleSettings ();
}

void handleBlocked (void) 
{
   long long device = clientAddress ();
   sendHtml (blocked_html); 

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
   Serial.println ("");
   // delay (10); // Without the delay, the printf will crash. I think it's because hostHeader is returning a garbage value.
   Serial.printf ("handlePortalCheck: %s%s\n", server.hostHeader ().c_str(), server.uri ().c_str());
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

      server.sendHeader ("Location", String ("http://") + server.client().localIP().toString() + "/legal.html", true);
      String content = redirect_html;
      content.replace ("login.example.com", server.client().localIP().toString());
content = "";
      server.send (302, "text/html", content);
   }
}

static void notFound (void)          
{
   Serial.printf ("Not found: %s%s\n", server.hostHeader ().c_str (), server.uri ().c_str());
   if (server.hostHeader() == server.client().localIP().toString())
      server.send (404, "text/html", "");
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
   server.on ("/getJson", HTTP_POST,  handleJsonRequest);
   server.on ("/questions.js", HTTP_GET,  handlequestionsjs);
   server.on ("/radio2.css", HTTP_GET,  handleRadioCSS);
   server.on ("/favicon.ico", HTTP_GET, notFound);
   server.on ("/status", HTTP_GET, handleStatus);
   server.on ("/status.html", HTTP_GET, handleStatus);
   server.on ("/status.js", HTTP_GET, handleStatusJs);
   server.on ("/bmwifi.js", HTTP_GET, handleBMWifiJs);

   server.on ("/settings.html", HTTP_GET, handleSettings);
   server.on ("/settings.html", HTTP_POST, handleSettingsPost);
   server.on ("/settings.js", HTTP_GET, handleSettingsJs);

   server.on ("/login.html", HTTP_GET, handleLogin);
   server.on ("/login.html", HTTP_POST, handleLogin);


   // server.on("/generate_204", handlePortalCheck);  //Android captive portal check.
   // server.on("/fwlink", handlePortalCheck);  //Microsoft captive portal check. 
   // server.on("/connecttest.txt", handlePortalCheck);  // Windows 10 captive portal check.
   // server.on("/redirect", handlePortalCheck);  // Windows 10 captive portal check.

   server.collectHeaders ("Request-Time", "User-Agent", "Cookie", "");

   server.begin ();
   yield ();

   Serial.printf ("Listening at: %s\n", ip.toString ().c_str());
}
