#include <Arduino.h>
#include "BMWifi.h"
const char redirect_html[] PROGMEM = 
"<html>\n"
"\n"
"<head>\n"
" <meta charset=\"utf-8\">\n"
" <title>BRC Open WiFi</title>\n"
" <title>Network Authentication Required</title>\n"
" <meta http-equiv=\"refresh\" content=\"5; url=http://login.example.com/legal.html\">\n"
"</head>\n"
"\n"
"<body>\n"
" <p>You need to <a href=\"http://login.example.com/legal.html\"> authenticate with the local network</a> in order to gain access.</p>\n"
"</body>\n"
"\n"
"</html>\n"
;
