

#if !defined (ESP8266)
#error Change your board type to generic ESP8266
// https://github.com/esp8266/Arduino#installing-with-boards-manager
#endif

#define NOT_AP    // Define for debugging as just a device on the network

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

U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(/* reset=*/ U8X8_PIN_NONE);   // Adafruit ESP8266/32u4/ARM Boards + FeatherWing OLED

#include "BMWifi.h"

// You need to supply your own Secret.h defining 
//#define SSID "<SSID>"      
//#define PASS "<password>"  
//#define APSSID  "<AP SSID>"
//#define APPASS  "<AP password>"

#include "Secret.h"




ESP8266WebServer server (80);                         // HTTP server will listen at port 80
const byte DNS_PORT = 53;

DNSServer dnsServer;

const int ledPin = 2;

struct eeprom_data_t  EEData;

void  ConnectToNetwork (void);
void  SetupAP (void);

void setup (void)
{
   char result[16];
   Serial.begin (115200);                           // full speed to monitor

   u8x8.begin();
   u8x8.setPowerSave(0);
 
   u8x8.setFont (u8x8_font_chroma48medium8_r);
   u8x8.drawString (0,0,"BRC Wifi");
//   u8x8.refreshDisplay();    // only required for SSD1606/7  

   pinMode (ledPin, OUTPUT);

   Serial.print ("\nBRC Wifi Starting\n");

   EEPROM.begin (128); // Can go to 4096, probably

//ESP.wdtDisable ();                               // used to debug, disable wachdog timer,
  
   #if defined (NOT_AP)
      ConnectToNetwork ();
   #else
      SetupAP ();
   #endif

   setupWebServer ();
   yield ();

   ReadEEData (EEDataAddr, &EEData, sizeof EEData);

   DisplayStatus ();
   yield ();
  
}

void SetupAP (void)
{
  int rc;
  Serial.print ("AP");
  WiFi.mode (WIFI_AP);
  Serial.print ("Set Mode");
//  WiFi.softAPConfig (IPAddress (192, 168, 4, 4), 0x01F4A8C0, 0x00FFFFFF);

  rc = WiFi.softAP (APSSID, APPASS);
  Serial.printf ("softAP : %d\n", rc);

  IPAddress ip = WiFi.softAPIP ();

  char result[16];
  sprintf (result, "AP is %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  Serial.println (result);

//  IPAddress local_ip; IPAddress gateway; IPAddress subnet;
//  local_ip

  dnsServer.setErrorReplyCode (DNSReplyCode::NoError);
  dnsServer.start (DNS_PORT, "*", ip);
}

void ConnectToNetwork (void)
{
   char stat[20];
   
   Serial.print ("Connecting");
   WiFi.disconnect ();
   wl_status_t ws = WiFi.begin (SSID, PASS);                         // Connect to WiFi network
   Serial.printf ("\nWiFi.Begin: %d\n", ws);
   
   while (WiFi.localIP()[0] == 0)
   {
      Serial.print (".");
      sprintf (stat, "%d\n", WiFi.status ());
      Serial.print (stat);
      delay (100);
   }
}

// https://android.stackexchange.com/questions/63481/how-does-android-determine-if-it-has-an-internet-connection
//https://android.stackexchange.com/questions/170387/android-wifi-says-connected-no-internet-but-internet-works-just-fine
//https://www.hackster.io/rayburne/esp8266-captive-portal-5798ff

boolean connect;
long lastConnectTry = 0;
int laststatus = WL_IDLE_STATUS;
const char *myHostname = "GTDonation";

void loop (void)
{
   static int noStats = -1;
   if (noStats != WiFi.softAPgetStationNum())
   {
      noStats = WiFi.softAPgetStationNum();
      Serial.printf ("Connects: %d\n", noStats);
   }
//   Serial.printf("Free Heap: %d Bytes\n", ESP.getFreeHeap());   
   if (connect) 
   {
      Serial.println ( "Connect requested" );
      connect = false;
      ConnectToNetwork ();
      lastConnectTry = millis();
   }
   
   {
      int s = WiFi.status();
      if (s == 0 && millis() > (lastConnectTry + 60000) ) 
      {
         /* If WLAN disconnected and idle try to connect */
         /* Don't set retry time too low as retry interfere the softAP operation */
         connect = true;
      }
      if (laststatus != s) 
      {
         // WLAN status change
         Serial.print ( "Status: " );
         Serial.println ( WiFiStatus (s) );
         laststatus = s;
         if (s == WL_CONNECTED) 
         {
            /* Just connected to WLAN */
            Serial.println ( "" );
            Serial.print ( "IP address: " );
            Serial.println ( WiFi.localIP() );
         
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
         else if (s == WL_NO_SSID_AVAIL) 
         {
           WiFi.disconnect();
         }
      }
   }
   
   
   /////////////
   dnsServer.processNextRequest();
   server.handleClient ();  // checks for incoming messages
    
   yield ();
}

String  WiFiStatus (int s)
{
  switch (s)
  {
    case WL_NO_SHIELD:  return "No Shield";
    case WL_IDLE_STATUS: return "Idle";
    case WL_NO_SSID_AVAIL: return "No SSID Available";
    case WL_SCAN_COMPLETED: return "Completed";
    case WL_CONNECTED: return "Connected";
    case WL_CONNECT_FAILED: return "Connect Failed";
    case WL_CONNECTION_LOST: return "Connection Lost";
    case WL_DISCONNECTED: return "Disconnected";
    default: return "Unknown status";
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

void showConnectedDevices()
{
    auto client_count = wifi_softap_get_station_num();
    Serial.printf("Total devices connected = %d\n", client_count);

    auto i = 1;
    struct station_info *station_list = wifi_softap_get_station_info();
    while (station_list != NULL) {
        auto station_ip = IPAddress((&station_list->ip)->addr).toString().c_str();
        char station_mac[18] = {0};
        sprintf(station_mac, "%02X:%02X:%02X:%02X:%02X:%02X", MAC2STR(station_list->bssid));
        Serial.printf("%d. %s %s", i++, station_ip, station_mac);
        station_list = STAILQ_NEXT(station_list, next);
    }
    wifi_softap_free_station_info();
}
