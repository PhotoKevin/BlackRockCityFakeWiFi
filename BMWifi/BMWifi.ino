//#define USE_LCD_DISPLAY
//#define NOT_AP    // Define for debugging as just a device on the network


#include <DNSServer.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Ticker.h>

#if defined (ESP32)
   #include <WiFi.h>
   #include <ESPmDNS.h>
   #include <esp_https_server.h>

#else
   #error Change your board type to an ESP32
#endif

char lastPageReq[512];

#include "Config.h"

#if defined (USE_LCD_DISPLAY)
// https://github.com/olikraus/u8g2
#include <U8x8lib.h>

// #define u8g2_HAVE_HW_SPI
// #ifdef u8g2_HAVE_HW_SPI
// #include <SPI.h>
// #endif

// Heltec WiFi Kit 8
//U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(/* reset=*/ 16);


// Heltec WiFi Kit 32
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);
//U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset=*/ 16);

#endif

//https://android.googlesource.com/platform/frameworks/base/+/c80f952/core/java/android/net/CaptivePortalTracker.java
#include "BMWifi.h"

// You need to supply your own Secret.h defining 
//#define NETWORK_SSID "<SSID>"      
//#define NETWORK_PASS "<password>"  
//#if defined (NOT_AP)
#include "Secret.h"
//#endif

bool clockSet = false;

IPAddress apIP(10,47,4,7);
IPAddress netMsk(255,255,255,0);


#if defined (ESP32)
   httpd_handle_t secure_http = NULL;
   httpd_handle_t insecure_http = NULL;
#endif

const byte DNS_PORT = 53;

DNSServer dnsServer;

struct eeprom_data_t  EEData;
int EEChanged = 0;

#if defined (NOT_AP)
void  ConnectToNetwork (void);
#endif
void  SetupAP (void);

//ADC_MODE(ADC_VCC);      // Needed to make the ESP.getVCC function work.

//#include "heltec.h"
void setup (void)
{
   Serial.begin (115200);                           // full speed to monitor
   
   // Initialize the Heltec ESP32 object
//   Heltec.begin(true /*DisplayEnable Enable*/, true /*LoRa Disable*/, true /*Serial Enable*/, true /*PABOOST Enable*/, 470E6 /**/);
//	Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Enable*/, true /*Serial Enable*/);


#if defined (USE_LCD_DISPLAY)
   u8x8.begin();
   u8x8.setPowerSave(0);
 
   u8x8.setFont (u8x8_font_chroma48medium8_r);
#endif
   Serial.print ("\nBMWifi Starting\n");

   EEPROM.begin (sizeof EEData); 
   ReadEEData (EEDataAddr, &EEData, sizeof EEData);
   if (EEData.eepromDataSize != sizeof EEData)
   {
      Serial.println ("Setting default EEData values");
      uint8_t ip[] = DEFAULT_IPADDRESS;
      uint8_t mask[] = DEFAULT_NETMASK;
      uint8_t master[] = DEFAULT_MASTER;
      memset (&EEData, 0, sizeof EEData);
      EEData.eepromDataSize = sizeof EEData;
      strncpy (EEData.SSID, DEFAULT_SSID, sizeof EEData.SSID);
      strncpy (EEData.username, DEFAULT_ADMIN, sizeof EEData.username);
      strncpy (EEData.password, DEFAULT_PASSWORD, sizeof EEData.password);
      strncpy (EEData.hostname, DEFAULT_HOSTNAME, sizeof EEData.hostname);
      for (int i=0; i<4; i++)
      {
         EEData.ipAddress[i] = ip[i];
         EEData.netmask[i] = mask[i];
      }

      for (int i=0; i<6; i++)
         EEData.masterDevice[i] = master[i];
      EEChanged = 1;
   }
strncpy (EEData.hostname, "www.gt.org", sizeof EEData.hostname);

   Serial.println (getSystemInformation ());
   #if defined (NOT_AP)
      ConnectToNetwork ();
   #else
      SetupAP ();
   #endif

   // Setup MDNS responder
   Serial.println (EEData.hostname);
   if (!MDNS.begin (EEData.hostname)) 
      Serial.println("Error setting up MDNS responder!");
   else 
   {
      Serial.printf ("mDNS responder started: %s\n", EEData.hostname);
      // Add service to MDNS-SD
      MDNS.addService ("http", "tcp", 80);
      MDNS.addService ("https", "tcp", 443);
   }


   setupWebServer ();

   yield ();
}

/// Setup the ESP32 as an Access Point

void SetupAP (void)
{
   int rc;
   WiFi.mode (WIFI_AP);

//   apIP = IPAddress(EEData.ipAddress);
//   netMsk = IPAddress(EEData.netmask);

   WiFi.softAPConfig (apIP, apIP, netMsk);
   rc = WiFi.softAP (EEData.SSID); // No password, this is an open access point

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
   char stat[20];
   
   Serial.print ("Connecting");
   WiFi.disconnect ();
   WiFi.mode (WIFI_STA);

   wl_status_t ws = WiFi.begin (NETWORK_SSID, NETWORK_PASS); // Connect to WiFi network
   Serial.printf ("\nWiFi.Begin: %d\n", ws);

   // Wait until we connect, and do some status messages
   while (WiFi.localIP()[0] == 0)
   {
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

// The OLED is 4 rows of 16 characters.
// 0123456789012345
// BRC WiFi
// Red xxx Ban xxx
// Batt xxxx
// 22-01-01 12:00

char prevLastPageReq[512];
void DisplayOLEDStatus (void)
{
   static time_t prevActivity = 0;
   static int prevRedirects = -1;
   static int prevBanned = -1;

   // Work out if we need to refresh the display. Do it as a batch so the serial output is all or nothing
   if (prevRedirects != EEData.legalShown || prevBanned != EEData.totalBanned || prevActivity != EEData.lastActivity)
   {
      prevRedirects = EEData.legalShown;
      prevBanned = EEData.totalBanned;
      prevActivity = EEData.lastActivity;

      char buffer[17];
#if defined (USE_LCD_DISPLAY)
      u8x8.drawString (0, 0, EEData.hostname);
#endif
      snprintf (buffer, sizeof buffer, "Red %-3d Ban %-3d ", EEData.legalShown, EEData.totalBanned);
      Serial.println (buffer);
#if defined (USE_LCD_DISPLAY)
      pad (buffer, sizeof buffer);
      u8x8.drawString (0, 1, buffer);
#endif

#if defined (ESP8266)
      snprintf (buffer, sizeof buffer, "Batt %-4d", ESP.getVcc());
      Serial.println (buffer);
#if defined (USE_LCD_DISPLAY)
      pad (buffer, sizeof buffer);
      u8x8.drawString (0, 2, buffer);
#endif
#endif

      strftime (buffer, sizeof buffer, "%y-%m-%d %H:%S", gmtime (&EEData.lastActivity));
      Serial.println (buffer);
#if defined (USE_LCD_DISPLAY)
      pad (buffer, sizeof buffer);
      u8x8.drawString (0, 3, buffer);
#endif
   }

   if (strncmp (lastPageReq, prevLastPageReq, sizeof prevLastPageReq) != 0)
   {
      strncpy (prevLastPageReq, lastPageReq, sizeof prevLastPageReq);
      Serial.print ("lastPageReq: ");
      Serial.println (lastPageReq);
   }

#if defined (USE_LCD_DISPLAY)
   u8x8.refreshDisplay();    // only required for SSD1606/7  
#endif
}


void loop (void)
{
   static unsigned long prevHeap = 4000000;
   DisplayOLEDStatus ();
   SaveEEDataIfNeeded (EEDataAddr, &EEData, sizeof EEData);

   static int noStats = -1;
   if (noStats != WiFi.softAPgetStationNum())
   {
      noStats = WiFi.softAPgetStationNum();
      Serial.printf ("Connected devices: %d\n", noStats);
   }

   unsigned long heap = ESP.getFreeHeap ();
   if (heap < prevHeap)
   {
 //     Serial.printf("Free Heap: %lu Bytes\n", heap);
      prevHeap = heap;
   }

   #if defined (NOT_AP)
      if (connectRequired) 
      {
         Serial.println ( "Connect requested" );
         connectRequired = false;
         ConnectToNetwork ();
         lastConnectTry = millis();
      }
   #endif

   int currentStatus = WiFi.status();
   if ((currentStatus == 0) && (millis() > (lastConnectTry + 60000))) 
   {
      /* If WLAN disconnected and idle try to connect */
      /* Don't set retry time too low as retry interfere the softAP operation */
      connectRequired = true;
   }

  
   if (laststatus != currentStatus) 
   {
      // WLAN status change
      Serial.print ("Status: ");
      Serial.println (WiFiStatus (currentStatus));
      laststatus = currentStatus;
      if (currentStatus == WL_CONNECTED) 
      {
         /* Just connected to WLAN */
         Serial.println ( "" );
         Serial.print ("IP address: ");
         Serial.println (WiFi.localIP());
      
      }
      else if (currentStatus == WL_NO_SSID_AVAIL) 
      {
        WiFi.disconnect();
      }
   }

   dnsServer.processNextRequest();
   yield ();
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
   #endif
   case WL_DISCONNECTED: return "Disconnected";         // 7
   default: return "Unknown status";                    //
   }
}


