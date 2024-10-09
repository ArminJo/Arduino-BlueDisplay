/*
 * BlueDisplayExample.cpp
 *
 *  Demo of using the BlueDisplay library for HC-05 on Arduino
 *  Blink frequency can be entered in 3 different ways by GUI.
 *  1. By + and - buttons
 *  2. By slider
 *  3. By numeric keypad
 *
 *  Output of actual delay is numeric and by slider bar
 *
 *  For handling time, the Arduino "time" library is used. You can install it also with "Tools -> Manage Libraries..." or "Ctrl+Shift+I" -> use "timekeeping" as filter string.
 *  The sources can also be found here: https://github.com/PaulStoffregen/Time
 *
 *  With 9600 baud, the minimal blink delay we observe is 200 ms because of the communication delay
 *  of 8 * printDemoString(), which requires 8*24 ms -> 192 ms
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

#include "PinDefinitionsAndMore.h"

#define LOCAL_DISPLAY_WIDTH     DISPLAY_HALF_VGA_WIDTH  // 320
#define LOCAL_DISPLAY_HEIGHT    DISPLAY_HALF_VGA_HEIGHT // 240

/*
 * Settings to configure the BlueDisplay library and to reduce its size
 * With 9600 baud, the minimal blink delay we observe is 200 ms because of the communication delay
 * of 8 * printDemoString(), which requires 8*24 ms -> 192 ms
 */
//#define BLUETOOTH_BAUD_RATE BAUD_115200   // Activate this, if you have reprogrammed the HC05 module for 115200, otherwise 9600 is used as baud rate
#define DO_NOT_NEED_BASIC_TOUCH_EVENTS    // Disables basic touch events like down, move and up. Saves 620 bytes program memory and 36 bytes RAM
//#define USE_SIMPLE_SERIAL                 // Do not use the Serial object. Saves up to 1250 bytes program memory and 185 bytes RAM, if Serial is not used otherwise
//#define USE_USB_SERIAL                    // Activate it, if you want to force using Serial instead of Serial1 for direct USB cable connection* to your smartphone / tablet.
#include "BlueDisplay.hpp"

#if defined(ESP32) || defined(ESP8266)
#define USE_C_TIME
#endif

#if defined(USE_C_TIME)
#include "time.h"
#else
#include "TimeLib.h"
#endif

#define DELAY_START_VALUE 600
#define DELAY_CHANGE_VALUE 10

#define SLIDER_X_POSITION 80

#define COLOR_DEMO_BACKGROUND COLOR16_BLUE
#define COLOR_CAPTION COLOR16_RED

#if defined(USE_C_TIME)
struct tm *sTimeInfo;
#endif

bool sConnected = false;
bool doBlink = true;
bool sInTestPage = false; // Creates a new page and displays test patterns

int16_t sDelay = DELAY_START_VALUE; // 600

// a string buffer for any purpose...
char sStringBuffer[128];

/*
 * The buttons
 */
BDButton TouchButtonBDExampleBlinkStartStop;
BDButton TouchButtonPlus;
BDButton TouchButtonMinus;
BDButton TouchButtonValueDirect;
BDButton TouchButtonTest;
BDButton TouchButtonBack;

// Touch handler for buttons
void doBDExampleBlinkStartStop(BDButton *aTheTouchedButton, int16_t aValue);
void doPlusMinus(BDButton *aTheTouchedButton, int16_t aValue);
void doSetDelay(float aValue);
void doGetDelay(BDButton *aTheTouchedButton, int16_t aValue);
void doTest(BDButton *aTheTouchedButton, int16_t aValue);
void doBack(BDButton *aTheTouchedButton, int16_t aValue);

/*
 * The horizontal slider
 */
BDSlider TouchSliderDelay;
// handler for value change
void doDelay(BDSlider *aTheTouchedSlider, uint16_t aSliderValue);

/*
 * Time functions
 */
void printTime();
void infoEventCallback(uint8_t aSubcommand, uint8_t aByteInfo, uint16_t aShortInfo, ByteShortLongFloatUnion aLongInfo);
#if ! defined(USE_C_TIME)
time_t requestTimeSync();
#endif

void printDemoString(void);

// Callback handler for (re)connect and reorientation
void initDisplay(void);
// Callback handler for redraw
void drawGui(void);

// PROGMEM messages sent by BlueDisplay1.debug() are truncated to 32 characters :-(, so must use RAM here
const char StartMessage[] = "START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY;

/*******************************************************************************************
 * Program code starts here
 *******************************************************************************************/

void setup() {
    // initialize the digital pin as an output.
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

#if defined(TONE_PIN)
    // to signal that boot has finished
    tone(TONE_PIN, 2000, 200);
#endif
}

void loop() {
    static unsigned long sLastMilisOfTimePrinted;

    if (!BlueDisplay1.isConnectionEstablished()) {
        /*
         * Not connected here, so send serial output, which is readable at the Arduino serial monitor
         */
        uint16_t tBlinkDuration = analogRead(ANALOG_INPUT_PIN);
        Serial.print("AnalogIn=");
        Serial.println(tBlinkDuration);

        digitalWrite(LED_BUILTIN, HIGH);
        // Delay and check for connection
        delayMillisWithCheckAndHandleEvents(tBlinkDuration / 2);
        digitalWrite(LED_BUILTIN, LOW);
        delayMillisWithCheckAndHandleEvents(tBlinkDuration / 2);

    } else if (!sInTestPage) {
        /*
         * Connected here
         */
        if (doBlink) {
            /*
             * Blinking is enabled, switch LED on
             */
            digitalWrite(LED_BUILTIN, HIGH);
            BlueDisplay1.fillCircle(LOCAL_DISPLAY_WIDTH / 2, LOCAL_DISPLAY_HEIGHT / 2, 20, COLOR16_RED);
            /*
             *  Wait for delay time and update "Demo" string at a rate 2*8 times the blink rate.
             *  Check touch events 8 times while waiting.
             */
            for (uint8_t i = 0; i < 8; ++i) {
                delayMillisWithCheckAndHandleEvents(sDelay / 8); // This can lead to display of the test page
                if (sInTestPage) {
                    return; // leave inner and outer loop immediately
                }
                printDemoString();
            }
            /*
             * LED off
             */
            digitalWrite(LED_BUILTIN, LOW);
            BlueDisplay1.fillCircle(LOCAL_DISPLAY_WIDTH / 2, LOCAL_DISPLAY_HEIGHT / 2, 20, COLOR_DEMO_BACKGROUND);
            for (uint8_t i = 0; i < 8; ++i) {
                delayMillisWithCheckAndHandleEvents(sDelay / 8); // This can lead to display of the test page
                if (sInTestPage) {
                    return; // leave inner and outer loop immediately
                }
                printDemoString();
            }
        }
        /*
         * print time every second
         */
        unsigned long tMillis = millis();
        if (tMillis - sLastMilisOfTimePrinted > 1000) {
            sLastMilisOfTimePrinted = tMillis;
#if defined(USE_C_TIME)
            BlueDisplay1.getInfo(SUBFUNCTION_GET_INFO_LOCAL_TIME, &infoEventCallback);
#endif
            printTime();
        }
    }

    checkAndHandleEvents();
}

void initDisplay(void) {

    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE | BD_FLAG_TOUCH_BASIC_DISABLE, LOCAL_DISPLAY_WIDTH,
    LOCAL_DISPLAY_HEIGHT);

    TouchButtonPlus.init(270, 80, 40, 40, COLOR16_YELLOW, F("+"), 33, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT,
    DELAY_CHANGE_VALUE, &doPlusMinus);
    TouchButtonPlus.setButtonAutorepeatTiming(600, 100, 10, 30);

    TouchButtonMinus.init(10, 80, 40, 40, COLOR16_YELLOW, F("-"), 33, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT,
            -DELAY_CHANGE_VALUE, &doPlusMinus);
    TouchButtonMinus.setButtonAutorepeatTiming(600, 100, 10, 30);

//    TouchButtonBDExampleBlinkStartStop.init(&ButtonStartStopInit, F("Start"), doBlink);
    TouchButtonBDExampleBlinkStartStop.init(30, 150, 140, 55, COLOR_DEMO_BACKGROUND, F("Start"), 44,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, doBlink, &doBDExampleBlinkStartStop);
    TouchButtonBDExampleBlinkStartStop.setCaptionForValueTrue(F("Stop"));

//    TouchButtonValueDirect.init(&ButtonValueDirectInit, F("..."));
    TouchButtonValueDirect.init(210, 150, 90, 55, COLOR16_YELLOW, F("..."), 44, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doGetDelay);

    TouchButtonTest.init(BUTTON_WIDTH_4_POS_4, 0, BUTTON_WIDTH_4, BUTTON_HEIGHT_6, COLOR16_DARK_BLUE, "Test", TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH, -1, &doTest);

    TouchButtonBack.init(BUTTON_WIDTH_4_POS_4, 0, BUTTON_WIDTH_4, BUTTON_HEIGHT_6, COLOR16_RED, "Back", TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH, -1, &doBack);

    TouchSliderDelay.init(SLIDER_X_POSITION, 40, 12, 150, 100, DELAY_START_VALUE, COLOR16_YELLOW, COLOR16_GREEN,
            FLAG_SLIDER_SHOW_BORDER | FLAG_SLIDER_SHOW_VALUE | FLAG_SLIDER_IS_HORIZONTAL, &doDelay);
    TouchSliderDelay.setCaptionProperties(TEXT_SIZE_22, FLAG_SLIDER_VALUE_CAPTION_ALIGN_RIGHT, 4, COLOR16_RED, COLOR_DEMO_BACKGROUND);
    TouchSliderDelay.setCaption("Delay");
    TouchSliderDelay.setScaleFactor(10); // Slider is virtually 10 times larger
    TouchSliderDelay.setValueUnitString("ms");

    TouchSliderDelay.setPrintValueProperties(TEXT_SIZE_22, FLAG_SLIDER_VALUE_CAPTION_ALIGN_LEFT, 4, COLOR16_WHITE, COLOR_DEMO_BACKGROUND);

    // here we have received a new local timestamp
#if defined(USE_C_TIME)
// initialize sTimeInfo
    sTimeInfo = localtime((const time_t*) &BlueDisplay1.mHostUnixTimestamp);
#else
    // Set function to call when time sync required (default: after 300 seconds)
    setSyncProvider(requestTimeSync); // calls getInfo() immediately
    setTime(BlueDisplay1.mHostUnixTimestamp);
#endif

    BlueDisplay1.debug(StartMessage);
}

void drawGui(void) {
    BlueDisplay1.clearDisplay(COLOR_DEMO_BACKGROUND);
    TouchButtonBDExampleBlinkStartStop.drawButton();
    TouchButtonPlus.drawButton();
    TouchButtonMinus.drawButton();
    TouchButtonValueDirect.drawButton();
    TouchButtonTest.drawButton();
    TouchSliderDelay.drawSlider();
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
/*
 * Change doBlink flag
 */
void doBDExampleBlinkStartStop(BDButton *aTheTouchedButton __attribute__((unused)), int16_t aValue) {
    doBlink = aValue;
}

#if defined(USE_C_TIME)
void infoEventCallback(uint8_t aSubcommand, uint8_t aByteInfo, uint16_t aShortInfo, ByteShortLongFloatUnion aLongInfo) {
    if (aSubcommand == SUBFUNCTION_GET_INFO_LOCAL_TIME) {
        sTimeInfo = localtime((const time_t*) &aLongInfo.uint32Value);
    }
}

void printTime() {
    sprintf_P(sStringBuffer, PSTR("%02d.%02d.%4d %02d:%02d:%02d"), sTimeInfo->tm_mday, sTimeInfo->tm_mon, sTimeInfo->tm_year + 1900,
            sTimeInfo->tm_hour, sTimeInfo->tm_min, sTimeInfo->tm_sec);
    BlueDisplay1.drawText(LOCAL_DISPLAY_WIDTH - 20 * TEXT_SIZE_11_WIDTH, LOCAL_DISPLAY_HEIGHT - TEXT_SIZE_11_HEIGHT, sStringBuffer,
            11,
            COLOR16_BLACK, COLOR_DEMO_BACKGROUND);
}

#else
/*
 * Is called every 5 minutes by default
 */
time_t requestTimeSync() {
    BlueDisplay1.getInfo(SUBFUNCTION_GET_INFO_LOCAL_TIME, &infoEventCallback);
    return 0; // the time will be sent later in response to getInfo
}

void infoEventCallback(uint8_t aSubcommand, uint8_t aByteInfo, uint16_t aShortInfo, ByteShortLongFloatUnion aLongInfo) {
    if (aSubcommand == SUBFUNCTION_GET_INFO_LOCAL_TIME) {
        setTime(aLongInfo.uint32Value);
        // to prove that it is called every 5 minutes by default
        // tone(TONE_PIN, 1000, 200);
    }
}

void printTime() {
    // 1600 byte code size for time handling plus print
    sprintf_P(sStringBuffer, PSTR("%02d.%02d.%4d %02d:%02d:%02d"), day(), month(), year(), hour(), minute(), second());
    BlueDisplay1.drawText(LOCAL_DISPLAY_WIDTH - 20 * TEXT_SIZE_11_WIDTH, LOCAL_DISPLAY_HEIGHT - TEXT_SIZE_11_HEIGHT, sStringBuffer, 11,
    COLOR16_BLACK, COLOR_DEMO_BACKGROUND);
}
#endif

/*
 * Is called by touch of both plus and minus buttons.
 * We just take the passed value and do not care which button was touched
 */
void doPlusMinus(BDButton *aTheTouchedButton __attribute__((unused)), int16_t aValue) {
    sDelay += aValue;
    if (sDelay < DELAY_CHANGE_VALUE) {
        sDelay = DELAY_CHANGE_VALUE;
    }
    if (!doBlink) {
        // enable blinking
        doBlink = true;
        TouchButtonBDExampleBlinkStartStop.setValueAndDraw(doBlink);
    }
    // set slider bar accordingly
    TouchSliderDelay.setValueAndDrawBar(sDelay);
    /*
     * Example for debug/toast output by BlueDisplay
     */
    BlueDisplay1.debug("Delay=", sDelay);
}

/*
 * Handler for number receive event - set delay to value
 */
void doSetDelay(float aValue) {
// clip value
    if (aValue > 100000) {
        aValue = 100000;
    } else if (aValue < 1) {
        aValue = 1;
    }
    sDelay = aValue;
    // set slider bar accordingly
    TouchSliderDelay.setValueAndDrawBar(sDelay);
}

/*
 * Request delay value as number
 */
void doGetDelay(BDButton *aTheTouchedButton, int16_t aValue) {
    BlueDisplay1.getNumberWithShortPrompt(&doSetDelay, "delay [ms]");
}

/*
 * Is called by touch or move on slider and sets the new delay according to the passed slider value
 */
void doDelay(BDSlider *aTheTouchedSlider __attribute__((unused)), uint16_t aSliderValue) {
    sDelay = aSliderValue;
}

/*
 * Request delay value as number
 */
void doBack(BDButton *aTheTouchedButton, int16_t aValue) {
    TouchButtonBack.deactivate();
    drawGui();
    sInTestPage = false;
}

/*
 * Creates a new page and displays test patterns
 */
void doTest(BDButton *aTheTouchedButton, int16_t aValue) {
    sInTestPage = true;
    BDButton::deactivateAll();
    BDSlider::deactivateAll();
    BlueDisplay1.testDisplay(); // Blocking draw of test patterns
    TouchButtonBack.drawButton(); // this also activates the button
}

#define MILLIS_PER_CHANGE 20 // gives minimal 2 seconds
void printDemoString(void) {
    static float tFadingFactor = 0.0; // 0 -> Background 1 -> Caption
    static float tInterpolationDelta = 0.01;

    static bool tFadingFactorDirectionFromBackground = true;
    static unsigned long MillisOfNextChange;

    // Timing
    unsigned long tMillis = millis();
    if (tMillis >= MillisOfNextChange) {
        MillisOfNextChange = tMillis + MILLIS_PER_CHANGE;

        // slow fade near background color
        if (tFadingFactor <= 0.1) {
            tInterpolationDelta = 0.002;
        } else {
            tInterpolationDelta = 0.01;
        }

        // manage fading factor
        if (tFadingFactorDirectionFromBackground) {
            tFadingFactor += tInterpolationDelta;
            if (tFadingFactor >= (1.0 - 0.01)) {
                // toggle direction to background
                tFadingFactorDirectionFromBackground = !tFadingFactorDirectionFromBackground;
            }
        } else {
            tFadingFactor -= tInterpolationDelta;
            if (tFadingFactor <= tInterpolationDelta) {
                // toggle direction
                tFadingFactorDirectionFromBackground = !tFadingFactorDirectionFromBackground;
            }
        }

        // get resulting color
        uint8_t ColorRed = COLOR16_GET_RED(COLOR_DEMO_BACKGROUND)
                + ((int16_t) ( COLOR16_GET_RED(COLOR_CAPTION) - COLOR16_GET_RED(COLOR_DEMO_BACKGROUND)) * tFadingFactor);
        uint8_t ColorGreen = COLOR16_GET_GREEN(COLOR_DEMO_BACKGROUND)
                + ((int16_t) (COLOR16_GET_GREEN(COLOR_CAPTION) - COLOR16_GET_GREEN(COLOR_DEMO_BACKGROUND)) * tFadingFactor);
        uint8_t ColorBlue = COLOR16_GET_BLUE(COLOR_DEMO_BACKGROUND)
                + ((int16_t) ( COLOR16_GET_BLUE(COLOR_CAPTION) - COLOR16_GET_BLUE(COLOR_DEMO_BACKGROUND)) * tFadingFactor);
        BlueDisplay1.drawText(LOCAL_DISPLAY_WIDTH / 2 - 2 * getTextWidth(36), 4 + getTextAscend(36), "Demo", 36,
                COLOR16(ColorRed, ColorGreen, ColorBlue), COLOR_DEMO_BACKGROUND);
    }
}
