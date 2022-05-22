

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
//#define APSSID  "<AP SSID>"
//#define APPASS  "<AP password>"

#include "Secret.h"

const char *myHostname = "BRCWiFi";
IPAddress apIP (8, 18, 8, 8);
IPAddress netMsk (255, 255, 255, 0);



ESP8266WebServer server (80);                         // HTTP server will listen at port 80
const byte DNS_PORT = 53;

DNSServer dnsServer;

const int ledPin = 2;

struct eeprom_data_t  EEData;
int EEChanged = 0;

void  ConnectToNetwork (void);
void  SetupAP (void);

void setup (void)
{
   Serial.begin (115200);                           // full speed to monitor

   u8x8.begin();
   u8x8.setPowerSave(0);
 
   u8x8.setFont (u8x8_font_chroma48medium8_r);
   u8x8.drawString (0,0,"BRC Wifi");
//   u8x8.refreshDisplay();    // only required for SSD1606/7  

   pinMode (ledPin, OUTPUT);

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
/// The APSSID and APPASS should have been 
/// defined in Secret.h which was included 
/// at the top.

void SetupAP (void)
{
   int rc;
   WiFi.mode (WIFI_AP);

   WiFi.softAPConfig (apIP, apIP, netMsk);
   rc = WiFi.softAP (APSSID);

//   rc = WiFi.softAP (APSSID, APPASS);
   Serial.printf ("softAP : %d\n", rc);

   IPAddress ip = WiFi.softAPIP ();

   char result[16];
   sprintf (result, "AP is %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
   Serial.println (result);

   // Set up a DNS server. 
   dnsServer.setErrorReplyCode (DNSReplyCode::NoError);
   dnsServer.start (DNS_PORT, "*", ip);
}

/// Connect the ESP to a network
/// This is really for debugging, it's a lot
/// easier to do most of the testing from a
/// desktop webbrowser with developer tools
/// than it is to work on a phone.
/// You need to define your user/pw in Secret.h
/// as included above

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

// https://android.stackexchange.com/questions/63481/how-does-android-determine-if-it-has-an-internet-connection
//https://android.stackexchange.com/questions/170387/android-wifi-says-connected-no-internet-but-internet-works-just-fine
//https://www.hackster.io/rayburne/esp8266-captive-portal-5798ff

boolean connectRequired;
long lastConnectTry = 0;
int laststatus = WL_IDLE_STATUS;


void DisplayOLEDStatus (void)
{
   static time_t prevTime = 0;
   static int prevRedirects = -1;
   static int prevBanned = -1;
   bool refresh = false;
   time_t now = time (NULL);

   if (prevRedirects != EEData.totalRedirects || prevBanned != EEData.totalBanned || difftime (now, prevTime) > 5)
      refresh = true;

   char buffer[32];
   u8x8.drawString (0,0,"BRC Wifi");

   snprintf (buffer, sizeof buffer, "Redirects %d  ", EEData.totalRedirects);
   if (refresh)
   {
      Serial.println (buffer);
      u8x8.drawString (0,1, buffer);
      prevRedirects = EEData.totalRedirects;
   }

   snprintf (buffer, sizeof buffer, "Banned %d", EEData.totalBanned);
   if (refresh)
   {
      Serial.println (buffer);
      u8x8.drawString (0,2, buffer);
      prevBanned = EEData.totalBanned;
   }

   if (refresh)
   { 
      strftime (buffer, sizeof buffer, "%FT%T", gmtime (&now));
      Serial.println (buffer);
      buffer[15] = '\0';
      u8x8.drawString (0,3, buffer);
      prevTime = now;
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
   if (connectRequired) 
   {
      Serial.println ( "Connect requested" );
      connectRequired = false;
      ConnectToNetwork ();
      lastConnectTry = millis();
   }
   

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
   char result[16];

   #if defined (NOT_AP)
      sprintf (result, "localIP: %3d.%3d.%3d.%3d\n", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
   #else  
      sprintf (result, "AP: %d.%d.%d.%d\n", WiFi.softAPIP()[0], WiFi.softAPIP()[1], WiFi.softAPIP()[2], WiFi.softAPIP()[3]);
   #endif
   
   Serial.print (result);
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
   delay(500);
}
