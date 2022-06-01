#include <Arduino.h>
#include "BMWifi.h"

#define BAN_SECONDS (2 * 60.0)

#define NUM_BANNED 20
struct banned_t banned[NUM_BANNED];


void expireBanned ()
{
   time_t currentTime = time (NULL);
   for (int i=0; i<NUM_BANNED; i++)
   {
      if (banned[i].timestamp != (time_t) 0)
      {
         float secs = difftime (currentTime, banned[i].timestamp);
         if (secs > BAN_SECONDS)
         {
            Serial.printf ("Removing %llx\n", banned[i].address);
            banned[i].timestamp = 0;
            banned[i].address = 0;
         }
      }
   }
}

int banExpires (long long address)
{
   time_t currentTime = time (NULL);
   for (int i=0; i<NUM_BANNED; i++)
   {
      if ((banned[i].address == address) &&
          (banned[i].timestamp != (time_t) 0))
      {
         float secs = BAN_SECONDS - difftime (currentTime, banned[i].timestamp);
         Serial.printf ("Ban expires: %f\n", secs);
         return (int) secs/60.0 + 1;
      }
   }

   return 0;
}

void banDevice (long long address)
{
   Serial.printf ("Banning %llx\n", address);
   time_t currentTime = time (NULL);
   for (int i=0; i<NUM_BANNED; i++)
   {
      if (banned[i].timestamp == (time_t) 0)
      {
         banned[i].address = address;
         banned[i].timestamp = currentTime;
         break;
      }
   }

   /* If there were no empty slots, they get a free pass */
}

int isBanned (long long address)
{
   expireBanned ();
   for (int i=0; i<NUM_BANNED; i++)
   {
      if (banned[i].address == address)
         return 1;
   }

   return 0;   
}
