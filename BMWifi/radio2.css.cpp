#include <Arduino.h>
#include "BMWifi.h"
const char radio2_css[] PROGMEM = 
"/* https://stackoverflow.com/questions/4920281/how-to-change-the-size-of-the-radio-button-using-css */\n"
"\n"
"input[type=\"radio\"]\n"
"{\n"
" -webkit-appearance: none;\n"
" -moz-appearance: none;\n"
" appearance: none;\n"
"\n"
" border-radius: 50%;\n"
" font: inherit;\n"
" width: 1em;\n"
" height: 1em;\n"
"\n"
" border: 0.15em solid currentColor;\n"
" transition: 0.2s all linear;\n"
" position: relative;\n"
" top: 0.1em;\n"
"}\n"
"\n"
"input[type=\"radio\"]:checked \n"
"{\n"
" border: .4em solid black;\n"
" outline: unset !important /* I added this one for Edge (chromium) support */\n"
"}"
;