/*
 *  BlueDisplayBlink.cpp
 *
 *
 *  Demo of using the BlueDisplay library for HC-05 on Arduino
 *  Blink frequency can be entered in 3 different ways by GUI.
 *  1. By + and - buttons
 *  2. By slider
 *  3. By numeric keypad
 *
 *  Output of actual delay is numeric and by slider bar

 *  Copyright (C) 2014  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay https://github.com/ArminJo/android-blue-display.
 *
 *  BlueDisplay is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#include <Arduino.h>

#include "BlueDisplay.h"

/****************************************************************************
 * Change this if you have reprogrammed the hc05 module for other baud rate
 ***************************************************************************/
#ifndef BLUETOOTH_BAUD_RATE
//#define BLUETOOTH_BAUD_RATE BAUD_115200
#define BLUETOOTH_BAUD_RATE BAUD_9600
#endif

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 256

bool doBlink = true;

#define VERSION_EXAMPLE "1.1"

/*
 * The Start Stop button
 */
BDButton TouchButtonBlinkStartStop;

// Touch handler for buttons
void doBlinkStartStop(BDButton * aTheTochedButton, int16_t aValue);

// Callback handler for (re)connect and resize
void initDisplay(void);
void drawGui(void);

void setup() {
    // Initialize the LED pin as output.
    pinMode(LED_BUILTIN, OUTPUT);

#ifdef USE_SIMPLE_SERIAL
    initSimpleSerial(BLUETOOTH_BAUD_RATE);
#else
#  if defined (USE_SERIAL1)
    Serial1.begin(BLUETOOTH_BAUD_RATE);
#    if defined(SERIAL_USB)
    delay(2000); // To be able to connect Serial monitor after reset and before first printout
#    endif
	// Just to know which program is running on my Arduino
	Serial.println(F("START " __FILE__ "\r\nVersion " VERSION_EXAMPLE " from " __DATE__));
#  else
    Serial.begin(BLUETOOTH_BAUD_RATE);
#  endif
#endif

    // Register callback handler and check for connection
    BlueDisplay1.initCommunication(&initDisplay, &drawGui);
}

void loop() {

    /*
     * This debug output can also be recognized at the Arduino Serial Monitor
     */
//    BlueDisplay1.debug("\r\nDoBlink=", (uint8_t) doBlink);
    if (doBlink) {
        // LED on
        digitalWrite(LED_BUILTIN, HIGH);
        BlueDisplay1.fillCircle(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, 20, COLOR_RED);
        delayMillisWithCheckAndHandleEvents(300);
    }

    if (doBlink) {
        // LED off
        digitalWrite(LED_BUILTIN, LOW);
        BlueDisplay1.fillCircle(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, 20, COLOR_BLUE);
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
    TouchButtonBlinkStartStop.init((DISPLAY_WIDTH - BUTTON_WIDTH_2) / 2, BUTTON_HEIGHT_4_256_LINE_4, BUTTON_WIDTH_2, BUTTON_HEIGHT_4_256,
    COLOR_BLUE, "Start", 44, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, doBlink, &doBlinkStartStop);
    TouchButtonBlinkStartStop.setCaptionForValueTrue("Stop");
}

/*
 * Function is called for resize + connect too
 */
void drawGui(void) {
    BlueDisplay1.clearDisplay(COLOR_BLUE);
    TouchButtonBlinkStartStop.drawButton();
}

/*
 * Change doBlink flag as well as color and caption of the button.
 */
void doBlinkStartStop(BDButton * aTheTouchedButton, int16_t aValue) {
    doBlink = aValue;
    /*
     * This debug output can also be recognized at the Arduino Serial Monitor
     */
    BlueDisplay1.debug("\r\nDoBlink=", (uint8_t) doBlink);
}
