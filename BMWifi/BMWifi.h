
struct eeprom_data_t
{
  uint16_t MoneyNeeded;
};

extern struct eeprom_data_t  EEData;
const int EEDataAddr = 40;
extern int displayRows;

extern const char brc_css [];
extern const char question_html[]; 
extern const char questions_js[];
extern const char legal_html[];
extern const char blocked_html[];
extern const char questions_json[];

#if defined (ESP8266WEBSERVER_H)
  extern ESP8266WebServer server;
#endif

void DisplayStatus (void);

void  setupWebServer (void);

void WriteEEData (int address, const void *data, size_t nbytes);
void ReadEEData (int address, void *data, size_t nbytes);


void client_status (void);
