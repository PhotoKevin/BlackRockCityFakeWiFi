#include <Arduino.h>
#include <string.h>
#include <time.h>

#if defined (ESP32)
   #include <esp_https_server.h>
   #include "esp_wifi.h"
#else
   #error Change your board type to an ESP32
#endif


#include "BMWifi.h"

#define MAXLOGINS 5

struct authenticated_t
{
   long long device;
   time_t expires;
};
struct  authenticated_t authenticated[MAXLOGINS];
static void expireDevices (void);
static bool isMasterDevice (httpd_req_t *req);


static long long mac2DeviceID (uint8_t *mac)
{
   long long address = 0;

   for (int i=0; i<6; i++)
   {
      address <<= 8;
      address += mac[i];
   }

   return address;
}

static long long ip2DeviceID (IPAddress ip)
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
         authenticated[i].device = 0;
      initialized = true;
   }
}


bool isLoggedIn (httpd_req_t *req)
{
   Serial.printf ("isLoggedIn?\n");
   initSecurity ();
   expireDevices ();

   if (isMasterDevice (req))
      return true;

   long long device = clientAddress (req);

   Serial.printf ("  <-- %llx\n", device);
   for (int i=0; i<MAXLOGINS; i++)
   {
      if (authenticated[i].device != 0)
      {
         if (device == authenticated[i].device)
         {
            Serial.println ("  --> Yes, logged in");
            return true;
         }
      }
   }

   Serial.println (" --> Not logged in");
   return false;
}

bool Login (String user, String pw, long long device)
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


static long getClientIp (httpd_req_t *req)
{
   int socket = httpd_req_to_sockfd (req);
   struct sockaddr_in6 name;
   socklen_t namelen = sizeof name;

   int rc = lwip_getpeername (socket, (struct sockaddr *) &name, &namelen);
   if (rc == 0)
   {
      if (name.sin6_family == AF_INET)
      {
         struct sockaddr_in   *addr_in = (struct sockaddr_in *) &name;
         in_addr_t ip_address = addr_in->sin_addr.s_addr;

         return (long) ip_address;

      }
      else if (name.sin6_family == AF_INET6)
      {
        
         // RFC4291 2.5.5.2.  IPv4-Mapped IPv6 Address
         // ::ffff:x:x:x:x is an ipv4 mapped into ipv6
         // As of 2022/07/30 that's the only type of IPv6 that the 
         // SDK returns.

         struct sockaddr_in6   *addr_in = (struct sockaddr_in6 *) &name;
         // The Arduino compiler chokes if you do this as one statement, so break it 
         // into two.
         in6_addr x = addr_in->sin6_addr;
         long ipv4 = x.un.u32_addr[3];
         return ipv4;
      }
   }

   return 0;
}

long long clientAddress (httpd_req_t *req)
{
   long long address = 0;
   long clientIP = getClientIp (req);

   wifi_sta_list_t wifi_sta_list;
   tcpip_adapter_sta_list_t adapter_sta_list;
   
   memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
	memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));

   esp_wifi_ap_get_sta_list (&wifi_sta_list);
   tcpip_adapter_get_sta_list (&wifi_sta_list, &adapter_sta_list);
   
   for (int i = 0; i < adapter_sta_list.num; i++) 
   {
   	tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
//    Serial.printf("%d - mac: %.2x:%.2x:%.2x:%.2x:%.2x:%.2x - IP: \n", i,
//		station.mac[0], station.mac[1], station.mac[2],
//		station.mac[3], station.mac[4], station.mac[5]);
//      Serial.printf ("%lx vs %lx\n", clientIP, station.ip.addr);

      if (clientIP == station.ip.addr)
         address = mac2DeviceID (station.mac);
   }


   if (address == 0)
      address = ip2DeviceID (clientIP);
      
   return address;
}

static bool isMasterDevice (httpd_req_t *req)
{
   // Serial.printf ("isMasterDevice\n");
   // Serial.printf ("  %lx",  mac2DeviceID (EEData.masterDevice));
   return clientAddress (req) == mac2DeviceID (EEData.masterDevice);
}
