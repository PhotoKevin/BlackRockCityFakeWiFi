#include <Arduino.h>
static const char brc_css [] PROGMEM = ".top"
"{"
" font-size: 2em;"
" font-family: Helvetica Neue, Helvetica, Arial, sans-serif;"
" text-align: center;"
" // background: #7a7467;"
" background: #a9a395;"
" // background: #4e493d;"
" padding-bottom: 0.5em;"
" padding-top: 0.5em;"
"}"
""
".normal"
"{"
" font-size: 1.5em;"
" font-family: Helvetica Neue, Helvetica, Arial, sans-serif;"
"}"
""
".main"
"{"
" margin-top: 1em;"
" margin-bottom: 0.0em;"
"// font-size: 1.5em;"
" font-family: Helvetica Neue, Helvetica, Arial, sans-serif;"
"}"
""
"body"
"{"
" width: 750px;"
" font-size: 1.5em;"
" font-family: Helvetica Neue, Helvetica, Arial, sans-serif;"
"}"
""
".answers"
"{"
" margin-top: 1em;"
" // padding-top: 2em;"
" margin-left: 5%;"
" align: center;"
""
"}"
"";

const char *brccss ()
{
   return brc_css;
}
