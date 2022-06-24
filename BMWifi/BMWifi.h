

struct banned_t
{
   long long address;
   time_t timestamp;
};

struct eeprom_data_t
{
   uint eepromDataSize;
   int totalBanned;
   int legalShown; // Counts the number of times the legalese has been sent.
   int legalAccepted; // Number of times people have accepted the legalese
   time_t lastActivity;
   int androidCount;
   int iPhoneCount;
   char SSID[33];    // IEEE 802.11 defines a max SSID of 32 bytes
   char hostname[33];
   char username[33];
   char password[33];
   uint8_t masterDevice[6];
   uint8_t ipAddress[4];
   uint8_t netmask[4];
   uint8_t playSound;
};

extern int EEChanged;

extern const char *myHostname;

extern struct eeprom_data_t  EEData;
const int EEDataAddr = 0;
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
extern const char bmwifi_js[];

extern const char settings_html[];
extern const char settings_js[];
extern const char login_html[];

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
String getSettings (void);

bool isMasterDevice (void);
bool isLoggedIn (long long device, const String cookie);
bool Login (String user, String pw, long long device, String &token);

void expireBanned (void);
int banExpires (long long address);
void banDevice (long long address);
int isBanned (long long address);
void handleBlocked (void) ;

long long clientAddress (void);
