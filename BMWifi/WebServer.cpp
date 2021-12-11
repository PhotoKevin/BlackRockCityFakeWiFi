#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Note: https://github.com/esp8266/Arduino/commit/9e82bf7c8db6274afa297262e1e16e685b1d56f9

#include "BMWifi.h"

static void handlePage (void);
static void handleDebug (void);
static void handleKrell (void);
static void donationPage (void);
static void debugPage (void);
static void krellPage (void);
static void redirectPage (void);

static void handlequestionsjs (void);
static void handlebrccss (void);

static unsigned long force;
static char correctURL[44];


void setupWebServer (void)
{
   IPAddress ip = WiFi.localIP ();
   
  if (ip[0] == 0)
    ip = WiFi.softAPIP ();

  
  sprintf (correctURL, "http://%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

  // Set up the endpoints for HTTP server
  server.on ("/donation", HTTP_POST, handlePage);
  server.on ("/donation", HTTP_GET,  handlePage);
  server.onNotFound (redirectPage);

  server.on ("/brc.css", HTTP_GET,  handlebrccss);
  server.on ("/BRC.css", HTTP_GET,  handlebrccss);
  server.on ("/questions.js", HTTP_GET,  handlequestionsjs);

  server.on ("/debug", HTTP_GET,  handleDebug);
  
  server.on ("/krell", HTTP_GET, handleKrell);
  server.on ("/krell", HTTP_POST, handleKrell);
  server.begin ();
  yield ();

  Serial.printf ("Listening at: %s\n", correctURL);
}


static void handlebrccss (void)
{
   Serial.print ("HandleBRCCSS");
   server.setContentLength (CONTENT_LENGTH_UNKNOWN);
   server.send (200, "text/html", "");
   server.sendContent (brccss());
   yield ();
}

static void handlequestionsjs (void)
{
   Serial.print ("HandleQuestionsJS");
   server.setContentLength (CONTENT_LENGTH_UNKNOWN);
   server.send (200, "text/html", "");
   server.sendContent (questionsjs());
   yield ();
}


static void handlePage (void)
{
  int changed = 0;


  if (changed)
    WriteEEData (EEDataAddr, &EEData, sizeof EEData);

  donationPage ();
  yield ();
//  if (changed)
//    ThermometerRowScan ();

//  Thermometer ();
}


static void handleKrell (void)
{

   krellPage ();
}


static void handleDebug (void)
{
  Serial.print ("HandleDebug");

  if (server.hasArg ("force"))
  {
     force = strtoul (server.arg ("force").c_str (), NULL, 0);
     Serial.print (server.arg ("force").c_str ());
  }

  debugPage ();
}

const char pageStart[] PROGMEM =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\">"
"<html>"
"<head>"
  "<title>GTDonation</title>"
  "<style>"
    "input, select, textarea, button {font-family:inherit;font-size:inherit;}"
//    "input.checkbox  {width : 2em;height :2em;padding: 0px;margin: 0px;}"

"input[type='checkbox'] {"
"  -moz-appearance: none;"
"  -webkit-appearance: none;"
"  appearance: none;"
"  vertical-align: middle;"
"  outline: none;"
"  font-size: inherit;"
"  cursor: pointer;"
"  width: 1.0em;"
"  height: 1.0em;"
"  background: white;"
"  border-radius: 0.25em;"
"  border: 0.125em solid #555;"
"  position: relative;"
"}"

"input[type='checkbox']:checked {"
"  background: #adf;"
"}"

"input[type='checkbox']:checked:after {"
"  content: \"X\";"
"  position: absolute;"
"  font-size: 90%;"
"  left: 0.0625em;"
"  top: -0.25em;"
"}"
  "</style>"
"</head>"
"<body style=\"font-size:2em\">";

const char pageEnd[] PROGMEM =
  "<input type=\"submit\" value=\"submit\">"
  "</fieldset>"
  "</form>"
  "<a href=\"./donation\">Donation</a> <a href=\"./krell\">Krell</a>"
  "</body>"
  "</html>";


static void donationPage (void)
{
  String Str;

  Serial.println ("donationPage");
  server.setContentLength (CONTENT_LENGTH_UNKNOWN);
  server.send (200, "text/html", "");
   server.sendContent (q1());
}




static void donationPagex (void)
{
  String Str;

  Serial.println ("donationPage");
  server.setContentLength (CONTENT_LENGTH_UNKNOWN);
  server.send (200, "text/html", "");

  server.sendContent (pageStart);
  server.sendContent (  F ("<form action=\""));
  server.sendContent (    correctURL);
  server.sendContent (  "/donation\" method=\"post\">");
  server.sendContent (    F ("<fieldset><legend>GT:</legend>"));

  server.sendContent (      F ("Current: <input type=\"number\" step=\"1\" name=\"current\" value=\""));
//                            Str = "";  Str +=  EEData.MoneyGathered;  server.sendContent (Str);
  server.sendContent (      F ("\"><br/>"));
  server.sendContent (      F ("<br/>"));

  server.sendContent (      F ("Target: <input type=\"number\" step=\"1\" pattern=\"[0-9]*\" name=\"target\" value=\""));
//                           Str = "";  Str += EEData.MoneyNeeded;  server.sendContent (Str);
  server.sendContent (      F ("\">"));

  server.sendContent (      F ("Brightness (0-15): <input type=\"number\" step=\"1\" pattern=\"[0-9]*\" name=\"bright\" value=\""));
//                           Str = "";  Str += EEData.Brightness;  server.sendContent (Str);
  server.sendContent (      F ("\">"));

  server.sendContent (pageEnd);
}


static void krellPage (void)
{
  String Str;

  server.setContentLength (CONTENT_LENGTH_UNKNOWN);
  server.send (200, "text/html", "");

  server.sendContent (pageStart);

  server.sendContent (  F ("<form action=\"/krell\" method=\"post\">"));
  server.sendContent (    F ("<fieldset><legend>GT:</legend>"));
  server.sendContent (      F ("Min: <input type=\"number\" step=\"1\" pattern=\"[0-9]*\" name=\"minBrightness\" value=\""));
//                           Str = "";  Str += EEData.minBrightness;  server.sendContent (Str);
  server.sendContent (      F ("\">"));
  server.sendContent (      F ("<br/>"));
  server.sendContent (      F ("Max: <input type=\"number\" step=\"1\" name=\"maxBrightness\" value=\""));
                            Str = "";  //Str +=  EEData.maxBrightness;  server.sendContent (Str);
  server.sendContent (      F ("\"><br/>"));

  server.sendContent (      F ("Seconds: <input type=\"number\" step=\"1\" name=\"krellSeconds\" value=\""));
                            Str = "";  //Str +=  EEData.krellSeconds;  server.sendContent (Str);
  server.sendContent (      F ("\"><br/>"));

  server.sendContent (    F ("<input type=\"checkbox\" name=\"krell\" value=\"1\" id=\"krell\" "));
  server.sendContent (F (">"));
  server.sendContent (F ("<label for=\"krell\">Krell</label><br />"));


  server.sendContent (pageEnd);
}

static void debugPage (void)
{
  String Str;

  server.setContentLength (CONTENT_LENGTH_UNKNOWN);
  server.send (200, "text/html", "");

  server.sendContent (pageStart);

  server.sendContent (  F ("<form action=\"/debug\" method=\"get\">"));
  server.sendContent (  F ("Val: <input type=\"text\" name=\"force\" value=\""));
      Str = "0x";
      Str += String (force, HEX);
      server.sendContent (Str);
  server.sendContent (  F ("\">"));
  server.sendContent (    F ("<input type=\"checkbox\" name=\"krell\" value=\"1\" id=\"krell\" "));
  server.sendContent (F (">"));
  server.sendContent (F ("<label for=\"krell\">Krell</label><br />"));

  server.sendContent (pageEnd);
}

// https://techtutorialsx.com/2018/07/22/esp32-arduino-http-server-template-processing/

// https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html

static void redirectPage (void)
{
   Serial.printf ("redirecting %s\n", correctURL);
   client_status ();
   Serial.println (WiFi.macAddress());


   WiFiClient cli = server.client ();
//   Serial.println (cli.toString ());
   IPAddress clientIP = server.client().remoteIP();
   Serial.println (clientIP.toString ());

   //         auto station_ip = IPAddress((&station_list->ip)->addr).toString().c_str();



   
  server.setContentLength (CONTENT_LENGTH_UNKNOWN);
  server.send (200, "text/html", "");
  server.sendContent (pageStart);
  
  String redirect = "<a href=\"";
  redirect += correctURL;
  redirect += "/donation";
  redirect += "\">Click here</a>";
  server.sendContent (redirect);
  server.sendContent (  "</body></html>");
}
