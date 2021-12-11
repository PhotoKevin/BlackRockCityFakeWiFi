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
