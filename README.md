# BlueDisplay Library for Arduino

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Build Status](https://travis-ci.org/ArminJo/Arduino-BlueDisplay.svg?branch=master)](https://travis-ci.org/ArminJo/Arduino-BlueDisplay)


This library allows an Arduino board with Bluetooth module like HC-05 to connect to the BlueDisplay app on your smartphone.

## SUMMARY
Let the Arduino sketch create a GUI with Graphics, Buttons and Sliders on your smartphone by simply connecting a HC-05 to the rx/tx pins of your Arduino.
The App receives draw requests from Arduino over Bluetooth and renders it.
GUI callback, touch and sensor events are sent back to Arduino.
No Android programming needed!

## Download
The actual version can be downloaded directly from GitHub [here](https://github.com/ArminJo/Arduino-BlueDisplay/blob/master/extras/BlueDisplay.zip?raw=true)

# Installation
Install this "BlueDisplay" library with *Tools -> Manage Libraries...* or *Ctrl+Shift+I*. Use "BlueDisplay" as filter string.
Or download BlueDisplay.zip file or use the GitHub *clone or download -> Download ZIP* button, and add the .zip file with *Sketch -> Include Library -> add .ZIP Library...*.  

## Features
- Graphic + text output as well as printf implementation.
- Draw chart from byte or short values. Enables clearing of last drawn chart.
- Play system tones.
- Touch button + slider objects with tone feedback.
- Button and slider callback as well as touch and sensor events are sent back to Arduino.
- Automatic and manually scaling of display region.
- Easy mapping of UTF-8 characters like Ohm, Celsius etc..
- Up to 115200 Baud using HC-05 modules.
- Local display of received and sent commands for debug purposes.
- Hex und ASCII output of received Bluetooth data at log level verbose.
- Debug messages as toasts.

## Examples
- BlueDisplayBlink - Simple example to check your installation.
- BlueDisplayExample - More elaborated example to shoe more features of the BlueDisplay library.
- HC_05_Initialization - Simple helper program to reconfigure your HC-05 module.
- RcCarControl - Example of controlling a RC-car by smartphone accelerometer sensor 
- SimpleTouchScreenDSO - 300 kSamples DSO without external hardware (except the HC-05 module). For AC input, only a capacitor and 4 resistors are needed.
More information under [Arduino-Simple-DSO](https://github.com/ArminJo/Arduino-Simple-DSO)
- US_Distance - Shows the distances measured by a HC-SR04 ultrasonic sensor. Can be used as a parking assistance.

## Extras
The extras folder (in the Arduino IDE use "Sketch/Show Sketch Folder" (or Ctrl+K) and then in the libraries/BlueDisplay/extras directory) 
contains more schematics, breadboard layouts and pictures which may help you building the example projects.

## Hints
If you need debugging with print() you must use the debug() functions since using Serial.print() etc. gives errors (we have only one serial port on the Arduino) . E.g.
```
BlueDisplay1.debug("\r\nDoBlink=", (uint8_t) doBlink);
```

To enable programming of the Arduino while the HC-05 module is connected, use a diode (eg. a BAT 42) to connect Arduino rx and HC-05 tx.
On Arduino MEGA 2560, TX1 is used, so no diode is needed.
```
                 |\ |
   Arduino-rx ___| \|___ HC-05-tx
                 | /|
                 |/ |
```


BlueDisplay example breadboard picture
![Breadboard picture](https://github.com/ArminJo/android-blue-display/blob/gh-pages/pictures/Blink1.jpg)
Fritzing schematic for BlueDisplay example
![Fritzing schematics](https://github.com/ArminJo/Arduino-BlueDisplay/blob/master/extras/BlueDisplayBlink_Steckplatine.png)
DSO with passive attenuator on breadboard
![DSO with passive attenuator on breadboard](https://github.com/ArminJo/android-blue-display/blob/gh-pages/pictures/ArduinoDSO.jpg)
At work
![DSO at work](https://github.com/ArminJo/android-blue-display/blob/gh-pages/pictures/DSO+Tablet.jpg)
Fritzing
![DSO Fritzing](https://github.com/ArminJo/Arduino-BlueDisplay/blob/master/extras/Arduino_Nano_DSO_Steckplatine.png)
Schematic
![DSO Schematic](https://github.com/ArminJo/Arduino-BlueDisplay/blob/master/extras/Arduino_Nano_DSO_Schaltplan.png)
DSO settings menu
![DSO settings menu](https://github.com/ArminJo/android-blue-display/blob/gh-pages/screenshots/DSOSettings.png)
DSO frequency generator menu
![Frequency generator menu](https://github.com/ArminJo/android-blue-display/blob/gh-pages/screenshots/Frequency.png)
Hacked RC car
![Hacked RC car](https://github.com/ArminJo/android-blue-display/blob/gh-pages/pictures/RCCar+Tablet.jpg)

RC car control display
![RC car control display](https://github.com/ArminJo/Arduino-BlueDisplay/blob/master/extras/RCCarControl.png)

