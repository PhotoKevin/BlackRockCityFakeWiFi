#include <Arduino.h>
#include "BMWifi.h"
const char checkbox_css[] PROGMEM = 
"input[type='checkbox'] \n"
"{\n"
" -moz-appearance: none;\n"
" -webkit-appearance: none;\n"
" appearance: none;\n"
" vertical-align: middle;\n"
" outline: none;\n"
" font-size: inherit;\n"
" cursor: pointer;\n"
" width: 1.0em;\n"
" height: 1.0em;\n"
" background: white;\n"
" border-radius: 0.25em;\n"
" border: 0.125em solid #555;\n"
" top: -0.1em;\n"
" position: relative;\n"
"}\n"
"\n"
"input[type='checkbox']:checked \n"
"{\n"
" background: #adf;\n"
"}\n"
"\n"
"input[type='checkbox']:checked:after \n"
"{\n"
" content: \"\2714\";\n"
" position: absolute;\n"
" font-size: 90%;\n"
" left: 0.0625em;\n"
" top: -0.25em;\n"
"}"
;