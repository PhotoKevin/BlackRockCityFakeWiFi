#include <Arduino.h>
#include <string.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <TypeConversion.h>
#include <Crypto.h>
#include <time.h>

#include "BMWifi.h"
using namespace experimental::crypto;

namespace TypeCast = experimental::TypeConversion;

#define MAXLOGINS 5

struct authenticated_t
{
   long long device;
   //uint8_t cookie[experimental::crypto::SHA256::NATURAL_LENGTH];
   String cookie;
   time_t expires;
};
struct  authenticated_t authenticated[MAXLOGINS];
static void expireIPs (void);


long long mac2DeviceID (uint8 *mac)
{
   long long address = 0;

   for (int i=0; i<6; i++)
   {
      address <<= 8;
      address += mac[i];
   }

   return address;
}

long long ip2DeviceID (IPAddress ip)
{
   long long address = 0;

   for (int i=0; i<4; i++)
   {
      address <<= 8;
      address += ip[i];
   }

   return address;   
}

static void initSecurity (void)
{
   static bool initialized = false;
   if (!initialized)
   {
      Serial.println ("Initializing security");
      for (int i=0; i<MAXLOGINS; i++)
         authenticated[i].cookie = "";
      initialized = true;
   }
}


bool isLoggedIn (long long device, const String cookie)
{
   initSecurity ();
   expireIPs ();

   Serial.printf ("isLoggedIn \n");
   Serial.println (cookie);
   for (int i=0; i<MAXLOGINS; i++)
   {
      Serial.printf ("Check pos %d: ", i);
      Serial.println (authenticated[i].cookie);
      if (authenticated[i].cookie != "")
      {
         Serial.printf ("Cookie %s contains %s\n", cookie.c_str(), authenticated[i].cookie.c_str());
         if (cookie.indexOf (authenticated[i].cookie) >= 0)
            return true;
      }
   }

   Serial.println ("Not logged in");
   return false;
}

bool Login (String user, String pw, long long device, String &token)
{
   initSecurity ();
   expireIPs ();

   if (user.equalsIgnoreCase(EEData.username) && pw.equals (EEData.password))
   {
      Serial.printf ("User/Pw good\n");
      for (int j=0; j<MAXLOGINS; j++)
      {
         if (authenticated[j].cookie == "")
         {
            Serial.printf ("Pos %d is open\n", j);
            uint8_t cookie[experimental::crypto::SHA256::NATURAL_LENGTH*2+1];
            SHA256::hash(&device, sizeof device, cookie);

            authenticated[j].device = device;

            authenticated[j].cookie = TypeCast::uint8ArrayToHexString (cookie, sizeof cookie);
            authenticated[j].expires = time (NULL) + 3*60;  // three minutes
            token = authenticated[j].cookie;
            Serial.printf ("Check pos %d: ", j);
            Serial.println (authenticated[j].cookie);

            return true;
         }
      }
   }

   return false;
}

static void expireDevices (void)
{
   time_t current = time (NULL);
   for (int i=0; i<MAXLOGINS; i++)
   {
      if ((authenticated[i].cookie != "") && (difftime (current, authenticated[i].expires) > 0))
      {
         Serial.println ("Expire " + authenticated[i].cookie);
         authenticated[i].cookie = "";
      }
   }
}


long long clientAddress (void)
{
   long long address = 0;
   WiFiClient cli = server.client ();
   IPAddress clientIP = server.client().remoteIP();
   
   uint8_t client_count = wifi_softap_get_station_num();
//   int i = 1;
   struct station_info *station_list = wifi_softap_get_station_info();
   while (station_list != NULL) 
   {
      IPAddress station = IPAddress ((&station_list->ip)->addr);
      if (clientIP == station)
      {
         address = mac2DeviceID (station_list->bssid);
         
         String station_ip = station.toString();
//         char station_mac[18] = {0};
//         sprintf(station_mac, "%02X:%02X:%02X:%02X:%02X:%02X", MAC2STR(station_list->bssid));         
//         Serial.printf ("%d. IP: %s MAC: %s\n", 0, station_ip.c_str(), station_mac);
         station_list = STAILQ_NEXT(station_list, next);
      }   
   }
   wifi_softap_free_station_info();

   if (address == 0)
      address = ip2DeviceID (clientIP);
      
   return address;
}

bool isMasterDevice (void)
{
//   Serial.printf ("%llx", clientAddress ());
//   Serial.printf (" vs %llx\n", mac2ll (EEData.masterDevice));
   return clientAddress () == mac2DeviceID (EEData.masterDevice);
   
}

