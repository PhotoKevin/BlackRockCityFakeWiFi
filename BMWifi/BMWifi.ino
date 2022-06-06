

#if !defined (ESP8266)
#error Change your board type to generic ESP8266
// https://github.com/esp8266/Arduino#installing-with-boards-manager
#endif

//#define NOT_AP    // Define for debugging as just a device on the network

#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Ticker.h>

#include "Config.h"

// https://github.com/olikraus/u8g2
#include <U8x8lib.h>

#ifdef u8g2_HAVE_HW_SPIx
#include <SPI.h>
#endif

U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(/* reset=*/ 16);

//https://android.googlesource.com/platform/frameworks/base/+/c80f952/core/java/android/net/CaptivePortalTracker.java
#include "BMWifi.h"

// You need to supply your own Secret.h defining 
//#define SSID "<SSID>"      
//#define PASS "<password>"  
#include "Secret.h"

const char *myHostname = HOST_NAME;
bool clockSet = false;

IPAddress apIP (IP_ADDRESS);
IPAddress netMsk (NET_MASK);

ESP8266WebServer server (80);                         // HTTP server will listen at port 80
const byte DNS_PORT = 53;

DNSServer dnsServer;

struct eeprom_data_t  EEData;
int EEChanged = 0;

#if defined (NOT_AP)
void  ConnectToNetwork (void);
#endif
void  SetupAP (void);

ADC_MODE(ADC_VCC);

void setup (void)
{
   Serial.begin (115200);                           // full speed to monitor

   u8x8.begin();
   u8x8.setPowerSave(0);
 
   u8x8.setFont (u8x8_font_chroma48medium8_r);
   u8x8.drawString (0, 0, myHostname);
   Serial.print ("\nBRC Wifi Starting\n");

   EEPROM.begin (128); // Can go to 4096, probably
  
   #if defined (NOT_AP)
      ConnectToNetwork ();
   #else
      SetupAP ();
   #endif

   setupWebServer ();
   yield ();

   ReadEEData (EEDataAddr, &EEData, sizeof EEData);
   if (EEData.totalBanned < 0)
      memset (&EEData, 0, sizeof EEData);

   DisplayStatus ();
   yield ();
}

/// Setup the ESP8266 as an Access Point

void SetupAP (void)
{
   int rc;
   WiFi.mode (WIFI_AP);

   WiFi.softAPConfig (apIP, apIP, netMsk);
   rc = WiFi.softAP (APSSID); // No password, this is an open access point

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
   wl_status_t ws = WiFi.begin (SSID, PASS); // Connect to WiFi network
   Serial.printf ("\nWiFi.Begin: %d\n", ws);

   // Wait until we connect, and do some status messages
   while (WiFi.localIP()[0] == 0)
   {
      Serial.print (".");
      sprintf (stat, "%d\n", WiFi.status ());
      Serial.print (stat);
      delay (1000);
   }
}
#endif

// https://android.stackexchange.com/questions/63481/how-does-android-determine-if-it-has-an-internet-connection
//https://android.stackexchange.com/questions/170387/android-wifi-says-connected-no-internet-but-internet-works-just-fine
//https://www.hackster.io/rayburne/esp8266-captive-portal-5798ff

boolean connectRequired;
long lastConnectTry = 0;
int laststatus = WL_IDLE_STATUS;


// Pad a string with blanks.
// This is used to clear to end-of-line on the OLE display
static void pad (char *str, size_t strsize)
{
   char *p = str + strlen (str);
   while (p - str < strsize-1)
   {
      *p++ = ' ';
      *p = '\0';
   }
}

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
   if (prevRedirects != EEData.totalRedirects || prevBanned != EEData.totalBanned || prevActivity != EEData.lastActivity)
   {
      prevRedirects = EEData.totalRedirects;
      prevBanned = EEData.totalBanned;
      prevActivity = EEData.lastActivity;

      char buffer[17];
      u8x8.drawString (0, 0,"BRC Wifi");

      snprintf (buffer, sizeof buffer, "Red %-3d Ban %-3d ", EEData.totalRedirects, EEData.totalBanned);
      pad (buffer, sizeof buffer);
      Serial.println (buffer);
      u8x8.drawString (0, 1, buffer);

      snprintf (buffer, sizeof buffer, "Batt %-4d", ESP.getVcc());
      pad (buffer, sizeof buffer);
      Serial.println (buffer);
      u8x8.drawString (0, 2, buffer);

      strftime (buffer, sizeof buffer, "%y-%m-%d %H:%S", gmtime (&EEData.lastActivity));
      pad (buffer, sizeof buffer);
      Serial.println (buffer);
      buffer[15] = '\0';
      u8x8.drawString (0, 3, buffer);
   }

   u8x8.refreshDisplay();    // only required for SSD1606/7  
}


void loop (void)
{
   DisplayOLEDStatus ();
   static int noStats = -1;
   if (noStats != WiFi.softAPgetStationNum())
   {
      noStats = WiFi.softAPgetStationNum();
      Serial.printf ("Connects: %d\n", noStats);
   }
//   Serial.printf("Free Heap: %d Bytes\n", ESP.getFreeHeap());   

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
      
         // Setup MDNS responder
         if (!MDNS.begin(myHostname)) 
            Serial.println("Error setting up MDNS responder!");
         else 
         {
            Serial.println("mDNS responder started");
            // Add service to MDNS-SD
            MDNS.addService("http", "tcp", 80);
         }
      }
      else if (currentStatus == WL_NO_SSID_AVAIL) 
      {
        WiFi.disconnect();
      }
   }
  
   
   /////////////
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

void DisplayStatus (void)
{
   #if defined (NOT_AP)
      Serial.print ("localIP: ");
      Serial.println (WiFi.localIP());
   #else
      Serial.print ("AP: ");
      Serial.println (WiFi.softAPIP());
   #endif
}


void client_status() 
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
