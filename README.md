
  ## Note: There is an updated version of the Tally that supports changing the Vmix source IP and GUID via a Web UI, negating the need for reflashing. Until I can write a proper Readme, the same readme will be used for this branch as well.
  

# VmixToESPTally

  

A simple firmware for ESP8266 Boards to make a portable Tally Light System!

  

### What are Tally Lights?

  

Tally Lights are used in the broadcasting world to let Camera Operators know when their feed is currently on Program or Preview, giving them a heads up on when they're On Air, or about to be (On Preview).

  

### How does this Project work?

  

This project is meant to be flashed to an ESP8266 Board, with one board being used for each instance where a tally light is needed. (Multiple lights from One Board Coming Soon?)

  

The Board is assigned to an input number (or GUID), connects to WiFi, pings the Vmix API (found at https://[vmix_machine_ip]:8088/api) to see if the input number given is in the <preview> or <active> tag. If so, it sets the color of the LEDs connected to GPIO Pins.

  
  

## Features

  

- Allows Input Reference by Input Number *or* Input GUID

- Comprehensive(ish) Debugging WiFi Status and Current Preview & Program Numbers over Serial

- Allows assignment of the Board to a static IP

- View Tally Status using a 4 Pin RGB LED (Neopixel Version Coming Soon!)

  

## Config

  

The config.h file in src is used to set the;

- WiFi SSID

- WiFi Password

- Vmix Machine IP (With Port)

- Input Number OR GUID

- Pins used for LEDs (Wiring Diagram for Default Config is given below)

- (Optional) Static IP, Gateway, and Subnet for the board

  

### Should I use an Input Number or it's GUID. Also, what's a GUID

  

A GUID is a unique identifier for an input in Vmix. Rearranging inputs will not affect the GUID. Therefore, it's a reliable way to ensure that you always point to the correct Input.

  

To find an input's GUID, you can go to the Vmix API XML (Found at ```http://[Vmix_Machine_IP]:[Port]/api```), and find the GUID corresponding the Input you need.

  

You can use either the Input Number or GUID with no change to the code.

## Installation

  

### Method 1: Using PlatformIO in Visual Studio Code

  

- Clone the repo on your Machine with;

```bash

git  clone  https://github.com/Dilagante/vmix2ESPTally

```

- Connect your ESP8266 Board over USB

- In the PlatformIO Tab of VSCode, open the folder containing the platformio.ini file

- Make any changes necessary in the config.h file (Vmix Machine IP, WiFi Credentials, etc.)

- Upload to your Board!

  

## Default Wiring Diagram

  

![Wiring Diagram](https://github.com/Dilagante/vmix2ESPTally/blob/master/Tally_Client_Schematic_v2.png)

  

## What's next?

  

- ESP32 Support (May work, currently untested)

- Separate version with Neopixel support (A single data pin to control an LED Strip like WS2812)

- Control multiple light strips from one board (Unpractical unless Cameras/Feeds are right next to each other but let's see)

  

## Libraries Used

  

- [ESP8266WiFi.h](https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/)

- [ESP8266HTTPClient.h](https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/)

- [ESP8266WebServer.h](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/)

- [EEPROM.h](https://github.com/esp8266/Arduino/tree/master/libraries/EEPROM/)

  
  

## Readme created in https://readme.so

  

## License

  

[MIT License](https://choosealicense.com/licenses/mit/)

  

Copyright© [2025] [Dilagante]

  

Permission is hereby granted, free of charge, to any person obtaining a copy

of this software and associated documentation files (the "Software"), to deal

in the Software without restriction, including without limitation the rights

to use, copy, modify, merge, publish, distribute, sublicense, and/or sell

copies of the Software, and to permit persons to whom the Software is

furnished to do so, subject to the following conditions:

  

The above copyright notice and this permission notice shall be included in all

copies or substantial portions of the Software.

  

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR

IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,

FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE

AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER

LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,

OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE

SOFTWARE.

  
  
  

![My cat](https://github.com/Dilagante/vmix2ESPTally/blob/master/Cat.jpg)
