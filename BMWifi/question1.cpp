#include <Arduino.h>
#include "BMWifi.h"
static const char question1_html[] PROGMEM = "</<!DOCTYPE html>"
"<html lang=\"en\" dir=\"ltr\">"
""
" <head>"
" <meta charset=\"utf-8\">"
" <title>BRC Open Wi Fi</title>"
" <link rel=\"stylesheet\" type=\"text/css\" href=\"BRC.css\" />"
" <script type=\"text/javascript\" src=\"questions.js\"></script>"
" <script type=\"text/javascript\">"
" var ans ="
" ["
" {correct: true, text : \"Qu1\"},"
" {correct: false, text : \"Qu2\"},"
" {correct: false, text : \"Qu3\"},"
" ];"
" function x()"
" {"
" answer1.innerHTML = ans[0].text;"
" option1.innerText = \"yy\";"
" o2.style.background = \"red\";"
" }"
" </script>"
""
""
" </head>"
""
" <body>"
""
" <div class=\"top\">"
" BRC Open WiFi"
" </div>"
" <div class=\"main\">"
" You need to demonstrate that you are a burner and not a robot."
" Which of the following is not one of the Ten Principles?"
" <div class=\"answers\">"
" <div style=\"background: #7a7467\" id=\"o1\">"
" <input type=\"radio\" id=\"option1\" name=\"answer\" value=\"1\" data-correct=\"1\" onclick=\"selectionChanged()\"/>"
" <label id=\"answer1\" for=\"option1\">Radical Self Entitlement</label>"
" </div>"
" <div style=\"background: #a9a395\" id=\"o2\">"
" <input type=\"radio\" id=\"option2\" name=\"answer\" value=\"2\" onclick=\"selectionChanged()\"/>"
" <label for=\"option2\">Radical Inclusion</label>"
" </div>"
" <div style=\"background: #4e493d\" id=\"o3\">"
" <input type=\"radio\" id=\"option3\" name=\"answer\" value=\"3\" onclick=\"selectionChanged()\"/>"
" <label for=\"option3\">Immediacy</label>"
" </div>"
" </div>"
" <div class=\"main\">"
" <button class=\"normal\" id=\"answer\" onclick=\"answerClick('question2.html')\" disabled=\"true\" value=\"Sub\">Answer</button> <span id=\"solution\" class=\"normal\"></span>"
" </div>"
"<button onclick=\"x()\">foo</button>"
" </div>"
" </body>"
"</html>"
"";


const char *q1 ()
{
   return question1_html;
}
