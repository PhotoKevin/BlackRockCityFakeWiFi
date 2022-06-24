# BlackRockCityFakeWiFi

## Purpose
Create a joke WiFi access point for Black Rock City (aka Burning Man). The person trying to use the access point gets a huge page of Terms and Conditions. Once they agree, they are run through several sets of questions to "Prove they're not a robot". 

Finally they get told that "Dorking around on the Internet" is against the principle of Immediacy and are dumped.


## Development Environment
I developed this on both a Heltec WiFi Kit 8 and Wemos D1 mini using the Arduino IDE, but it should work on any generic ESP-8266 board that is supported by Arduino. The Heltec board is nice in that it has a built in display and can run
off of either USB or a LiPo battery and can charge the battery from USB. Apparently Heltec doesn't  make
it anymore but clones can be purchased from eBay or AliExpress.

The web pages were created with the [Atom](https://atom.io) editor. A small C program is then used to convert the
pages into data stored in the main flash memory. The C program was developed in Visual Studio 2019.

[Wemos D1 mini v3.0](http://www.wemos.cc)

[WiFi Kit 8](https://heltec.org/project/wifi-kit-8/)


## Captive Portal
The code implements a Captive Portal which operates like the typical free WiFi you'd find at a resturant.
When the user connects it grabs all traffic from that user regardless of what they asked for and redirects it 
to its signon process. I owe a lot to the [Mobile Rick Roll](https://github.com/idolpx/mobile-rr) project for their work in figuring out how to 
implement this.


## Building the project
I'm using the Arduino 2.0 IDE. It's still in beta (2022-06) but has been working well.

- After installing the IDE go to the preferences (Control-Comma) and add an Additional Board Manager URL of https://arduino.esp8266.com/stable/package_esp8266com_index.json


- Exit and restart the IDE.

- In the Boards Manager (left column, second icon from the top) find "esp8266 by ESP8266 Community" and install it.

- Now go to the Library Manager (left column, third icon from the top) and install "ArduionJson by Benoit Blanchon".
If you are using the WiFi Kit 8 board with an LCD display, you will also want to install "U8g2 by oliver"

- Plug your ESP board into a USB port.

- Click on the drop down in the button bar. Find "Generic ESP8266 Module" and click it. Also select the COM port for the board.

- Lastly click on the Upload (right arrow) button to compile and install the software.

## Accessing the Configuration Page
This sounds more complicated than it really is. The overall flow is to connect to the access point, then go to a hidden login page. Then go to the settings.

- Plug the Ardunio board into power and get out your phone.

- Disable data access.
- Find the access point and connect to it.
At this point you will get a typical pop up saying you need to log into the access point. 
- Ignoring the login prompt, open your browser and try to open a page such as Bing or Google. The access point will capture that request and send you to a giant page of legalese. 
- Look at the address bar. It should say something like http://10.47.4.7/legal.html. Change that be login.html leaving the rest of it alone (e.g. http://10.47.4.7/login.html)
- Now you'll have a username/login prompt page. The default credentials are admin/***REMOVED*** (both are all lower case). Entering those will bring up the status page. 
- Lastly click the Settings button at the bottom of the page.
You can now make your changes.

## Configuration Options
- SSID - Name of the Access Point that shows up on your phone.
- Username - Username for accessing the statistics and settings.
- Password - Password for same. If left blank the previous password will remain in effect. 
- IP Address - The IP Address for the Access Point.
- Netmask - The Netmask for the Access Point. You almost certainly should not change this.
- Master Device - The MAC address of a master device (aka your phone) that will be automatically logged in and sent to the status page.
- Current Device - The MAC address of the device you're currently using. Displayed so you can copy it into the Master Device if you want.



