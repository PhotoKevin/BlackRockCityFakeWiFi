

struct banned_t
{
   long long address;
   time_t timestamp;
};

struct eeprom_data_t
{
   int totalBanned;
   int totalRedirects;
   time_t lastActivity;
};
extern int EEChanged;

extern const char *myHostname;

extern struct eeprom_data_t  EEData;
const int EEDataAddr = 40;
extern int displayRows;
extern bool clockSet;

extern const char checkbox_css [];
extern const char brc_css [];
extern const char question_html[]; 
extern const char questions_js[];
extern const char legal_html[];
extern const char blocked_html[];
extern const char questions_json[];
extern const char debugdata_js[];
extern const char radio2_css[];
extern const char banned_js[];
extern const char banned_html[];
extern const char redirect_html[];
extern const char status_html[];
extern const char status_js[];

#if defined (ESP8266WEBSERVER_H)
  extern ESP8266WebServer server;
#endif

void DisplayStatus (void);
void DisplayOLEDStatus (void);

void  setupWebServer (void);

void WriteEEData (int address, const void *data, size_t nbytes);
void ReadEEData (int address, void *data, size_t nbytes);
void SaveEEDataIfNeeded (int address, void *data, size_t nbytes);

void client_status (void);

String getSystemInformation (void);

void expireBanned (void);
int banExpires (long long address);
void banDevice (long long address);
int isBanned (long long address);
void handleBlocked (void) ;
