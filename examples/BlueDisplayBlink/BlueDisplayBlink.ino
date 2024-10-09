/*
 * BlueDisplayBlink.cpp
 *
 *  Demo of using the BlueDisplay library for HC-05 on Arduino
 *
 *  Copyright (C) 2014-2023  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay https://github.com/ArminJo/Arduino-BlueDisplay.
 *
 *  BlueDisplay is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#include <Arduino.h>

#define DISPLAY_WIDTH  DISPLAY_HALF_VGA_WIDTH  // 320
#define DISPLAY_HEIGHT DISPLAY_HALF_VGA_HEIGHT // 240

/*
 * Settings to configure the BlueDisplay library and to reduce its size
 */
//#define BLUETOOTH_BAUD_RATE BAUD_115200   // Activate this, if you have reprogrammed the HC05 module for 115200, otherwise 9600 is used as baud rate
#define DO_NOT_NEED_BASIC_TOUCH_EVENTS    // Disables basic touch events like down, move and up. Saves 620 bytes program memory and 36 bytes RAM
//#define USE_SIMPLE_SERIAL                 // Do not use the Serial object. Saves up to 1250 bytes program memory and 185 bytes RAM, if Serial is not used otherwise
//#define USE_USB_SERIAL                    // Activate it, if you want to force using Serial instead of Serial1 for direct USB cable connection* to your smartphone / tablet.
#include "BlueDisplay.hpp"

bool doBlink = true;

/*
 * The Start Stop button
 */
BDButton TouchButtonBlinkStartStop;

// Touch handler for buttons
void doBlinkStartStop(BDButton *aTheTouchedButton, int16_t aValue);

// Callback handler for (re)connect and resize
void initDisplay(void);
void drawGui(void);

// PROGMEM messages sent by BlueDisplay1.debug() are truncated to 32 characters :-(, so must use RAM here
const char StartMessage[] = "START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY;

/*******************************************************************************************
 * Program code starts here
 *******************************************************************************************/

void setup() {
    // Initialize the LED pin as output.
    pinMode(LED_BUILTIN, OUTPUT);

#if defined(ESP32)
    Serial.begin(115200);
    Serial.println(StartMessage);
    initSerial("ESP-BD_Example");
    Serial.println("Start ESP32 BT-client with name \"ESP-BD_Example\"");
#else
    initSerial();
#endif

    /*
     * Register callback handler and check for connection still established.
     * For ESP32 and after power on at other platforms, Bluetooth is just enabled here,
     * but the android app is not manually (re)connected to us, so we are definitely not connected here!
     * In this case, the periodic call of checkAndHandleEvents() in the main loop catches the connection build up message
     * from the android app at the time of manual (re)connection and in turn calls the initDisplay() and drawGui() functions.
     */
    BlueDisplay1.initCommunication(&initDisplay, &drawGui);

#if defined(USE_SERIAL1) || defined(ESP32) // USE_SERIAL1 may be defined in BlueSerial.h
// Serial(0) is available for Serial.print output.
#  if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/ \
    || defined(SERIALUSB_PID)  || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#  endif
// Just to know which program is running on my Arduino
    Serial.println(StartMessage);
#elif !defined(USE_SIMPLE_SERIAL)
    // If using simple serial on first USART we cannot use Serial.print, since this uses the same interrupt vector as simple serial.
    if (!BlueDisplay1.isConnectionEstablished()) {
#  if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/ \
    || defined(SERIALUSB_PID)  || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_attiny3217)
        delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#  endif
        // If connection is enabled, this message was already sent as BlueDisplay1.debug()
        Serial.println(StartMessage);
    }
#endif
}

void loop() {

    /*
     * This debug output can also be recognized at the Arduino Serial Monitor
     */
//    BlueDisplay1.debug("\r\nDoBlink=", (uint8_t) doBlink);
    if (doBlink) {
        // LED on
        digitalWrite(LED_BUILTIN, HIGH);
        BlueDisplay1.fillCircle(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, 20, COLOR16_RED);
        delayMillisWithCheckAndHandleEvents(300);
    }

    if (doBlink) {
        // LED off
        digitalWrite(LED_BUILTIN, LOW);
        BlueDisplay1.fillCircle(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, 20, COLOR16_BLUE);
        delayMillisWithCheckAndHandleEvents(300);
    }

    // To get blink enable event
    checkAndHandleEvents();
}

/*
 * Function used as callback handler for connect too
 */
void initDisplay(void) {
    // Initialize display size and flags
    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE | BD_FLAG_TOUCH_BASIC_DISABLE, DISPLAY_WIDTH,
    DISPLAY_HEIGHT);
    // Initialize button position, size, colors etc.
    TouchButtonBlinkStartStop.init((DISPLAY_WIDTH - BUTTON_WIDTH_2) / 2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_2,
    BUTTON_HEIGHT_4, COLOR16_BLUE, "Start", 44, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, doBlink,
            &doBlinkStartStop);
    TouchButtonBlinkStartStop.setCaptionForValueTrue("Stop");

    BlueDisplay1.debug(StartMessage);
}

/*
 * Function is called for resize + connect too
 */
void drawGui(void) {
    BlueDisplay1.clearDisplay(COLOR16_BLUE);
    TouchButtonBlinkStartStop.drawButton();
}

/*
 * Change doBlink flag as well as color and caption of the button.
 */
void doBlinkStartStop(BDButton *aTheTouchedButton __attribute__((unused)), int16_t aValue) {
    doBlink = aValue;
    /*
     * This debug output can also be recognized at the Arduino Serial Monitor
     */
    BlueDisplay1.debug("\r\nDoBlink=", (uint8_t) doBlink);
}
