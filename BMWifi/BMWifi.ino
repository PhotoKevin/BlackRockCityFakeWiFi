//#define USE_LCD_DISPLAY
//#define NOT_AP    // Define for debugging as just a device on the network

#if !defined (ESP8266)
#error Change your board type to generic ESP8266
// https://github.com/esp8266/Arduino#installing-with-boards-manager
#endif

#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Ticker.h>

#include "Config.h"

#if defined (USE_LCD_DISPLAY)
// https://github.com/olikraus/u8g2
#include <U8x8lib.h>

#ifdef u8g2_HAVE_HW_SPIx
#include <SPI.h>
#endif

U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(/* reset=*/ 16);
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

ESP8266WebServer server (80);                         // HTTP server will listen at port 80
const byte DNS_PORT = 53;

DNSServer dnsServer;

struct eeprom_data_t  EEData;
int EEChanged = 0;

#if defined (NOT_AP)
void  ConnectToNetwork (void);
#endif
void  SetupAP (void);

ADC_MODE(ADC_VCC);      // Needed to make the ESP.getVCC function work.

void setup (void)
{
   Serial.begin (115200);                           // full speed to monitor

#if defined (USE_LCD_DISPLAY)
   u8x8.begin();
   u8x8.setPowerSave(0);
 
   u8x8.setFont (u8x8_font_chroma48medium8_r);
#endif
   Serial.print ("\nBMWifi Starting\n");

   EEPROM.begin (sizeof EEData); 
   Serial.printf ("EEData is %d bytes\n", sizeof EEData);   
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

   Serial.println (getSystemInformation ());
   #if defined (NOT_AP)
      ConnectToNetwork ();
   #else
      SetupAP ();
   #endif

   setupWebServer ();
   yield ();

   if (EEData.totalBanned < 0)
      memset (&EEData, 0, sizeof EEData);

   yield ();
}

/// Setup the ESP8266 as an Access Point

void SetupAP (void)
{
   int rc;
   WiFi.mode (WIFI_AP);

//   apIP = IPAddress(EEData.ipAddress);
//   netMsk = IPAddress(EEData.netmask);

   WiFi.softAPConfig (apIP, apIP, netMsk);
   rc = WiFi.softAP (EEData.SSID); // No password, this is an open access point

   Serial.printf ("softAP : %d\n", rc);

   Serial.print ("AP is ");
   Serial.println (WiFi.softAPIP ().toString());

   // Set up a DNS server. 
   dnsServer.setErrorReplyCode (DNSReplyCode::NoError);
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

// The OLED is 4 rows of 16 characters.
// 0123456789012345
// BRC WiFi
// Red xxx Ban xxx
// Batt xxxx
// 22-01-01 12:00

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
      pad (buffer, sizeof buffer);
      Serial.println (buffer);
#if defined (USE_LCD_DISPLAY)
      u8x8.drawString (0, 1, buffer);
#endif

      snprintf (buffer, sizeof buffer, "Batt %-4d", ESP.getVcc());
      pad (buffer, sizeof buffer);
      Serial.println (buffer);
#if defined (USE_LCD_DISPLAY)
      u8x8.drawString (0, 2, buffer);
#endif

      strftime (buffer, sizeof buffer, "%y-%m-%d %H:%S", gmtime (&EEData.lastActivity));
      pad (buffer, sizeof buffer);
      Serial.println (buffer);
#if defined (USE_LCD_DISPLAY)
      u8x8.drawString (0, 3, buffer);
#endif
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
      Serial.printf ("Connects: %d\n", noStats);
   }

   unsigned long heap = ESP.getFreeHeap ();
   if (heap < prevHeap)
   {
      Serial.printf("Free Heap: %lu Bytes\n", heap);
      prevHeap = heap;
   }

   #if defined (NOT_AP)
//      MDNS.update ();
      MDNS.announce ();
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
      
         // Setup MDNS responder
         if (!MDNS.begin(EEData.hostname, WiFi.localIP())) 
            Serial.println("Error setting up MDNS responder!");
         else 
         {
            Serial.printf ("mDNS responder started: %s\n", EEData.hostname);
            // Add service to MDNS-SD
            MDNS.addService ("http", "tcp", 80);
         }
      }
      else if (currentStatus == WL_NO_SSID_AVAIL) 
      {
        WiFi.disconnect();
      }
   }

   dnsServer.processNextRequest();
   server.handleClient ();  // checks for incoming messages
    
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
   case WL_WRONG_PASSWORD: return "Wrong Password";     // 6
   case WL_DISCONNECTED: return "Disconnected";         // 7
   default: return "Unknown status";                    //
   }
}

void xDisplayStatus (void)
{
   #if defined (NOT_AP)
      Serial.print ("localIP: ");
      Serial.println (WiFi.localIP());
   #else
      Serial.print ("AP: ");
      Serial.println (WiFi.softAPIP());
   #endif
}


void displayClients () 
{
   unsigned char number_client;
   struct station_info *stat_info;
   
   struct ip_addr *IPaddress;
   IPAddress address;
   int i=1;
   
   number_client= wifi_softap_get_station_num();
   stat_info = wifi_softap_get_station_info();
   
   Serial.print(" Total connected_client are = ");
   Serial.println(number_client);
   
   while (stat_info != NULL) 
   {
      IPaddress = (ip_addr *) &stat_info->ip;
      address = IPaddress->addr;
      
      Serial.print("client= ");
      
      Serial.print(i);
      Serial.print(" ip adress is = ");
      Serial.print((address));
      Serial.print(" with mac adress is = ");
      
      Serial.print(stat_info->bssid[0],HEX);
      Serial.print(stat_info->bssid[1],HEX);
      Serial.print(stat_info->bssid[2],HEX);
      Serial.print(stat_info->bssid[3],HEX);
      Serial.print(stat_info->bssid[4],HEX);
      Serial.print(stat_info->bssid[5],HEX);
      
      stat_info = STAILQ_NEXT(stat_info, next);
      i++;
      Serial.println();
   }
   wifi_softap_free_station_info();

   delay(500);
}
