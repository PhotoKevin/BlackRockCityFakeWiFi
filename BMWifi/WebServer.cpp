#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Note: https://github.com/esp8266/Arduino/commit/9e82bf7c8db6274afa297262e1e16e685b1d56f9

#include "BMWifi.h"


//static unsigned long force;
static char correctURL[44];



String processor (const String &var) 
{
   Serial.println ("HHH " + var);
   return String ();
}

static void send (const char *txt)
{
   Serial.println (server.uri());
   server.setContentLength (CONTENT_LENGTH_UNKNOWN);
   server.send (200, "text/html", "");
   server.sendContent (txt);
   yield ();
   
}

static void handlebrccss (void)
{
   Serial.print ("HandleBRC CSS");
   send (brc_css);
}

static void handleQuestionJson (void)
{
   send (questions_json);
}

static void handleQuestion (void)
{
   send (question_html);
}

static void handleLegal (void)
{
   send (legal_html);
}

static void handleBlocked (void)
{
   send (blocked_html);
}

static void handlequestionsjs (void)
{
   Serial.print ("HandleQuestionsJS");
   send (questions_js);
}

// https://www.esp8266.com/viewtopic.php?f=8&t=4307


#include <U8x8lib.h>



// https://techtutorialsx.com/2018/07/22/esp32-arduino-http-server-template-processing/

// https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html

extern U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8;

static void redirectPage (void)
{
   Serial.printf ("redirecting %s\n", correctURL);
   client_status ();
   Serial.print ("WiFi.macAddress: ");
   Serial.println (WiFi.macAddress());

   Serial.println (server.uri());

   WiFiClient cli = server.client ();
//   Serial.println (cli.toString ());
   IPAddress clientIP = server.client().remoteIP();
   Serial.println ("Client IP: ");
   Serial.println (clientIP.toString ());

   char ip[30];
   clientIP.toString().toCharArray (ip, 16);
   u8x8.drawString (0, 1, ip);
//   u8x8.refreshDisplay();    // only required for SSD1606/7  

   //         auto station_ip = IPAddress((&station_list->ip)->addr).toString().c_str();
   
  server.setContentLength (CONTENT_LENGTH_UNKNOWN);
  server.send (200, "text/html", "");
  //server.sendContent (pageStart);
  
  String redirect = "<a href=\"";
  redirect += correctURL;
  redirect += "/legal.html";
  redirect += "\">Click here</a>";
  server.sendContent (redirect);
  server.sendContent (  "</body></html>");
}

void setupWebServer (void)
{
   IPAddress ip = WiFi.localIP ();
   
  if (ip[0] == 0)
    ip = WiFi.softAPIP ();

  
  sprintf (correctURL, "http://%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

  // Set up the endpoints for HTTP server
//  server.on ("/donation", HTTP_POST, handlePage);
//  server.on ("/donation", HTTP_GET,  handlePage);
  server.onNotFound (redirectPage);

  server.on ("/question.html", HTTP_GET,  handleQuestion);
  server.on ("/legal.html", HTTP_GET,  handleLegal);
  server.on ("/blocked.html", HTTP_GET,  handleBlocked);
  server.on ("/brc.css", HTTP_GET,  handlebrccss);
  server.on ("/getJson", HTTP_POST,  handleQuestionJson);
  server.on ("/questions.js", HTTP_GET,  handlequestionsjs);

  server.begin ();
  yield ();

  Serial.printf ("Listening at: %s\n", correctURL);
}
