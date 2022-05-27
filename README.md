# BlackRockCityFakeWiFi

## Purpose
Create a joke WiFi access point for Black Rock City (aka Burning Man). The person trying to use the access point gets a huge 
page of Terms and Conditions. Once they agree, they are run through several sets of questions to "Prove their not a robot". 
Finally they get told that "Dorking around on the Internet" is against the principle of Immediacy and are dumped.


## Development Environment
I developed this on both a Heltec WiFi Kit 8 and Wemos D1 mini using the Arduino IDE, but it should work on any generic ESP-8266
board that is supported by Arduino. The Heltec board is nice in that it has a built in display and can run
off of either USB or a LiPo battery and can charge the battery from USB. Apparently Heltec doesn't  make
anymore but clones can be purchased from eBay or AliExpress.

The web pages were created with the [Atom](https://atom.io) editor. A small C program is then used to convert the
pages into data stored in the main flash memory. The C program was developed in Visual Studio 2019.

[Wemos D1 mini v3.0](http://www.wemos.cc)

[WiFi Kit 8](https://heltec.org/project/wifi-kit-8/)


## Captive Portal
The code implements a Captive Portal which operates like the typical free WiFi you'd find at a resturant.
When the user connects it grabs all traffic from that user regardless of what they asked for and redirects it 
to its signon process. I owe a lot to the [Mobile Rick Roll](https://github.com/idolpx/mobile-rr) project for their work in figuring out how to 
implement this.

