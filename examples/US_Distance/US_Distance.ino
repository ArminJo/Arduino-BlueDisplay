/*
 *  US_Distance.cpp
 *  Demo of using the BlueDisplay library for HC-05 on Arduino
 *  Shows the distances measured by a HC-SR04 ultrasonic sensor
 *  Can be used as a parking assistance.
 *  This is an example for using a fullscreen GUI.
 *
 *  Copyright (C) 2014-2022  Armin Joachimsmeyer
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

#include "PinDefinitionsAndMore.h"

/*
 * Settings to configure the BlueDisplay library and to reduce its size
 */
#define DO_NOT_NEED_BASIC_TOUCH_EVENTS // Disables basic touch events like down, move and up. Saves 620 bytes program memory and 36 bytes RAM
//#define BD_USE_SIMPLE_SERIAL // Do not use the Serial object. Saves up to 1250 bytes program memory and 185 bytes RAM, if Serial is not used otherwise
#include "BlueDisplay.hpp"

#include "HCSR04.hpp"

/****************************************************************************
 * Change this if you have reprogrammed the hc05 module for other baud rate
 ***************************************************************************/
#if !defined(BLUETOOTH_BAUD_RATE)
//#define BLUETOOTH_BAUD_RATE BAUD_115200
#define BLUETOOTH_BAUD_RATE BAUD_9600
#endif

//#define US_SENSOR_SUPPORTS_1_PIN_MODE // Activate it, if you use modified HC-SR04 modules or HY-SRF05 ones

#define MEASUREMENT_INTERVAL_MILLIS 50
#define DISTANCE_TIMEOUT_CM         300  // cm timeout for US reading

int sButtonTextSize;
int sCaptionStartX;
int sValueStartY;
int sCount;

/*
 * The Start Stop button
 */
BDButton TouchButtonStartStop;
BDButton TouchButtonOffset;
bool doTone = true;
int sOffset = 0;

BDSlider SliderShowDistance;

char sStringBuffer[100];

uint8_t sLastBDToneIndex = 0;
bool sToneIsOff = true;

void handleConnectAndReorientation(void);
void drawGui(void);

void doStartStop(BDButton *aTheTouchedButton __attribute__((unused)), int16_t aValue);
void doGetOffset(BDButton *aTheTouchedButton __attribute__((unused)), int16_t aValue __attribute__((unused)));

// PROGMEM messages sent by BlueDisplay1.debug() are truncated to 32 characters :-(, so must use RAM here
const char StartMessage[] = "START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY;

/*******************************************************************************************
 * Program code starts here
 *******************************************************************************************/

void setup(void) {

#if defined(US_SENSOR_SUPPORTS_1_PIN_MODE)
    pinMode(LED_BUILTIN, OUTPUT);

    initUSDistancePin(TRIGGER_OUT_PIN);
#else
    initUSDistancePins(TRIGGER_OUT_PIN, ECHO_IN_PIN);
#endif

#if defined(ESP32)
    Serial.begin(115200);
    Serial.println(StartMessage);
    initSerial("ESP-BD_Example");
    Serial.println("Start ESP32 BT-client with name \"ESP-BD_Example\"");
#else
    initSerial(BLUETOOTH_BAUD_RATE);
#endif

    /*
     * Register callback handler and wait for 300 ms if Bluetooth connection is still active.
     * For ESP32 and after power on of the Bluetooth module (HC-05) at other platforms, Bluetooth connection is most likely not active here.
     *
     * If active, mCurrentDisplaySize and mHostUnixTimestamp are set and initDisplay() and drawGui() functions are called.
     * If not active, the periodic call of checkAndHandleEvents() in the main loop waits for the (re)connection and then performs the same actions.
     */
    uint16_t tConnectDurationMillis = BlueDisplay1.initCommunication(&handleConnectAndReorientation, &drawGui); // introduces up to 1.5 seconds delay
#if !defined(BD_USE_SIMPLE_SERIAL)
    if (tConnectDurationMillis > 0) {
        Serial.print("Connection established after ");
        Serial.print(tConnectDurationMillis);
        Serial.println(" ms");
    } else {
        Serial.println(F("No connection after " STR(CONNECTIOM_TIMEOUT_MILLIS) " ms"));
    }
#endif

#if defined(BD_USE_SERIAL1) || defined(ESP32) // BD_USE_SERIAL1 may be defined in BlueSerial.h
// Serial(0) is available for Serial.print output.
#  if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/ \
    || defined(SERIALUSB_PID)  || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#  endif
// Just to know which program is running on my Arduino
    Serial.println(StartMessage);
#elif !defined(BD_USE_SIMPLE_SERIAL)
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

    /*
     * on double tone, we received max canvas size. Otherwise no connection is available.
     */
    if (BlueDisplay1.isConnectionEstablished()) {
        tone(TONE_PIN, 3000, 50);
        delay(100);
    }
    tone(TONE_PIN, 3000, 50);
    delay(100);
}

void loop(void) {
    getUSDistanceAsCentimeterWithCentimeterTimeout(DISTANCE_TIMEOUT_CM);
    auto tUSDistanceCentimeter = sUSDistanceCentimeter;
#if ! defined(BD_USE_SIMPLE_SERIAL) || defined(BD_USE_SERIAL1)
    // If using simple serial on first USART we cannot use Serial.print, since this uses the same interrupt vector as simple serial.
#  if !defined(BD_USE_SERIAL1) && !defined(ESP32)
    // If we do not use Serial1 for BlueDisplay communication, we must check if we are not connected and therefore Serial is available for info output.
    if (!BlueDisplay1.isConnectionEstablished()) {
#  endif
        if (tUSDistanceCentimeter == DISTANCE_TIMEOUT_RESULT) {
            Serial.println("timeout");
        } else {
            Serial.print(tUSDistanceCentimeter);
            Serial.print(" cm, ");
            Serial.print(sUSDistanceMicroseconds);
            Serial.println(" micro secounds.");
        }
#  if !defined(BD_USE_SERIAL1) && !defined(ESP32)
    }
#  endif
#endif

    if (tUSDistanceCentimeter == DISTANCE_TIMEOUT_RESULT) {
        /*
         * Here distance timeout happened
         */
        tone(TONE_PIN, 1000, 50);
        delay(100);
        tone(TONE_PIN, 2000, 50);
        delay((100 - MEASUREMENT_INTERVAL_MILLIS) - 20);

    } else {
        if (doTone && tUSDistanceCentimeter < 100) {
            /*
             * local feedback for distances < 100 cm
             */
            tone(TONE_PIN, tUSDistanceCentimeter * 32, MEASUREMENT_INTERVAL_MILLIS + 20);
        }
        tUSDistanceCentimeter -= sOffset;
        if (tUSDistanceCentimeter != sLastUSDistanceCentimeter) {
            if (BlueDisplay1.mBlueDisplayConnectionEstablished) {
                uint16_t tCmXPosition = BlueDisplay1.drawUnsignedByte(getTextWidth(sButtonTextSize * 2), sValueStartY,
                        tUSDistanceCentimeter, sButtonTextSize * 2, COLOR16_YELLOW, COLOR16_BLUE);
                BlueDisplay1.drawText(tCmXPosition, sValueStartY, "cm", sButtonTextSize, COLOR16_WHITE, COLOR16_BLUE);
                SliderShowDistance.setValueAndDrawBar(tUSDistanceCentimeter);
            }
            if (tUSDistanceCentimeter >= 40 || !doTone) {
                /*
                 * Silence here
                 */
                if (!sToneIsOff) {
                    sToneIsOff = true;
                    if (BlueDisplay1.mBlueDisplayConnectionEstablished) {
                        // Stop BD tone
                        BlueDisplay1.playTone(TONE_SILENCE);
                    }
                }
            } else {
                sToneIsOff = false;
                /*
                 * Switch tones only if range changes
                 */
                if (BlueDisplay1.mBlueDisplayConnectionEstablished) {
                    uint8_t tBDToneIndex;
                    if (tUSDistanceCentimeter > 30) {
                        tBDToneIndex = 22;
                    } else if (tUSDistanceCentimeter > 20) {
                        tBDToneIndex = 17;
                    } else if (tUSDistanceCentimeter > 10) {
                        tBDToneIndex = 18;
                    } else if (tUSDistanceCentimeter > 3) {
                        tBDToneIndex = 16;
                    } else {
                        tBDToneIndex = 36;
                    }
                    if (sLastBDToneIndex != tBDToneIndex) {
                        sLastBDToneIndex = tBDToneIndex;
                        BlueDisplay1.playTone(tBDToneIndex);
                    }
                }
            }
        }
        sLastUSDistanceCentimeter = tUSDistanceCentimeter;
    }
    checkAndHandleEvents();
    delay(MEASUREMENT_INTERVAL_MILLIS); // < 200
}

/*
 * Handle change from landscape to portrait
 */
void handleConnectAndReorientation(void) {
//    tone(TONE_PIN, 1000, 50);
    // manage positions according to actual display size
    int tCurrentDisplayWidth = BlueDisplay1.getHostDisplayWidth();
    int tCurrentDisplayHeight = BlueDisplay1.getHostDisplayHeight();
    if (tCurrentDisplayWidth < tCurrentDisplayHeight) {
        // Portrait -> change to landscape 3/2 format
        tCurrentDisplayHeight = (tCurrentDisplayWidth / 3) * 2;
    }
    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_TOUCH_BASIC_DISABLE, tCurrentDisplayWidth,
            tCurrentDisplayHeight);

    /*
     * Compute text sizes etc.
     */
    sButtonTextSize = tCurrentDisplayHeight / 4;
    // Position Caption at middle of screen
    sCaptionStartX = (tCurrentDisplayWidth - (getTextWidth(sButtonTextSize) * strlen("Distance"))) / 2;

    snprintf(sStringBuffer, sizeof(sStringBuffer), "sCurrentDisplayWidth=%d", tCurrentDisplayWidth);
    BlueDisplay1.debugMessage(sStringBuffer);

    if (sCaptionStartX < 0) {
        sCaptionStartX = 0;
    }

    sValueStartY = getTextAscend(sButtonTextSize * 2) + sButtonTextSize + sButtonTextSize / 4;

    SliderShowDistance.init(0, sButtonTextSize * 3, sButtonTextSize / 4, tCurrentDisplayWidth, 199, 0, COLOR16_BLUE,
    COLOR16_GREEN, FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    SliderShowDistance.setScaleFactor(200.0 / tCurrentDisplayWidth);

    // Initialize button position, size, colors etc.
    TouchButtonStartStop.init(0, BUTTON_HEIGHT_5_DYN_LINE_5, BUTTON_WIDTH_3_DYN, BUTTON_HEIGHT_5_DYN, COLOR16_BLUE, "Start Tone",
            sButtonTextSize / 3, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, doTone, &doStartStop);
    TouchButtonStartStop.setTextForValueTrue("Stop Tone");

    TouchButtonOffset.init(BUTTON_WIDTH_3_DYN_POS_3, BUTTON_HEIGHT_5_DYN_LINE_5, BUTTON_WIDTH_3_DYN, BUTTON_HEIGHT_5_DYN,
    COLOR16_RED, "Offset", sButtonTextSize / 3, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doGetOffset);

    BlueDisplay1.debug(StartMessage);
}

/*
 * Function used as callback handler for redraw event
 */
void drawGui(void) {
    BlueDisplay1.clearDisplay(COLOR16_BLUE);
    BlueDisplay1.drawText(sCaptionStartX, sButtonTextSize, "Distance", sButtonTextSize, COLOR16_WHITE, COLOR16_BLUE);
    SliderShowDistance.drawSlider();
    TouchButtonStartStop.drawButton();
    TouchButtonOffset.drawButton();
}

/*
 * Change doTone flag as well as color and caption of the button.
 */
void doStartStop(BDButton *aTheTouchedButton __attribute__((unused)), int16_t aValue) {
    doTone = aValue;
    if (!aValue) {
        // Stop tone
        BlueDisplay1.playTone(TONE_SILENCE);
    }
}

/*
 * Handler for number receive event - set delay to value
 */
void doSetOffset(float aValue) {
// clip value
    if (aValue > 20) {
        aValue = 20;
    } else if (aValue < -20) {
        aValue = -20;
    }
    sOffset = aValue;
}
/*
 * Request delay value as number
 */
void doGetOffset(BDButton *aTheTouchedButton __attribute__((unused)), int16_t aValue __attribute__((unused))) {
    BlueDisplay1.getNumberWithShortPrompt(&doSetOffset, "Offset distance [cm]");
}

