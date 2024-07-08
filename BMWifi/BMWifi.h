

struct banned_t
{
   uint64_t address;
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
   int unknownCount;
   char SSID[33];    // IEEE 802.11 defines a max SSID of 32 bytes
   char hostname[33];
   char username[33];
   char password[33];
   uint8_t masterDevice[6];
   uint8_t ipAddress[4];
   uint8_t netmask[4];
   uint8_t playSound;
   char lastUnknownUserAgent[200];
};

extern char lastPageReq[512];
extern int EEChanged;
extern int RestartRequired;

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

#if defined (_ESPAsyncWebSrv_H_) || defined (_ESPAsyncWebServer_H_)
   uint64_t clientAddress (AsyncWebServerRequest *req);
   bool isLoggedIn (AsyncWebServerRequest *req);
   String getSettings (AsyncWebServerRequest *req);
#endif

String  WiFiStatus (int s);
void DisplayStatus (void);
void DisplayOLEDStatus (void);

void  setupWebServer (void);

void WriteEEData (int address, const void *data, size_t nbytes);
void ReadEEData (int address, void *data, size_t nbytes);
void SaveEEDataIfNeeded (int address, void *data, size_t nbytes);

void client_status (void);
const char *localIP (void);
void str_copy (char *dest, const char *src, size_t len);

//void dump (void *pkt, size_t len);

String getSystemInformation (void);
String getTitle (void);
void strtrim (char *str);

bool Login (String user, String pw, uint64_t device);

void expireBanned (void);
int banExpires (uint64_t address);
void banDevice (uint64_t address);
int isBanned (uint64_t address);


// Visitors.cpp
int hasVisited (uint64_t deviceId);
void visited  (uint64_t deviceId);

