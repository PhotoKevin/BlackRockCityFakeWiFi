#include <Arduino.h>
#include <time.h>

// #define USE_HTTPSS

#if defined (ESP32)
   #include <WiFi.h>
   #include <ESPAsyncWebSrv.h>

#elif defined (ESP8266)
   #include <ESP8266WiFi.h>
   #include <ESP8266mDNS.h>
   #include <ESPAsyncTCP.h>
   #include <ESPAsyncWebSrv.h>
#else
   #error Change your board type to an ESP32
#endif

// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html

#include "BMWifi.h"


//typedef int esp_err_t;


static char *getHeader (AsyncWebServerRequest *req, const char *headername, char *header, size_t headersize)
{
   header[0] = '\0';
   if (req->hasHeader (headername))
   {
      AsyncWebHeader *hdr = req->getHeader (headername);

      strncpy (header, hdr->value().c_str(), headersize);
      header[headersize-1] = '\0';
      strtrim (header);
   }

   return header;
}

static char *getUserAgent (AsyncWebServerRequest *req, char *agent, size_t agentsize)
{
   return getHeader (req, "User-Agent", agent, agentsize);
}


static char *getHost (AsyncWebServerRequest *req, char *hostname, size_t hostsize)
{
   return getHeader (req, "Host", hostname, hostsize);
}


static void setLastPageRequested (AsyncWebServerRequest *req)
{
   char host[60];
   snprintf (lastPageReq, sizeof lastPageReq, "http://%s%s", getHost (req, host, sizeof host), req->url ().c_str());
}

void strtrim (char *str)
{
   size_t               len;

   if (str != NULL)
   {
      len                  =  strlen (str);
      while ((len > 0) && isspace (str[len-1]))
      {
         str[len-1]        =  '\0';
         len               -= 1;
      }

      char *p              =  str;
      while (isspace (*p))
         p                 += 1;

      if (p != str)
         memmove (str, p, strlen(p)+1);
   }
}

void urldecode (char *tgt, const char *src)
{
   char *dst = tgt;   
   while (*src) 
   {
      if ((src[0] == '%') && isxdigit (src[1]) && isxdigit (src[2])) 
      {
         char a, b;
         a = src[1];
         b = src[2];

         if (isdigit (a))
            a -= '0';
         else
            a = tolower (a) - 'a' + 10;

         if (isdigit (b))
            b -= '0';
         else
            b = tolower (b) - 'a' + 10;

         *dst++ = a*0x10 + b;
         src += 3;
      } 
      else if (*src == '+') 
      {
         *dst++ = ' ';
         src++;
      }
      else
         *dst++ = *src++;
   }
   *dst++ = '\0';

   strtrim (tgt);
}


void setClock (const char *timestamp)
{
   if (timestamp != NULL)
   {
      int year, month, day;
      int hour, minute, second;

      Serial.printf ("setClock (\"%s\")\n", timestamp);
      int n = sscanf (timestamp, "%04d-0%02d-%02dT%02d:%02d:%02d", &year, &month, &day, &hour, &minute, &second);
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

static bool allowSend (AsyncWebServerRequest *req, String page)
{
   bool rc = false;

   bool banned = isBanned (clientAddress(req));

   if (! banned)
      rc = true;
   else if (page == "/blocked.hmtl")
      rc = true;
   else if (page == "/banned.hmtl")
      rc = true;
   else if (page == "/login.hmtl")
      rc = true;

   Serial.printf ("Checking allowed: %s --> %s\n",  page.c_str(), rc ? "OK" : "NOK");
   return rc;
}


static void sendHeaders (AsyncWebServerRequest *req)
{
   // httpd_resp_set_hdr (req, "Cache-Control", "no-cache, no-store, must-revalidate");
   // httpd_resp_set_hdr (req, "Pragma", "no-cache");
   // httpd_resp_set_hdr (req, "Expires", "-1");
}

static void sendHtml (AsyncWebServerRequest *req, const char *txt)
{
   setLastPageRequested (req);

   IPAddress clientIP = req->client()->remoteIP();
   bool allowed = allowSend (req, req->url());

   Serial.printf ("SendHtml %s -> %s (%s)\n", lastPageReq, clientIP.toString ().c_str (), allowed ? "allowed" : "not allowed");
   sendHeaders (req);

   if (allowed)
      req->send (200, "text/html", txt);
   else
      req->send (200, "text/html", banned_html);
}

static void sendJs (AsyncWebServerRequest *req, const char *txt)
{
   setLastPageRequested (req);

   Serial.printf ("SendJs %s\n", lastPageReq);

   sendHeaders (req);
   req->send (200, "application/javascript", txt);
}

static void sendCss (AsyncWebServerRequest *req, const char *txt)
{
   setLastPageRequested (req);
   Serial.printf ("SendCss %s\n", lastPageReq);

   sendHeaders (req);
   req->send (200, "text/css", txt);
}

static int  send302 (AsyncWebServerRequest *req, const char *page)
{
   char location[200];

   setLastPageRequested (req);
   snprintf (location, sizeof location, "http://%s/%s", localIP (), page);
   Serial.printf ("send302 -> %s\n", location);
// //   sendHeaders (req);

//    httpd_resp_set_hdr (req, "X-Frame-Options", "deny" );
//    httpd_resp_set_hdr (req, "Cache-Control", "no-cache" );
//    httpd_resp_set_hdr (req, "Pragma", "no-cache" );
//    httpd_resp_set_hdr (req, "Location", location);

//   req->send (307, "text/html", redirect_html);
   req->redirect (location);
   return 0;
}

static void  handleRestrictedPage (AsyncWebServerRequest *req, const char *requestedPage)
{
   Serial.printf ("handleRestrictedPage\n");
   if (isLoggedIn (req))
      sendHtml (req, requestedPage);
   else
   {
      send302 (req, "login.html");
   }
}

static void handleBanned (AsyncWebServerRequest *req)      {Serial.println ("handleBanned"); sendHtml (req, banned_html);}

static void handleRadioCSS (AsyncWebServerRequest *req)    {sendCss (req, radio2_css);}
static void handlebrccss (AsyncWebServerRequest *req)      {sendCss (req, brc_css);}
static void handleCheckboxCSS (AsyncWebServerRequest *req) {sendCss (req, checkbox_css);}

static void handlequestionsjs (AsyncWebServerRequest *req) {sendJs (req, questions_js);}
static void handleBannedJs (AsyncWebServerRequest *req)    {sendJs (req, banned_js);}
static void handleBMWifiJs (AsyncWebServerRequest *req)    {sendJs (req, bmwifi_js);}
static void handleDebugData (AsyncWebServerRequest *req)   {sendJs (req, debugdata_js);}
static void handleStatusJs (AsyncWebServerRequest *req)    {sendJs (req, status_js);}
static void handleSettingsJs (AsyncWebServerRequest *req)  {sendJs (req, settings_js);}
static void handleStatus (AsyncWebServerRequest *req)      {handleRestrictedPage (req, status_html);}
static void handleSettings (AsyncWebServerRequest *req)    {return handleRestrictedPage (req, settings_html);}

static void handleLegal (AsyncWebServerRequest *req)       
{
   Serial.println ("handleLegal");
   char agent[200];
	
   String userAgent = getUserAgent (req, agent, sizeof agent);
   uint64_t client = clientAddress (req);

   if (! isLoggedIn (req) && !hasVisited (client) && strcmp (req->url ().c_str (), "/legal.html") == 0)
   {
      if (userAgent.indexOf ("Android") >= 0)
         EEData.androidCount++;
      else if (userAgent.indexOf ("iPhone") >= 0)
         EEData.iPhoneCount++;
      
      EEData.legalShown += 1;
      visited (client);
   }

   if (clockSet)
      EEData.lastActivity = time (NULL);
   EEChanged = 1;
   sendHtml (req, legal_html);
}

static void getPostParameter (AsyncWebServerRequest *req, const char *name, char *value, size_t valsize)
{
   value[0] = '\0';
   AsyncWebParameter *parameter = req->getParam (name, true, false);
   if (parameter != nullptr)
      strncpy (value, parameter->value().c_str(), valsize);
   value[valsize-1] = '\0';
}

static void handleLogin (AsyncWebServerRequest *req)
{
   char username[50];
   char password[50];
   char timestamp[50];

   Serial.println ("handleLogin");

   getPostParameter (req, "username", username, sizeof username);
   getPostParameter (req, "password", password, sizeof password);
   getPostParameter (req, "timeStamp", timestamp, sizeof timestamp);

   urldecode (username, username);
   urldecode (password, password);
   urldecode (timestamp, timestamp);

   Serial.printf ("user: %s; pass:%s; time:%s", username, password, timestamp);

   bool loggedin = false;
   if (username[0] != '\0' && password[0] != '\0')
   {
      setClock (timestamp);
      loggedin = Login (username, password, clientAddress (req));
   }   

   if (loggedin)
   {
      send302 (req, "status.html");
   }
   else
      sendHtml (req, login_html);

   Serial.printf ("Returning OK from handlLogin\n");
}

static void handleQuestion (AsyncWebServerRequest *req)    
{
   char timestamp[50];
   timestamp[0] = '\0';

   Serial.printf ("handleQuestion\n");
   getPostParameter (req, "timeStamp", timestamp, sizeof timestamp);
   urldecode (timestamp, timestamp);
   setClock (timestamp);
   sendHtml (req, question_html);
   Serial.printf ("Leave handleQuestion");
}


static void handleJsonRequest (AsyncWebServerRequest *req) 
{
   Serial.println ("handleJsonRequest");

   char request[200] = "";
   char timestamp[200] = "";

   getPostParameter (req, "request", request, sizeof request);
   getPostParameter (req, "timeStamp", timestamp, sizeof timestamp);

   urldecode (request, request);
   urldecode (timestamp, timestamp);   
   if (!clockSet)
      setClock (timestamp);

   Serial.printf ("Ajax request of %s\n", request);

   if (strcmp (request, "question") == 0)
      sendJs (req, questions_json);

   else if (strcmp (request, "expire") == 0)
   {
      char json[44];
      uint64_t device = clientAddress (req);
      int expires = banExpires (device);

      snprintf (json, sizeof json, "{\"expire\" : \"%d\"}", expires);
      sendJs (req, json);
   }
   else if (strcmp (request, "status") == 0)
   {
      if (isLoggedIn (req))
         sendJs (req, getSystemInformation ().c_str());
   }
   else if (strcmp (request, "settings") == 0)
   {
      if (isLoggedIn (req))
         sendJs (req, getSettings (req).c_str());
   }
   else if (strcmp (request, "getTitle") == 0)
      sendJs (req, getTitle ().c_str());
   else if (strcmp (request, "resetCounts") == 0)
   {
      if (isLoggedIn (req))
      {
         EEData.totalBanned = 0;
         EEData.legalShown = 0;
         EEData.legalAccepted = 0;
         EEData.androidCount = 0;
         EEData.iPhoneCount = 0;
         EEData.lastActivity = time (NULL);
         EEChanged = 1;
      }
   }
   else if (strcmp (request, "") == 0)
      Serial.println ("Empty Ajax request");
   else
      Serial.printf ("Unknown AJAX request: %s\n", request);

   Serial.printf ("   Ajax request complete\n");
}


static void parseAddress (uint8_t *addressBytes, const char *address, int base)
{
   char s[40];

   strncpy (s, address, sizeof s);
   s[sizeof s - 1] = '\0';

   Serial.printf ("Parsing %s\n", s);

   int i = 0;
   char *p = s;
   while (p != NULL && strlen (p) > 0)
   {
//      Serial.printf ("... %s\n", p);
      addressBytes[i++] = (uint8_t) strtol (p, &p, base);
//      Serial.printf ("....%d\n", addressBytes[i-1]);
      if (*p != '\0')
         p++;
   }
}

void handleSettingsPost (AsyncWebServerRequest *req)
{
   char ssid[100];
   char username[50];
   char password[50];
   char hostname[100];
   char masterDevice[50];
   char ipAddress[50];
   char netmask[50];
   char playSound[10];

   Serial.println ("handleSettingsPost");
   getPostParameter (req, "ssid", ssid, sizeof ssid);
   getPostParameter (req, "username", username, sizeof username);
   getPostParameter (req, "password", password, sizeof password); 
   getPostParameter (req, "hostname", hostname, sizeof hostname);
   getPostParameter (req, "masterDevice", masterDevice, sizeof masterDevice);
   getPostParameter (req, "ipAddress", ipAddress, sizeof ipAddress);
   getPostParameter (req, "netmask", netmask, sizeof netmask);
   getPostParameter (req, "playSound", playSound, sizeof playSound);

   urldecode (ssid, ssid);
   urldecode (username, username);
   urldecode (password, password);
   urldecode (hostname, hostname);
   urldecode (masterDevice, masterDevice);
   urldecode (ipAddress, ipAddress);
   urldecode (netmask, netmask);

   if (isLoggedIn (req))
   {
      Serial.println ("  make changes");
      if (strlen (ssid) > 0)
         strncpy (EEData.SSID, ssid, sizeof EEData.SSID); 

      if (strlen (username) > 0)
         strncpy (EEData.username, username, sizeof EEData.username);

      if (strlen (password) > 0)
         strncpy (EEData.password, password, sizeof EEData.password);

      if (strlen (hostname) > 0)
         strncpy (EEData.hostname, hostname, sizeof EEData.hostname);

      if (strlen (masterDevice) > 0)
         parseAddress (EEData.masterDevice, masterDevice, 16);

      if (strlen (ipAddress) > 0)
         parseAddress (EEData.ipAddress, ipAddress, 10);

      if (strlen (netmask) > 0)
         parseAddress (EEData.netmask, netmask, 10);

      if (strlen (playSound) > 0)
         EEData.playSound = atoi (playSound);

      EEChanged = 1;
      RestartRequired = 1;
   }

   handleSettings (req);
}

void handleBlocked (AsyncWebServerRequest *req) 
{
   Serial.println ("handleBlocked");
   uint64_t device = clientAddress (req);
   sendHtml (req, blocked_html); 

   if (!isBanned (device) && !isLoggedIn (req))
   {
      if (clockSet)
         EEData.lastActivity = time (NULL);

      banDevice (device);
      Serial.printf ("Device %llx is Banned\n", device);

      EEData.totalBanned += 1;
      Serial.printf ("Banned: %d\n", EEData.totalBanned);
      EEChanged = 1;      
   }
}



void handlePortalCheck (AsyncWebServerRequest *req) 
{
   char host[59];   
   getHost (req, host, sizeof host);
   setLastPageRequested (req);

   Serial.printf ("   handlePortalCheck: %s\n", lastPageReq);
   if (isLoggedIn (req))
   {
      send302 (req, "status.html");
   }
   else
   {
      Serial.printf ("   Redirect %s to captive portal\n", lastPageReq);
      send302 (req, "legal.html");
   }
}

static void notFound (AsyncWebServerRequest *req)
{
   setLastPageRequested (req);
   char host[512];

   char ipAddress[50];
   snprintf (ipAddress, sizeof ipAddress, "%d.%d.%d.%d", EEData.ipAddress[0], EEData.ipAddress[1], EEData.ipAddress[2], EEData.ipAddress[3]);

   if ((strcmp (host, EEData.hostname) == 0) || (strcmp (host, ipAddress) == 0))
   {
      Serial.printf ("   Send 404\n");
      req->send(404, "text/plain", "Not found");
   }
   else
      handlePortalCheck (req);
}

extern AsyncWebServer server;
extern AsyncEventSource events;

void setupWebServer (void)
{
   Serial.println ("Server(s) started, registering handlers");

   server.onNotFound (notFound);

   server.on ("/bmwifi.js", HTTP_GET, handleBMWifiJs);
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
   //server_on ("/favicon.ico", HTTP_GET, notFound);
   server.on ("/status", HTTP_GET, handleStatus);
   server.on ("/status.html", HTTP_GET, handleStatus);
   server.on ("/status.js", HTTP_GET, handleStatusJs);

   server.on ("/settings.html", HTTP_GET, handleSettings);
   server.on ("/settings.html", HTTP_POST, handleSettingsPost);
   server.on ("/settings.js", HTTP_GET, handleSettingsJs);

   server.on ("/login.html", HTTP_GET, handleLogin);
   server.on ("/login.html", HTTP_POST, handleLogin);

   server.begin ();
   Serial.printf ("Webserver setup complete\n");
}
