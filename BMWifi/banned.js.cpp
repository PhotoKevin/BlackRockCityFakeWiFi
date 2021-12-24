#include <Arduino.h>
#include "BMWifi.h"
const char banned_js[] PROGMEM = 
" const xhr = new XMLHttpRequest();\n"
"\n"
" function expirationHandler ()\n"
" {\n"
" if (xhr.readyState === XMLHttpRequest.DONE)\n"
" {\n"
" if (xhr.status === 200)\n"
" {\n"
" let data = JSON.parse (xhr.responseText);\n"
" expire.innerHTML = data.expire;\n"
" }\n"
" else\n"
" {\n"
" //alert('There was a problem with the request: ' + xhr.status);\n"
" }\n"
" }\n"
" }\n"
"\n"
"\n"
" function getExpirationTime ()\n"
" {\n"
" const url = 'getJson';\n"
" var formData = new FormData();\n"
" formData.append (\"request\", \"expire\");\n"
"\n"
" xhr.open ('POST', url, true);\n"
" xhr.onreadystatechange = expirationHandler;\n"
" xhr.send (formData);\n"
" }\n"
"\n"
;
