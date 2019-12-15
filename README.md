# [BlueDisplay](https://github.com/ArminJo/Arduino-BlueDisplay) Library for Arduino
### Version 1.0.2
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Installation instructions](https://www.ardu-badge.com/badge/Arduino-BlueDisplay.svg?)](https://www.ardu-badge.com/Arduino-BlueDisplay)
[![Build Status](https://travis-ci.org/ArminJo/Arduino-BlueDisplay.svg?branch=master)](https://travis-ci.org/ArminJo/Arduino-BlueDisplay)
[![Hit Counter](https://hitcounter.pythonanywhere.com/count/tag.svg?url=https%3A%2F%2Fgithub.com%2FArminJo%2FArduino-BlueDisplay)](https://github.com/brentvollebregt/hit-counter)

This library enables an Android smartphone / tablet to act as a graphical display for your Arduino.

## SUMMARY
Let your Arduino program create a GUI with **Graphics, Text, Buttons and Sliders** on your smartphone / tablet by simply
connecting a HC-05 to the rx/tx pins of your Arduino. 
Directly connecting the Arduino with an USB cable and an USB-OTG adapter to your smartphone is also supported.<br/>
The App receives draw requests from Arduino over Bluetooth and renders it.
GUI callback, touch and sensor events are sent back to the Arduino.
**No Android programming needed!**

# Installation
Install this "BlueDisplay" library with *Tools -> Manage Libraries...* or *Ctrl+Shift+I*. Use "BlueDisplay" as filter string.<br/>
Or download BlueDisplay.zip file or use the GitHub *clone or download -> Download ZIP* button, and add the .zip file with *Sketch -> Include Library -> add .ZIP Library...*.<br/>
On Android you need to install the [BlueDisplay app](https://play.google.com/store/apps/details?id=de.joachimsmeyer.android.bluedisplay).

## Features
- Graphic + text output as well as printf implementation.
- Draw chart from byte or short values. Enables clearing of last drawn chart.
- Play system tones.
- Touch button + slider objects with tone feedback.
- Button and slider callback as well as touch and sensor events are sent back to Arduino.
- Automatic and manually scaling of display region.
- Easy mapping of UTF-8 characters like Ohm, Celsius etc..
- Up to 115200 Baud using HC-05 modules.
- USB OTG connection can be used instead of Bluetooth.
- Local display of received and sent commands for debugging purposes.
- Hex and ASCII output of received Bluetooth data at log level verbose.
- Debug messages as toasts.

## Examples
Before using the examples, take care that the Bluetooth-module (e.g. the the HC-05 module) is connected to your Android device and is visible in the Bluetooth Settings.

All examples initially use the baudrate of 9600. Especially the SimpleTouchScreenDSO example will run smoother with a baudrate of 115200.<br/>
For this, change the example baudrate by deactivating the line `#define BLUETOOTH_BAUD_RATE BAUD_9600` and activating `#define BLUETOOTH_BAUD_RATE BAUD_115200`.<br/>
**AND** change the Bluetooth-module baudrate e.g. by using the BTModuleProgrammer.ino example.


- BTModuleProgrammer - Simple helper program to configure your HC-05 or JDY-31 modules name and default baudrate with a serial monitor.
- BlueDisplayBlink - Simple example to check your installation.
- BlueDisplayExample - More elaborated example to shoe more features of the BlueDisplay library.
- RcCarControl - Example of controlling a RC-car by smartphone accelerometer sensor
- **SimpleTouchScreenDSO** - 300 kSamples DSO without external hardware (except the HC-05 module). For AC input, only a capacitor and 4 resistors are needed.
More information at [Arduino-Simple-DSO](https://github.com/ArminJo/Arduino-BlueDisplay/tree/master/examples/SimpleTouchScreenDSO)
- US_Distance - Shows the distances measured by a HC-SR04 ultrasonic sensor. Can be used as a parking assistance.

## Extras
The extras folder (in the Arduino IDE use "Sketch/Show Sketch Folder" (or Ctrl+K) and then in the libraries/BlueDisplay/extras directory)
contains more schematics, breadboard layouts and pictures which may help you building the example projects.

## Hints
If you need debugging you must use the `debug()` functions since using `Serial.print()` etc. gives errors (we have only one serial port on the Arduino) . E.g.
```
BlueDisplay1.debug("DoBlink=", doBlink);
```
The debug content will then show up as toast on your Android device and is stored in the log like other commands received by the app.
Change the **log level** in the app to see the information you need of the BlueDisplay communication.

To enable programming of the Arduino while the HC-05 module is connected, use a diode (eg. a BAT 42) to connect Arduino rx and HC-05 tx.
On Arduino MEGA 2560, TX1 is used, so no diode is needed.
```
                 |\ |
   Arduino-rx ___| \|___ HC-05-tx
                 | /|
                 |/ |
```

| Fritzing schematic for BlueDisplay example | BlueDisplay example breadboard picture |
| :-: | :-: |
| ![Fritzing schematics](https://github.com/ArminJo/Arduino-BlueDisplay/blob/master/extras/BlueDisplayBlink_Steckplatine.png) | ![Breadboard picture](https://github.com/ArminJo/android-blue-display/blob/gh-pages/pictures/Blink1.jpg) |
| RC car control display | Hacked RC car |
| ![RC car control display](https://github.com/ArminJo/Arduino-BlueDisplay/blob/master/extras/RCCarControl.png) | ![Hacked RC car](https://github.com/ArminJo/android-blue-display/blob/gh-pages/pictures/RCCar+Tablet.jpg) |

# Revision History
### Version 1.0.2
- Porting to STM32.

### Version 1.0.1
- Changed default baud rate for all examples to 9600.
- Renamed `USART_send()` to `sendUSART()`.
- DSO example Version 3.1.

### Version 1.0.0
Initial Arduino library version

# Revision History corresponding to the Android BlueDisplay App
### V 3.7
Handling of no input for getNumber.
Slider setScaleFactor() does not scale the actual value, mostly delivered as initial value at init().
### V 3.6
connect, reconnect and autoconnect improved/added. Improved debug() command. Simplified Red/Green button handling.
### V 3.5
Slider scaling changed and unit value added.
### V 3.4
Timeout for data messages. Get number initial value fixed.
Bug autorepeat button in conjunction with UseUpEventForButtons fixed.
### V 3.3
Fixed silent tone bug for Android Lollipop and other bugs. Multiline text /r /n handling.
Android time accessible on Arduino. Debug messages as toasts. Changed create button.
Slider values scalable. GUI multi touch.Hex and ASCII output of received Bluetooth data at log level verbose.
### V 3.2
Improved tone and fullscreen handling. Internal refactoring. Bugfixes and minor improvements.
### V 3.1
Local display of received and sent commands for debug purposes.
### V 3.0
Android sensor accessible by Arduino.

## Travis CI
The NeoPatterns library examples are built on Travis CI for the following boards:

- Arduino Uno
- Arduino Leonardo
- Arduino cplayClassic
- Arduino STM32:stm32:GenF1:pnum=BLUEPILL_F103C8

## Requests for modifications / extensions
Please write me a PM including your motivation/problem if you need a modification or an extension.
