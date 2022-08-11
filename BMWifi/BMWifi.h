

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

extern char lastPageReq[512];
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


extern unsigned char GTCertificate_DER[];
extern unsigned char GTCertificateP_DER[];
extern unsigned int GTCertificate_DER_len;
extern unsigned int GTCertificateP_DER_len;

//extern const char *cert_pem;
//extern const char *key_pem;

extern const char bmwifi_gt_org_crt_pem[];
extern const char bmwifi_gt_org_key_pem[];


#if defined (_ESP_HTTPS_SERVER_H_)
   extern httpd_handle_t secure_http;
   extern httpd_handle_t insecure_http;
   long long clientAddress (httpd_req_t *req);
//   bool isMasterDevice (httpd_req_t *req);
   bool isLoggedIn (httpd_req_t *req);
   String getSettings (httpd_req_t *req);
#endif

String  WiFiStatus (int s);
void DisplayStatus (void);
void DisplayOLEDStatus (void);

void  setupWebServer (void);

void WriteEEData (int address, const void *data, size_t nbytes);
void ReadEEData (int address, void *data, size_t nbytes);
void SaveEEDataIfNeeded (int address, void *data, size_t nbytes);

void client_status (void);

void dump (void *pkt, size_t len);

String getSystemInformation (void);



bool Login (String user, String pw, long long device, String &token);

void expireBanned (void);
int banExpires (long long address);
void banDevice (long long address);
int isBanned (long long address);
void handleBlocked (void) ;


