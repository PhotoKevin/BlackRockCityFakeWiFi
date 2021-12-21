#include <Arduino.h>
#include "BMWifi.h"
const char blocked_html[] PROGMEM = 
"<!DOCTYPE html>\n"
"<html lang=\"en\" dir=\"ltr\">\n"
"\n"
" <head>\n"
" <meta charset=\"utf-8\">\n"
" <title>BRC Open WiFi</title>\n"
" <link rel=\"stylesheet\" href=\"brc.css\">\n"
" </head>\n"
" <body>\n"
" <script src=\"questions.js\"></script>\n"
"\n"
" <div class=\"top centerHorizontal\" id=\"top\">\n"
" BRC Open WiFi\n"
" </div>\n"
" <div class=\"main centerHorizontal\" id=\"main\">\n"
" Thank you for showing that you are not a robot. Access to internet services is DENIED due to\n"
" violations of the Terms & Conditions which clearly listed \"Dorking about on the internet\" under\n"
" \"Examples of Unacceptable Uses\" as it is a violation of the principle of Immediacy.\n"
" </div>\n"
"\n"
" <div class=\"main centerHorizontal\" id=\"blocked\">\n"
" Your device ID has been added to the banned list.\n"
" </div>\n"
" </body>\n"
"</html>\n"
;
