#include <Arduino.h>
#include <time.h>

// #define USE_HTTPSS

#if defined (ESP32)
   #include <WiFi.h>
   #include <lwip/sockets.h>

   #include <esp_http_server.h>

#else
   #error Change your board type to an ESP32
#endif

// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html

#include "BMWifi.h"

static char *getHeader (httpd_req_t *req, const char *headername, char *header, size_t headersize)
{
   header[0] = '\0';
   size_t buf_len = httpd_req_get_hdr_value_len (req, headername) + 1;
   if (buf_len > 1) 
   {
      char *buf = (char *) malloc (buf_len);
      if (buf != NULL)
      {
         if (httpd_req_get_hdr_value_str (req, headername, buf, buf_len) == ESP_OK) 
         {
            strncpy (header, buf, headersize);
            header[headersize-1] = '\0';
         }
         free (buf);
      }
   }

   strtrim (header);
   return header;
}

static char *getUserAgent (httpd_req_t *req, char *agent, size_t agentsize)
{
   return getHeader (req, "User-Agent", agent, agentsize);
}


static char *getHost (httpd_req_t *req, char *hostname, size_t hostsize)
{
   char *rc = getHeader (req, "Host", hostname, hostsize);
   size_t len = strlen (hostname);

   return rc;
}


static char *getArgString (httpd_req_t *req)
{
   char *buf = NULL;

   int rc;
   if (req->content_len > 1)
   {
      Serial.printf ("getArgString from content: %d bytes\n", req->content_len);
      buf = (char *) malloc (req->content_len+1);
      if (buf != NULL)
      {
         rc = httpd_req_recv (req, buf, req->content_len);
         Serial.printf ("httpd_req_recv -> %d\n", rc);         
         if (rc > 0)
            buf[rc] = '\0';
      }
   }
   else
   {
      size_t buf_len = httpd_req_get_url_query_len(req);
      Serial.printf ("getArgString from url: %d bytes\n", buf_len);

      if (buf_len > 1) 
      {
         buf = (char *) malloc(buf_len+1);
         if (buf != NULL)
         {
            if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
               buf[buf_len] = '\0';
         }
      }
   }

   if (buf != NULL)
      Serial.printf ("Arg String: %s\n", buf);
   return buf;
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

static bool allowSend (httpd_req_t *req, String page)
{
   bool rc = false;

   bool banned = isBanned (clientAddress(req));
   yield ();

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


static void sendHeaders (httpd_req_t *req)
{
   // httpd_resp_set_hdr (req, "Cache-Control", "no-cache, no-store, must-revalidate");
   // httpd_resp_set_hdr (req, "Pragma", "no-cache");
   // httpd_resp_set_hdr (req, "Expires", "-1");
}

static void sendHtml (httpd_req_t *req, const char *txt)
{
   char host[512]; 
   snprintf (lastPageReq, sizeof lastPageReq, "http://%s%s", getHost (req, host, sizeof host), req->uri);

   IPAddress clientIP = IPAddress (); // server.client().remoteIP();
   bool allowed = allowSend (req, req->uri);

   Serial.printf ("SendHtml %s -> %s (%s)\n", lastPageReq, clientIP.toString ().c_str (), allowed ? "allowed" : "not allowed");
   sendHeaders (req);
   httpd_resp_set_type (req, "text/html");

   if (allowed)
      httpd_resp_sendstr (req, txt);
   else
      httpd_resp_sendstr (req, banned_html);
}

static void sendJs (httpd_req_t *req, const char *txt)
{
   char host[512];
   snprintf (lastPageReq, sizeof lastPageReq, "http://%s%s", getHost (req, host, sizeof host), req->uri);
   Serial.printf ("SendJs %s\n", lastPageReq);

   sendHeaders (req);
   httpd_resp_set_type (req, "application/javascript");
   httpd_resp_send (req , txt, strlen (txt));

   yield ();
}

static void sendCss (httpd_req_t *req, const char *txt)
{
   char host[200];
   snprintf (lastPageReq, sizeof lastPageReq, "http://%s%s", getHost (req, host, sizeof host), req->uri);
   Serial.printf ("SendCss %s\n", lastPageReq);

   sendHeaders (req);
   httpd_resp_set_type (req, "text/css");
   httpd_resp_sendstr (req, txt);

   yield ();
}

static int  send302 (httpd_req_t *req, const char *page)
{
   char location[1024];
   char host[60];
   snprintf (lastPageReq, sizeof lastPageReq, "http://%s%s", getHost (req, host, sizeof host), req->uri);

   snprintf (location, sizeof location, "http://%s/%s", localIP (), page);

   Serial.printf ("send302 -> %s\n", location);
//   sendHeaders (req);

   httpd_resp_set_hdr (req, "X-Frame-Options", "deny" );
   httpd_resp_set_hdr (req, "Cache-Control", "no-cache" );
   httpd_resp_set_hdr (req, "Pragma", "no-cache" );
   httpd_resp_set_hdr (req, "Location", location);
   httpd_resp_set_status (req, "307");
   httpd_resp_set_type (req, "text/html");
   httpd_resp_sendstr (req, redirect_html);
   Serial.println ("302 sent");

   return ESP_OK;
}

static esp_err_t  handleRestrictedPage (httpd_req_t *req, const char *requestedPage)
{
   Serial.printf ("handleRestrictedPage\n");
   if (isLoggedIn (req))
      sendHtml (req, requestedPage);
   else
   {
      return send302 (req, "login.html");
   }
   
   return ESP_OK;
}


static esp_err_t handleBanned (httpd_req_t *req)      {sendHtml (req, banned_html); return ESP_OK;}
static esp_err_t handleRadioCSS (httpd_req_t *req)    {sendCss (req, radio2_css); return ESP_OK;}
static esp_err_t handlebrccss (httpd_req_t *req)      {sendCss (req, brc_css); return ESP_OK;}
static esp_err_t handleCheckboxCSS (httpd_req_t *req) {sendCss (req, checkbox_css); return ESP_OK;}

static esp_err_t handlequestionsjs (httpd_req_t *req) {sendJs (req, questions_js); return ESP_OK;}
static esp_err_t handleBannedJs (httpd_req_t *req)    {sendJs (req, banned_js); return ESP_OK;}
static esp_err_t handleBMWifiJs (httpd_req_t *req)    {sendJs (req, bmwifi_js); return ESP_OK;}
static esp_err_t handleDebugData (httpd_req_t *req)   {sendJs (req, debugdata_js); return ESP_OK;}
static esp_err_t handleStatusJs (httpd_req_t *req)    {sendJs (req, status_js); return ESP_OK;}
static esp_err_t handleSettingsJs (httpd_req_t *req)  {sendJs (req, settings_js); return ESP_OK;}
static esp_err_t handleStatus (httpd_req_t *req)      {handleRestrictedPage (req, status_html); return ESP_OK;}
static esp_err_t handleSettings (httpd_req_t *req)    {return handleRestrictedPage (req, settings_html);}

static esp_err_t handleLegal (httpd_req_t *req)       
{
   Serial.println ("handleLegal");
   char agent[200];

   String userAgent = getUserAgent (req, agent, sizeof agent);
   Serial.printf ("userAgent is %s\n", agent);

   yield ();
   if (! isLoggedIn (req))
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
   sendHtml (req, legal_html);

   return ESP_OK;
}

static esp_err_t handleLogin (httpd_req_t *req)
{
   char *argumentBuffer;
   char username[50];
   char password[50];
   char timestamp[50];

   username[0] = '\0';
   password[0] = '\0';
   timestamp[0] = '\0';


   Serial.println ("handleLogin");
   argumentBuffer = getArgString (req);
   if (argumentBuffer != NULL)
   {
      Serial.printf ("getting args %s\n", argumentBuffer);
      if (httpd_query_key_value (argumentBuffer, "username", username, sizeof username) != ESP_OK)
         username[0] = '\0';

      if (httpd_query_key_value (argumentBuffer, "password", password, sizeof password) != ESP_OK)
         password[0] = '\0';

      if (httpd_query_key_value (argumentBuffer, "timestamp", timestamp, sizeof timestamp) != ESP_OK)
         timestamp[0] = '\0';
      free (argumentBuffer);

      urldecode (username, username);
      urldecode (password, password);
      urldecode (timestamp, timestamp);

      Serial.printf ("user: %s; pass:%s; time:%s", username, password, timestamp);
   }

   bool loggedin = false;
   if (username[0] != '\0' && password[0] != '\0')
   {
      setClock (timestamp);
      loggedin = Login (username, password, clientAddress (req));
   }   

   if (loggedin)
   {
      return send302 (req, "status.html");
   }
   else
      sendHtml (req, login_html);

   Serial.printf ("Returning OK from handlLogin\n");
   return ESP_OK;
}

static esp_err_t handleQuestion (httpd_req_t *req)    
{
   char *argumentBuffer;
   char timestamp[50];
   timestamp[0] = '\0';

   Serial.printf ("handleQuestion\n");
   argumentBuffer = getArgString (req);
   if (argumentBuffer != NULL)
   {
      if (httpd_query_key_value (argumentBuffer, "timestamp", timestamp, sizeof timestamp) != ESP_OK)
         timestamp[0] = '\0';
      free (argumentBuffer);

      urldecode (timestamp, timestamp);
   }

   setClock (timestamp);
   sendHtml (req, question_html);
   Serial.printf ("Leave handleQuestion");
   return ESP_OK;
}


static esp_err_t handleJsonRequest (httpd_req_t *req) 
{
   Serial.println ("handleJsonRequest");

   char *argumentBuffer;
   char request[200];
   char timestamp[200];
   argumentBuffer = getArgString (req);
   if (argumentBuffer != NULL)
   {
      if (httpd_query_key_value (argumentBuffer, "request", request, sizeof request) != ESP_OK)
         request[0] = '\0';

      if (httpd_query_key_value (argumentBuffer, "timestamp", timestamp, sizeof timestamp) != ESP_OK)
         timestamp[0] = '\0';

      free (argumentBuffer);
   }
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
      long long device = clientAddress (req);
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
      }
   }
   else if (strcmp (request, "") == 0)
      Serial.println ("Empty Ajax request");
   else
      Serial.printf ("Unknown AJAX request: %s\n", request);

   Serial.printf ("   Ajax request complete\n");
   
   return ESP_OK;
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

esp_err_t handleSettingsPost (httpd_req_t *req)
{
   char *argumentBuffer;
   char ssid[100];
   char username[50];
   char password[50];
   char hostname[100];
   char masterDevice[50];
   char ipAddress[50];
   char netmask[50];
   char playSound[10];

   argumentBuffer = getArgString (req);
   if (argumentBuffer != NULL)
   {
      Serial.printf ("AgrBuf: %s\n", argumentBuffer);

      if (httpd_query_key_value (argumentBuffer, "ssid", ssid, sizeof ssid) != ESP_OK)
         ssid[0] = '\0';

      if (httpd_query_key_value (argumentBuffer, "username", username, sizeof username) != ESP_OK)
         username[0] = '\0';

      if (httpd_query_key_value (argumentBuffer, "password", password, sizeof password) != ESP_OK)
         password[0] = '\0';

      if (httpd_query_key_value (argumentBuffer, "hostname", hostname, sizeof hostname) != ESP_OK)
         hostname[0] = '\0';
      if (httpd_query_key_value (argumentBuffer, "masterDevice", masterDevice, sizeof masterDevice) != ESP_OK)
         masterDevice[0] = '\0';
      if (httpd_query_key_value (argumentBuffer, "ipAddress", ipAddress, sizeof ipAddress) != ESP_OK)
         ipAddress[0] = '\0';
      if (httpd_query_key_value (argumentBuffer, "netmask", netmask, sizeof netmask) != ESP_OK)
         netmask[0] = '\0';
      if (httpd_query_key_value (argumentBuffer, "playSound", playSound, sizeof playSound) != ESP_OK)
         playSound[0] = '\0';

      free (argumentBuffer);
   }

   urldecode (ssid, ssid);
   urldecode (username, username);
   urldecode (password, password);
   urldecode (hostname, hostname);
   urldecode (masterDevice, masterDevice);
   urldecode (ipAddress, ipAddress);
   urldecode (netmask, netmask);

   Serial.println ("handleSettingsPost");
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

   return handleSettings (req);
}

static esp_err_t handleBlocked (httpd_req_t *req) 
{
   long long device = clientAddress (req);
   sendHtml (req, blocked_html); 

   if (!isLoggedIn (req))
      EEData.totalBanned += 1;
      
   if (clockSet)
      EEData.lastActivity = time (NULL);
   EEChanged = 1;

   banDevice (device);
   if (isBanned (device))
      Serial.printf ("Device %llx is Banned\n", device);

   return ESP_OK;
}



esp_err_t handlePortalCheck (httpd_req_t *req) 
{
   char host[59];   
   getHost (req, host, sizeof host);

   Serial.printf ("   handlePortalCheck: http://%s%s\n", host, req->uri);
   if (isLoggedIn (req))
   {
      return send302 (req, "status.html");
   }
   else
   {
      Serial.printf ("   Redirect http://%s%s to captive portal\n", host, req->uri);
      return send302 (req, "legal.html");
   }
}

static esp_err_t notFound (httpd_req_t *req, httpd_err_code_t error_code)
{
   char host[512];

   getHost (req, host, sizeof host);
   Serial.printf ("Not found: http://%s%s\n", host, req->uri);

   char ipAddress[50];
   snprintf (ipAddress, sizeof ipAddress, "%d.%d.%d.%d", EEData.ipAddress[0], EEData.ipAddress[1], EEData.ipAddress[2], EEData.ipAddress[3]);

   if ((strcmp (host, EEData.hostname) == 0) || (strcmp (host, ipAddress) == 0))
   {
      Serial.printf ("   Send 404\n");
      httpd_resp_send_404 (req);
   }
   else
      handlePortalCheck (req);

   return ESP_OK;
}

void server_on (const char *uri, httpd_method_t method, esp_err_t (*handler)(httpd_req_t *r))
{
   httpd_uri_t index_uri = 
   {
      .uri = uri,
      .method = method,
      .handler = handler,
      .user_ctx = NULL
   };

   if (insecure_http != NULL)
   {
      int rc = httpd_register_uri_handler (insecure_http, &index_uri);
      if (rc != 0)
         Serial.printf ("insecure httpd_register_uri_handler FAILED: %d\n", rc);
   }
}


void setupWebServer (void)
{
   httpd_config insecure_config;
   memset (&insecure_config, 0, sizeof insecure_config);


   insecure_config = HTTPD_DEFAULT_CONFIG ();
   insecure_config.max_uri_handlers   = 25;
   insecure_config.ctrl_port          = 32769;
   int rc = ESP_OK + 1;

   rc = httpd_start (&insecure_http, &insecure_config);
   if (rc != ESP_OK)
      Serial.printf ("Error %d starting insecure_http\n", rc);


   if (rc == ESP_OK)
   {
      Serial.println ("Server(s) started, registering handlers");

      if (insecure_http != NULL)
         httpd_register_err_handler (insecure_http, HTTPD_404_NOT_FOUND, notFound);

      for (int i=0; i<HTTPD_ERR_CODE_MAX; i++)
         httpd_register_err_handler (insecure_http, (httpd_err_code_t) i, notFound);

      server_on ("/bmwifi.js", HTTP_GET, handleBMWifiJs);
      server_on ("/", HTTP_GET, handleLegal);
      server_on ("/checkbox.css", HTTP_GET,  handleCheckboxCSS);
      server_on ("/question.html", HTTP_GET,  handleQuestion);
      server_on ("/question.html", HTTP_POST,  handleQuestion);
      server_on ("/legal.html", HTTP_GET,  handleLegal);
      server_on ("/banned.html", HTTP_GET,  handleBanned);
      server_on ("/banned.js", HTTP_GET,  handleBannedJs);
      server_on ("/blocked.html", HTTP_GET,  handleBlocked);
      server_on ("/debugdata.js", HTTP_GET,  handleDebugData);
      server_on ("/brc.css", HTTP_GET,  handlebrccss);
      server_on ("/getJson", HTTP_POST,  handleJsonRequest);
      server_on ("/questions.js", HTTP_GET,  handlequestionsjs);
      server_on ("/radio2.css", HTTP_GET,  handleRadioCSS);
      //server_on ("/favicon.ico", HTTP_GET, notFound);
      server_on ("/status", HTTP_GET, handleStatus);
      server_on ("/status.html", HTTP_GET, handleStatus);
      server_on ("/status.js", HTTP_GET, handleStatusJs);

      server_on ("/settings.html", HTTP_GET, handleSettings);
      server_on ("/settings.html", HTTP_POST, handleSettingsPost);
      server_on ("/settings.js", HTTP_GET, handleSettingsJs);

      server_on ("/login.html", HTTP_GET, handleLogin);
      server_on ("/login.html", HTTP_POST, handleLogin);
   }
   yield ();

   Serial.printf ("Webserver setup complete\n");
}
