/*
 * BlueDisplayBlink.cpp
 *
 *  Demo of using the BlueDisplay library for HC-05 on Arduino
 *
 *  Copyright (C) 2014-2020  Armin Joachimsmeyer
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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
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
void doBlinkStartStop(BDButton *aTheTochedButton, int16_t aValue);

// Callback handler for (re)connect and resize
void initDisplay(void);
void drawGui(void);

/*******************************************************************************************
 * Program code starts here
 *******************************************************************************************/

void setup() {
    // Initialize the LED pin as output.
    pinMode(LED_BUILTIN, OUTPUT);

#if defined(ESP32)
    Serial.begin(115200);
    Serial.println("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY);
    initSerial("ESP-BD_Example");
    Serial.println("Start ESP32 BT-client with name \"ESP-BD_Example\"");
#else
    initSerial();
#endif

    // Register callback handler and check for connection
    BlueDisplay1.initCommunication(&initDisplay, &drawGui);

#if defined(USE_SERIAL1) // defined in BlueSerial.h
// Serial(0) is available for Serial.print output.
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/|| defined(SERIALUSB_PID) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#  endif
// Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY));
#else
    BlueDisplay1.debug("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY);
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
