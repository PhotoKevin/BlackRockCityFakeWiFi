#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#if defined (ESP32)
   #include <ESPAsyncWebSrv.h>
   #include "esp_wifi.h"
   #include "esp_wifi_ap_get_sta_list.h"
#elif defined (ESP8266)
   #include <ESPAsyncWebSrv.h>
#else
   #error Change your board type to an ESP32
#endif


#include "BMWifi.h"

#define MAXLOGINS 5

struct authenticated_t
{
   uint64_t device;
   time_t expires;
};
struct  authenticated_t authenticated[MAXLOGINS];
static void expireDevices (void);
static bool isMasterDevice (AsyncWebServerRequest *req);


static uint64_t mac2DeviceID (uint8_t *mac)
{
   uint64_t address = 0;

   for (int i=0; i<6; i++)
   {
      address <<= 8;
      address += mac[i];
   }

   return address;
}

static uint64_t ip2DeviceID (IPAddress ip)
{
   uint64_t address = 0;

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
      for (int i=0; i<MAXLOGINS; i++)
         authenticated[i].device = 0;
      initialized = true;
   }
}


bool isLoggedIn (AsyncWebServerRequest *req)
{
   initSecurity ();
   expireDevices ();

   if (isMasterDevice (req))
      return true;

   uint64_t device = clientAddress (req);

   Serial.printf ("isLoggedIn?  %llx", device);
   for (int i=0; i<MAXLOGINS; i++)
   {
      if (authenticated[i].device != 0)
      {
         if (device == authenticated[i].device)
         {
            Serial.println ("  --> Yes");
            return true;
         }
      }
   }

   Serial.println (" --> No");
   return false;
}

bool Login (String user, String pw, uint64_t device)
{
   Serial.printf ("Logging in\n");
   initSecurity ();
   expireDevices ();

   user.trim ();
   pw.trim ();   
   
   if (user.equalsIgnoreCase(EEData.username) && pw.equals (EEData.password))
   {
      Serial.printf ("User/Pw good\n");
      for (int j=0; j<MAXLOGINS; j++)
      {
         if (authenticated[j].device == 0)
         {
            authenticated[j].device = device;
            authenticated[j].expires = time (NULL) + 3*60;  // three minutes
            Serial.printf ("Logged in user %s\n", user.c_str());

            return true;
         }
      }
   }

   Serial.printf ("Rejected user %s\n", user.c_str());
   return false;
}

static void expireDevices (void)
{
   time_t current = time (NULL);
   for (int i=0; i<MAXLOGINS; i++)
   {
      if ((authenticated[i].device != 0) && (difftime (current, authenticated[i].expires) > 0))
      {
         Serial.println ("Expire " + authenticated[i].device);
         authenticated[i].device = 0;
      }
   }
}

void dump (void *pkt, size_t len)
{
   uint8_t *p = (uint8_t *) pkt;
   for (size_t i=0; i<len; i++)
      Serial.printf ("%02x ", p[i]);
}


static u32_t getClientIp (AsyncWebServerRequest *req)
{
    return req->client()->getRemoteAddress();
}

#if defined (ESP32)
uint64_t clientAddress (AsyncWebServerRequest *req)
{
   uint64_t address = 0;
   long clientIP = getClientIp (req);

   wifi_sta_list_t wifi_sta_list;
   wifi_sta_mac_ip_list_t adapter_sta_list;
   
   memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
   memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));

   esp_wifi_ap_get_sta_list (&wifi_sta_list);
   esp_wifi_ap_get_sta_list_with_ip (&wifi_sta_list, &adapter_sta_list);
   
   for (int i = 0; i < adapter_sta_list.num; i++) 
   {
   	esp_netif_pair_mac_ip_t station = adapter_sta_list.sta[i];
//    Serial.printf("%d - mac: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x - IP: \n", i,
//		station.mac[0], station.mac[1], station.mac[2],
//		station.mac[3], station.mac[4], station.mac[5]);
//      Serial.printf ("%lx vs %lx\n", clientIP, station.ip.addr);

      if (clientIP == station.ip.addr)
         address = mac2DeviceID (station.mac);
   }

   if (address == 0)
      address = ip2DeviceID (IPAddress ((uint8_t*)&clientIP));
      
   return address;
}

#elif defined (ESP8266)
uint64_t clientAddress (AsyncWebServerRequest *req)
{
   uint64_t address = 0;
   u32_t clientIP = getClientIp (req);

   struct station_info *station_list = wifi_softap_get_station_info();
   while (station_list != NULL) 
   {
      if (clientIP == station_list->ip.addr)
         address = mac2DeviceID (station_list->bssid);

      station_list = STAILQ_NEXT(station_list, next);
   }
   wifi_softap_free_station_info();

   if (address == 0)
      address = ip2DeviceID (IPAddress ((uint8_t*)&clientIP));
      
   return address;
}
#endif

static bool isMasterDevice (AsyncWebServerRequest *req)
{
   // Serial.printf ("isMasterDevice\n");
   // Serial.printf ("  %lx",  mac2DeviceID (EEData.masterDevice));
   return clientAddress (req) == mac2DeviceID (EEData.masterDevice);
}
