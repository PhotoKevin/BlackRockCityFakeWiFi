#include <Arduino.h>
#include <EEPROM.h>
#include "BMWifi.h"


void WriteEEData (int address, const void *data, size_t nbytes)
{
  size_t    i;
  uint8_t   by;
  uint8_t   *p = (uint8_t *) data;

  for (i=0; i<nbytes; i++)
  {
    EEPROM.write (address+i, p[i]);
    yield();
  }
  EEPROM.commit ();
}

void ReadEEData (int address, void *data, size_t nbytes)
{
  size_t    i;
  uint8_t   *p = (uint8_t *) data;

  for (i=0; i<nbytes; i++)
    p[i] = EEPROM.read (address+i);
}

static time_t lastSave = 0;
void SaveEEDataIfNeeded (int address, void *data, size_t nbytes)
{
   time_t now = time (NULL);
   float secs = difftime (lastSave, now);
   if (secs > 60 && EEChanged)
   {
      Serial.println ("Saving EEData");
      WriteEEData (address, data, nbytes);
      lastSave = now;
      EEChanged = 0;
   }
}
