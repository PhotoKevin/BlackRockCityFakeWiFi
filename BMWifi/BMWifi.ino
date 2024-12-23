//#define NOT_AP    // Define for debugging as just a device on the network

//#define IDEA_SPARK // Define for IdeaSpark boards. Needed since they don't have their own board type.

#if defined (ARDUINO_wifi_kit_8) || defined (IDEA_SPARK) || defined (ARDUINO_HELTEC_WIFI_KIT_32_V3) || defined (ARDUINO_HELTEC_WIFI_KIT_32)
   #define USE_LCD_DISPLAY
   // #define USE_U8G2 // Define if you want to use the G2 version of the libraries. Pointless really. 
#endif

#include <DNSServer.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Ticker.h>
#if defined (ESP32)
   #include <esp_wifi.h>
   #include <WiFi.h>
   #include <ESPmDNS.h>
   #include <ESPAsyncWebServer.h>
#elif defined (ESP8266)

   #include "ets_sys.h"
   #include <ESP8266WiFi.h>
   #include <ESP8266mDNS.h>
   #include <ESPAsyncTCP.h>
   #include <ESPAsyncWebServer.h>
#else
   #error Change your board type to an ESP32
#endif

char lastPageReq[512];

#include "Config.h"

// https://github.com/olikraus/u8g2
#if defined (USE_LCD_DISPLAY)
   #if defined (USE_U8G2)
      #include <U8g2lib.h>
   #else
      #include <U8x8lib.h>
   #endif

   #if defined (ARDUINO_wifi_kit_8)
      // Heltec WiFi Kit 8
      #if defined (USE_U8G2)
         U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8x8(U8G2_R0, /* reset=*/ 16, U8X8_PIN_NONE, U8X8_PIN_NONE);
      #else
         U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(/* reset=*/ 16);
      #endif

   #elif defined (ARDUINO_HELTEC_WIFI_KIT_32_V3)
      U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 18, /* data=*/ 17, /* reset=*/ 21);

   #elif defined (ARDUINO_HELTEC_WIFI_KIT_32)
      U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

   #elif defined (IDEA_SPARK)
      #if defined (U8G2LIB_HH)
         U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8x8 (U8G2_R0, 22, 21, U8X8_PIN_NONE);
         #define drawString drawStr
      #else
         U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8 (22, 21, U8X8_PIN_NONE);
      #endif
   #endif

#endif

//https://android.googlesource.com/platform/frameworks/base/+/c80f952/core/java/android/net/CaptivePortalTracker.java
#include "BMWifi.h"

// You need to supply your own Secret.h defining 
//#define NETWORK_SSID "<SSID>"      
//#define NETWORK_PASS "<password>"  
#if defined (NOT_AP)
   #include "Secret.h"
#endif

bool clockSet = false;

IPAddress apIP(10,47,4,7);
IPAddress netMsk(255,255,255,0);

AsyncWebServer server(80);
AsyncEventSource events("/events");

const byte DNS_PORT = 53;

DNSServer dnsServer;

struct eeprom_data_t  EEData;
int EEChanged = 0;
int RestartRequired = 0;

#if defined (NOT_AP)
   void  ConnectToNetwork (void);
#else
   void  SetupAP (void);
#endif

//ADC_MODE(ADC_VCC);      // Needed to make the ESP.getVCC function work.

//#include "heltec.h"
void setup (void)
{
   RestartRequired = 0;
//   Serial.begin (9600);                           // full speed to monitor
   Serial.begin (115200);                           // full speed to monitor
   delay(2000);
   Serial.println ("\n\n\n");
   Serial.println (ARDUINO_BOARD);

   memset (lastPageReq, 0, sizeof lastPageReq);
   #if defined (USE_LCD_DISPLAY)
      Serial.println ("Setting up LCD");
      u8x8.begin();
      u8x8.setPowerSave (0);
      #if defined (U8G2LIB_HH)
         u8x8.setFont (u8g2_font_chroma48medium8_8r);
      #else
         u8x8.setFont (u8x8_font_chroma48medium8_r);
      #endif
   #endif
   Serial.print ("\nBMWifi Starting\n");

   EEPROM.begin (sizeof EEData); 
   ReadEEData (EEDataAddr, &EEData, sizeof EEData);
   if (EEData.eepromDataSize != sizeof EEData)
   {
      Serial.println ("Setting default EEData values");
      uint8_t *macaddr = getMacAddress ();
      uint8_t lastMacOctet = macaddr[5];
      if (lastMacOctet == 0)
         lastMacOctet = 1;

      uint8_t ip[] = DEFAULT_IPADDRESS;
      uint8_t mask[] = DEFAULT_NETMASK;
      uint8_t master[] = DEFAULT_MASTER;
      memset (&EEData, 0, sizeof EEData);
      EEData.eepromDataSize = sizeof EEData;
      str_copy (EEData.username, DEFAULT_ADMIN, sizeof EEData.username);
      str_copy (EEData.password, DEFAULT_PASSWORD, sizeof EEData.password);
      EEData.displayTimeoutSeconds = 90;

      snprintf (EEData.SSID, sizeof EEData.SSID, "%s %d", DEFAULT_SSID, lastMacOctet);
      snprintf (EEData.hostname, sizeof EEData.hostname, "%s %d", DEFAULT_HOSTNAME, lastMacOctet);
      ip[3] = lastMacOctet % 0x7f;

      for (int i=0; i<4; i++)
      {
         EEData.ipAddress[i] = ip[i];
         EEData.netmask[i] = mask[i];
      }

      for (int i=0; i<6; i++)
         EEData.masterDevice[i] = master[i];
      EEChanged = 1;
   }

   #if defined (NOT_AP)
      ConnectToNetwork ();
   #else
      SetupAP ();
   #endif

   Serial.print ("SDK Version: ");
   Serial.println (ESP.getSdkVersion());
   Serial.println (getSystemInformation ());

#if !defined (NOT_AP)

   // Setup MDNS responder
   Serial.println (EEData.hostname);
   if (!MDNS.begin (EEData.hostname)) 
      Serial.println("Error setting up MDNS responder!");
   else 
   {
      Serial.printf ("mDNS responder started: %s\n", EEData.hostname);
      // Add service to MDNS-SD
      MDNS.addService ("http", "tcp", 80);
//      MDNS.addService ("https", "tcp", 443);
   }
#endif

   setupWebServer ();

}


/// Setup the ESP32 as an Access Point

void SetupAP (void)
{
//   WiFi.disconnect (true, true);
   WiFi.mode (WIFI_AP);

   apIP = IPAddress (EEData.ipAddress);
//   netMsk = IPAddress (EEData.netmask);

   WiFi.softAPConfig (apIP, apIP, netMsk);
   WiFi.softAP (EEData.SSID); // No password, this is an open access point

   Serial.print ("AP is ");
   Serial.println (WiFi.softAPIP ().toString());

   // Set up a DNS server. 
   dnsServer.setErrorReplyCode (DNSReplyCode::NoError);

   dnsServer.setTTL(0);
   dnsServer.start (DNS_PORT, "*", WiFi.softAPIP ());
}

/// Connect the ESP to a network
/// This is really for debugging, it's a lot
/// easier to do most of the testing from a
/// desktop webbrowser with developer tools
/// than it is to work on a phone.
/// You need to define your user/pw in Secret.h
/// as included above

#if defined (NOT_AP)
void ConnectToNetwork (void)
{
   Serial.print ("Connecting");
   WiFi.disconnect ();
   WiFi.mode (WIFI_STA);

   wl_status_t ws = WiFi.begin (NETWORK_SSID, NETWORK_PASS); // Connect to WiFi network
   Serial.printf ("\nWiFi.Begin: %d\n", ws);

   // Wait until we connect, and do some status messages
   while (WiFi.localIP()[0] == 0)
   {
      char stat[20];
   
      Serial.print (".");
      sprintf (stat, "%d\n", WiFi.status ());
      Serial.print (stat);
      delay (1000);
   }

   Serial.print ("localIP: ");
   Serial.println (WiFi.localIP());
}
#endif

boolean connectRequired;
unsigned long lastConnectTry = 0;
int laststatus = WL_IDLE_STATUS;

#if defined (USE_LCD_DISPLAY)
// Pad a string with blanks.
// This is used to clear to end-of-line on the OLE display
static void pad (char *str, size_t strsize)
{
   char *p = str + strlen (str);
   while ((size_t) (p - str) < strsize-1)
   {
      *p++ = ' ';
      *p = '\0';
   }
}
#endif


void drawString (int x, int y, const char *s)
{
   #if defined (U8G2LIB_HH)
      u8x8.drawStr (8*x+8, 8*y+8, s);
//      u8x8.sendBuffer ();
   #elif defined (U8X8LIB_HH)
      u8x8.drawString (x, y, s);
   #endif
}

// The OLED is 4 rows of 16 characters.
// 0123456789012345
// BRC WiFi
// Red xxx Ban xxx
// Batt xxxx
// 22-01-01 12:00

char prevLastPageReq[512];
void DisplayOLEDStatus (void)
{
   static time_t lastDisplayUpdate = 0;
   static time_t prevActivity = 0;
   static int prevRedirects = -1;
   static int prevBanned = -1;

   // Work out if we need to refresh the display. Do it as a batch so the serial output is all or nothing
   if (prevRedirects != EEData.legalShown || prevBanned != EEData.totalBanned || prevActivity != EEData.lastActivity)
   {
      #if defined (U8G2LIB_HH)
         u8x8.setPowerSave (0);
      #endif
      (void) time (&lastDisplayUpdate);
      prevRedirects = EEData.legalShown;
      prevBanned = EEData.totalBanned;
      prevActivity = EEData.lastActivity;

      char buffer[17];
      #if defined (USE_LCD_DISPLAY)
         drawString (0, 0, EEData.hostname);
      #endif
     
      snprintf (buffer, sizeof buffer, "Red %-3d Ban %-3d ", EEData.legalShown, EEData.totalBanned);
      Serial.println (buffer);
      yield ();
      #if defined (USE_LCD_DISPLAY)
         pad (buffer, sizeof buffer);
         drawString (0, 1, buffer);
      #endif

      strftime (buffer, sizeof buffer, "%y-%m-%d %H:%M", gmtime (&EEData.lastActivity));
      Serial.println (buffer);
      yield ();
      #if defined (USE_LCD_DISPLAY)
         pad (buffer, sizeof buffer);
         drawString (0, 3, buffer);
         #if defined (U8G2LIB_HH)
            u8x8.sendBuffer ();
            Serial.println ("sendBuffer");
         #endif
      #endif
   }

   if (strncmp (lastPageReq, prevLastPageReq, sizeof prevLastPageReq) != 0)
   {
      str_copy (prevLastPageReq, lastPageReq, sizeof prevLastPageReq);
      Serial.print ("Last Page Requested: ");
      Serial.println (lastPageReq);
      yield ();
   }

   #if defined (USE_LCD_DISPLAY)
      u8x8.refreshDisplay();    // only required for SSD1606/7  

      time_t now = time (NULL);
      if ((EEData.displayTimeoutSeconds > 0) && difftime (now, lastDisplayUpdate) > EEData.displayTimeoutSeconds)
         u8x8.setPowerSave (1);
   #endif
}

void loop (void)
{
//   static unsigned long prevHeap = 4000000;
   DisplayOLEDStatus ();
   SaveEEDataIfNeeded (EEDataAddr, &EEData, sizeof EEData);

   if (RestartRequired)
   {
      if (EEChanged)
         WriteEEData (EEDataAddr, &EEData, sizeof EEData);

      ESP.restart ();
      while (1)
         ;
   }
   yield ();

   #if defined (NOT_AP)
      if (connectRequired) 
      {
         Serial.println ( "Connect requested" );
         connectRequired = false;
         ConnectToNetwork ();
         lastConnectTry = millis();
         yield ();
      }
   #endif

   int currentStatus = WiFi.status();
   if ((currentStatus == 0) && (millis() > (lastConnectTry + 60000))) 
   {
      /* If WLAN disconnected and idle try to connect */
      /* Don't set retry time too low as retry interfere the softAP operation */
      connectRequired = true;
   }

   yield ();
  
   if (laststatus != currentStatus) 
   {
      // WLAN status change
      Serial.print ("Status: ");
      Serial.println (WiFiStatus (currentStatus));
      yield ();
      laststatus = currentStatus;
      if (currentStatus == WL_CONNECTED) 
      {
         /* Just connected to WLAN */
         Serial.println ( "" );
         Serial.print ("IP address: ");
         Serial.println (WiFi.localIP());
         yield ();
      }
      else if (currentStatus == WL_NO_SSID_AVAIL) 
      {
        WiFi.disconnect();
      }
   }

   dnsServer.processNextRequest();
}

/// Convert a WiFi Status to a human readable string
/// Codes defined in wl_definitions.h
String  WiFiStatus (int s)
{
   switch (s)
   {
   case WL_NO_SHIELD:  return "No Shield";              // 255
   case WL_IDLE_STATUS: return "Idle";                  // 0
   case WL_NO_SSID_AVAIL: return "No SSID Available";   // 1
   case WL_SCAN_COMPLETED: return "Completed";          // 2
   case WL_CONNECTED: return "Connected";               // 3
   case WL_CONNECT_FAILED: return "Connect Failed";     // 4
   case WL_CONNECTION_LOST: return "Connection Lost";   // 5
   #if defined (ESP8266)
      case WL_WRONG_PASSWORD: return "Wrong Password";     // 6
   #else
      case WL_STOPPED: return "Stopped";                   // 254
   #endif
   case WL_DISCONNECTED: return "Disconnected";         // 7
   default: return "Unknown status";                    //
   }
}

uint8_t *getMacAddress ()
{
   static uint8_t _macAddress[6];
   #if defined (ESP8266)
      wifi_get_macaddr (STATION_IF, _macAddress);
   #elif defined (ESP32)

      wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT ();
      esp_wifi_init (&cfg);
      esp_err_t rc = esp_wifi_get_mac (WIFI_IF_STA, _macAddress);
      esp_wifi_deinit ();
      Serial.printf ("rc = %x\n", rc);
   #else
      memset (_macAddress, 0, sizeof _macAddress);
   #endif
   Serial.printf ("%02x:%02x:%02x:%02x:%02x:%02x\n", _macAddress[0], _macAddress[1], _macAddress[2], _macAddress[3], _macAddress[4], _macAddress[5]);

   return _macAddress;
}

static char localIPAddress[4*3+3+1] = "";
const char *localIP (void)
{
   if (localIPAddress[0] =='\0')
   {
      #if defined (NOT_AP)
         str_copy (localIPAddress, WiFi.localIP().toString().c_str(), sizeof localIPAddress);
      #else
         str_copy (localIPAddress, WiFi.softAPIP().toString().c_str(), sizeof localIPAddress);
      #endif
   }

   return localIPAddress;   
}

/// Copy the src string to the destination and insert a 
/// trailing null if needed.
void str_copy (char *dest, const char *src, size_t len)
{
   strncpy (dest, src, len);
   dest[len-1] = '\0';
}