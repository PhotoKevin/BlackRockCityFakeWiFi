#include <Arduino.h>
#include "BMWifi.h"

#define NUM_VISITORS 20
uint64_t visitors[NUM_VISITORS];
size_t head = 0;

int hasVisited (uint64_t deviceId)
{
   for (int i=0; i<NUM_VISITORS; i++)
      if (deviceId == visitors[i])
         return 1;

   return 0;
}

void visited  (uint64_t deviceId)
{
   visitors[head] = deviceId;
   head  += 1;
   if (head >= NUM_VISITORS)
      head = 0;
}
