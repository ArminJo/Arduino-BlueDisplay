/*
 * ManySlidersAndButtons.cpp
 *
 * Example for 8 / 16 sliders and 8 buttons
 * to control 8 / 16 Parameter values and 6 functions of an arduino application.
 * Buttons can be Red/Green toggle buttons like the 3. one.
 * Includes the implementation of a 4 value slider (PATTERN_SLIDER) with user provided text value.
 *
 *  Copyright (C) 2025  Armin Joachimsmeyer
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

#define NUMBER_OF_LEFT_SLIDERS      8
#define NUMBER_OF_RIGHT_SLIDERS     6
#define NUMBER_OF_BUTTONS           7
#define DISPLAY_BACKGROUND_COLOR COLOR16_WHITE

#define  CAPTION_TEXT_SIZE          16

/*
 * We use fixed screen layout here, to be able to use constants for all the positioning and size values
 */
//#define DISPLAY_WIDTH  DISPLAY_HALF_VGA_WIDTH  // 320
//#define DISPLAY_WIDTH  380  // For tablets
#define DISPLAY_WIDTH  370  // For tablets with small right black border so access the log without swiping from the left edge
#define DISPLAY_HEIGHT DISPLAY_HALF_VGA_HEIGHT // 240

/*
 * Configuration of the BlueDisplay library
 */
#define BLUETOOTH_BAUD_RATE BAUD_115200   // Activate this, if you have reprogrammed the HC05 module for 115200, otherwise 9600 is used as baud rate
#define DO_NOT_NEED_BASIC_TOUCH_EVENTS    // Disables basic touch events down, move and up. Saves 620 bytes program memory and 36 bytes RAM
#define DO_NOT_NEED_LONG_TOUCH_DOWN_AND_SWIPE_EVENTS  // Disables LongTouchDown and SwipeEnd events. Saves up to 88 bytes program memory and 4 bytes RAM.
#define DO_NOT_NEED_SPEAK_EVENTS            // Disables SpeakingDone event handling. Saves up to 54 bytes program memory and 18 bytes RAM.
#define ONLY_CONNECT_EVENT_REQUIRED       // Disables reorientation, redraw and SensorChange events
//#define BD_USE_SIMPLE_SERIAL              // Do not use the Serial object. Saves up to 1250 bytes program memory and 185 bytes RAM, if Serial is not used otherwise
//#define BD_USE_USB_SERIAL                 // Activate it, if you want to force using Serial instead of Serial1 for direct USB cable connection* to your smartphone / tablet.
#include "BlueDisplay.hpp"

#include "ManySlidersAndButtonsHelper.h"

/********************
 *      SLIDERS
 ********************/
/*
 * PROGMEM caption strings for sliders
 */
const char sString_Brightness[] PROGMEM = "Brightness";
const char sString_LoopDelay[] PROGMEM = "Loop delay";
const char sString_Variation[] PROGMEM = "Variation";
const char sString_PlusMinus[] PROGMEM = "Plus Minus";
const char sString_Minus[] PROGMEM = "Minus";
const char sString_Pattern[] PROGMEM = "Pattern";
const char sString_PositiveRange[] PROGMEM = "Positive range";

/*
 * PROGMEM array of PROGMEM structures for left sliders
 */
#if NUMBER_OF_LEFT_SLIDERS > 0
/**
 * An enum consisting of indexes of all sliders, used in doSlider() to determine the individual function for each slider.
 * !!!Must be the same order as used in SliderStaticInfoStruct!!!
 */
typedef enum {
    BRIGHTNESS_SLIDER = 0,
    DELAY_SLIDER,
    VARIATION_SLIDER,
    PLUS_MINUS_SLIDER,
    MINUS_SLIDER,
    PATTERN_SLIDER,
    EFFECT_MAX_LENGTH_SLIDER,
    SLIDER_8
} slider_Index_t;

#  if defined(ARDUINO) && defined(__AVR__)
static const SliderStaticInfoStruct sLeftSliderStaticInfoArray[NUMBER_OF_LEFT_SLIDERS] PROGMEM = {
#  else
static const SliderStaticInfoStruct sLeftSliderStaticInfoArray[NUMBER_OF_LEFT_SLIDERS]= {
#  endif
        { sString_Brightness, BD_SCREEN_BRIGHTNESS_MIN, BD_SCREEN_BRIGHTNESS_MAX, BD_SCREEN_BRIGHTNESS_MAX }, /*Brightness*/
        { sString_LoopDelay, 0, 1000, 500 }, /*Loop delay*/
        { sString_Variation, 0, 255, 255 }, /*Variation*/
        { sString_PlusMinus, -1000, 1000, 0 }, /*Plus Minus*/
        { sString_Minus, -255, 0, 0 }, /*Minus*/
        { sString_Pattern, 0, 3, 3 } /*Pattern 0 - 3*/
#  if NUMBER_OF_LEFT_SLIDERS > 6
        , { sString_PositiveRange, 100, 200, 200 } /*PositiveRange*/
#  endif
#  if NUMBER_OF_LEFT_SLIDERS > 7
        , { sString_ParameterValue8, 0, 255, 200 } /*Parameter value 8*/
#  endif
        };
#endif

#if NUMBER_OF_RIGHT_SLIDERS > 0
// PROGMEM array of PROGMEM structures for right sliders

#if defined(ARDUINO) && defined(__AVR__)
static const SliderStaticInfoStruct sRightSliderStaticInfoArray[NUMBER_OF_RIGHT_SLIDERS] PROGMEM = {
#else
static const SliderStaticInfoStruct sRightSliderStaticInfoArray[NUMBER_OF_RIGHT_SLIDERS]= {
#endif
#if NUMBER_OF_RIGHT_SLIDERS > 0
        { sString_ParameterValue9, 0, 255, 200 } /*Parameter value 9*/
#endif
#if NUMBER_OF_RIGHT_SLIDERS > 1
        , { sString_ParameterValue10, 0, 255, 200 } /*Parameter value 10*/
#endif
#if NUMBER_OF_RIGHT_SLIDERS > 2
        , { sString_ParameterValue11, 0, 255, 200 } /*Parameter value 11*/
#endif
#if NUMBER_OF_RIGHT_SLIDERS > 3
        , { sString_ParameterValue12, 0, 255, 200 } /*Parameter value 12*/
#endif
#if NUMBER_OF_RIGHT_SLIDERS > 4
        , { sString_ParameterValue13, 0, 255, 200 } /*Parameter value 13*/
#endif
#if NUMBER_OF_RIGHT_SLIDERS > 5
        , { sString_ParameterValue14, 0, 255, 200 } /*Parameter value 14*/
#endif
#if NUMBER_OF_RIGHT_SLIDERS > 6
        ,{ sString_ParameterValue15, 0, 255, 200 } /*Parameter value 15*/
#endif
#if NUMBER_OF_RIGHT_SLIDERS > 7
        ,{ sString_ParameterValue16, 0, 255, 200 } /*Parameter value 16*/
#endif
        };
#endif

/********************
 *     BUTTONS
 ********************/
// PROGMEM text strings for Test button
const char sString_Test[] PROGMEM = "Test";

/**
 * An enum consisting of indexes of all buttons, used in doButton() to determine the individual function for each button.
 * !!!Must be the same order as used in sButtonsTextArray!!!
 */
typedef enum {
    LOAD_BUTTON = 0, STORE_BUTTON, LED_BUTTON, TEST_BUTTON, BUTTON_AUTOREPEAT, BUTTON_SPEAK, BUTTON_7, BUTTON_8
} button_Index_t;

// Button text must be declared as PROGMEM separately for AVR, and cannot be used directly in initialization below
const char sString_ButtonAutorepeat[] PROGMEM = "Auto\nrepeat"; // 2 line text
const char sString_ButtonSpeak[] PROGMEM = "Speak"; // 2 line text

/*
 * PROGMEM array of PROGMEM structures for buttons
 */
#if defined(ARDUINO) && defined(__AVR__)
static const ButtonStaticInfoStruct sButtonStaticInfoStructArray[NUMBER_OF_BUTTONS] PROGMEM = {
#else
        static const ButtonStaticInfoStruct sButtonStaticInfoStructArray[NUMBER_OF_BUTTONS] = {
#endif
        LOAD_BUTTON, sString_Load, nullptr, /*Load values*/
        STORE_BUTTON, sString_Store, nullptr /*Store values*/
#if NUMBER_OF_BUTTONS > 2
        , 0, sString_LedOff, sString_LedOn /* This creates a RED / GREEN button */
#endif
#if NUMBER_OF_BUTTONS > 3
        , TEST_BUTTON, sString_Test, nullptr
#endif
#if NUMBER_OF_BUTTONS > 4
        , BUTTON_AUTOREPEAT, sString_ButtonAutorepeat, nullptr /* Autorepeat timing must be set manually in initDisplay() */
#endif
#if NUMBER_OF_BUTTONS > 5
        , BUTTON_SPEAK, sString_ButtonSpeak, nullptr
#endif
#if NUMBER_OF_BUTTONS > 6
        , BUTTON_7, sString_Button7, nullptr
#endif
#if NUMBER_OF_BUTTONS > 7
        , BUTTON_8, sString_Button8, nullptr
#endif
        };

/*
 * These 2 functions are required by ManySlidersAndButtonsHelper.hpp
 */
void doSlider(BDSlider *aTheTouchedSlider, int16_t aSliderValue);
void doPatternSlider(BDSlider *aTheTouchedSlider, int16_t aSliderValue);
void doButton(BDButton *aTheTouchedButton, int16_t aValue);
#include "ManySlidersAndButtonsHelper.hpp"

void initDisplay(void);
void drawDisplay(void);
void handleAutoRepeat();
void TestNewSliderAndButton();
void doTestSlider(BDSlider *aTheTouchedSlider, int16_t aSliderValue);
void doTestButton(BDButton *aTheTouchedButton, int16_t aValue);

const char StartMessage[] PROGMEM = "START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

#if defined(ESP32)
    Serial.begin(115200);
    Serial.println(StartMessage);
    initSerial("ESP-BD_Example");
    Serial.println("Start ESP32 BT-client with name \"ESP-BD_Example\"");
#else
    initSerial(); // On standard AVR platforms this results in Serial.begin(BLUETOOTH_BAUD_RATE) or Serial.begin(9600)
#endif
    BlueDisplay1.initCommunication(&Serial, &initDisplay); // Introduces up to 1.5 seconds delay and calls initDisplay(), if BT connection is available

#if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/ \
    || defined(SERIALUSB_PID)  || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_attiny3217)
delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#endif
// Just to know which program is running on my Arduino
    Serial.println(reinterpret_cast<const __FlashStringHelper*>(StartMessage));

    Serial.println(F("Setup finished"));
}

void loop() {
    checkAndHandleEvents(); // check for new data

    /*
     * Your code here
     */

    delayMillisWithCheckAndHandleEvents(sLeftSliderValues[DELAY_SLIDER]); // Using value of DELAY_SLIDER and check for new data
}

/*
 * Function used as callback handler for connect too
 */
void initDisplay(void) {
// Initialize display size and flags
    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE | BD_FLAG_SCREEN_ORIENTATION_LOCK_SENSOR_LANDSCAPE, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    initSlidersAndButtons(sLeftSliderStaticInfoArray, sRightSliderStaticInfoArray, sButtonStaticInfoStructArray);
    // Here slider values are loaded from EEPROM

    /*
     * Change pattern slider value handling to manual and set callback for manual value handling
     */
    sLeftSliderArray[PATTERN_SLIDER].setFlags(FLAG_SLIDER_IS_HORIZONTAL); // Reset FLAG_SLIDER_SHOW_VALUE flag, since we provide the value text by our own
    sLeftSliderArray[PATTERN_SLIDER].setCallback(&doPatternSlider);

    /*
     * After 700 ms repeat 5 times every 500ms, after this, repeat every 100 ms
     */
    sButtonArray[BUTTON_AUTOREPEAT].setButtonAutorepeatTiming(700, 500, 5, 100);
    drawDisplay();
    BlueDisplay1.speakStringBlockingWait(F("Display ready")); // Maximum 32 characters supported for F("")
}

void drawDisplay(void) {
    BlueDisplay1.clearDisplay(DISPLAY_BACKGROUND_COLOR);
    BlueDisplay1.drawText(STRING_ALIGN_MIDDLE_XPOS, CAPTION_TEXT_SIZE / 2,
            F(STR(NUMBER_OF_LEFT_SLIDERS) " + " STR(NUMBER_OF_RIGHT_SLIDERS) " sliders + " STR(NUMBER_OF_BUTTONS) " buttons demo"),
            CAPTION_TEXT_SIZE, COLOR16_BLUE, DISPLAY_BACKGROUND_COLOR);
    drawSlidersAndButtons();
    doPatternSlider(&sLeftSliderArray[PATTERN_SLIDER], sLeftSliderValues[PATTERN_SLIDER]); // Print value string after drawing slider
}

/*
 * Is called by touch or move on a slider and sets the new value
 */
void doSlider(BDSlider *aTheTouchedSlider, int16_t aSliderValue) {
    BDSliderIndex_t tSliderIndex = aTheTouchedSlider->mSliderIndex;

#if NUMBER_OF_RIGHT_SLIDERS > 0
    bool tIsLeftSlider = tSliderIndex < NUMBER_OF_LEFT_SLIDERS;
    if (tIsLeftSlider) {
        // Left sliders with index from 0 to NUMBER_OF_LEFT_SLIDERS - 1
        sLeftSliderValues[tSliderIndex] = aSliderValue; // Store value for later usage
    } else {
        // Right sliders with index from NUMBER_OF_LEFT_SLIDERS to NUMBER_OF_LEFT_SLIDERS + NUMBER_OF_RIGHT_SLIDERS - 1
        tSliderIndex -= NUMBER_OF_LEFT_SLIDERS;
        sRightSliderValues[tSliderIndex] = aSliderValue; // Store value for later usage
    }

    if (tIsLeftSlider) {

#else
    sLeftSliderValues[tSliderIndex] = aSliderValue;
#endif

        /*
         * alternative search solution if order is unknown
         */
//    for (uint8_t i = 0; i < NUMBER_OF_SLIDER_AND_BUTTON_LINES; i++) {
//        if (sLeftSliderArray[i] == *aTheTouchedSlider) {
//            BlueDisplay1.debug("LeftSliderArray index=", i);
//        }
//    }
        // Only check left sliders here
        switch (tSliderIndex) {
        case BRIGHTNESS_SLIDER:
            BlueDisplay1.setScreenBrightness(aSliderValue);
            break;
        default:
            char tStringBuffer[28];
            snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("Left slider %2u value=%hd"), tSliderIndex + 1, aSliderValue);
            BlueDisplay1.debug(tStringBuffer);
            break;
        }
#if NUMBER_OF_RIGHT_SLIDERS > 0
    } else {
        char tStringBuffer[27];
        snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("Right slider %2u value=%hd"), tSliderIndex + 1, aSliderValue); // Start with slider number 1
        BlueDisplay1.debug(tStringBuffer);
    }
#endif
}

/*
 * Here we do not handle the pattern slider in the default callback,
 * but use a special callback for it to make code more readable
 */
void doPatternSlider(BDSlider *aTheTouchedSlider, int16_t aSliderValue) {
    const char *tValueString;

    sLeftSliderValues[PATTERN_SLIDER] = aSliderValue; // Store value for later usage

    switch (aSliderValue) {
    case 0:
        tValueString = "  Constant temperature";
        break;
    case 1:
        tValueString = "    Random temperature";
        break;
    case 2:
        tValueString = "Random and some darker";
        break;
    case 3:
        tValueString = "Random darker/brighter";
        break;
    }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
    aTheTouchedSlider->printValue(tValueString);
#pragma GCC diagnostic pop
}

void handleAutoRepeat() {
    static int sCount = 0;
    sCount++;
    BlueDisplay1.drawShort(0, 0, sCount, BUTTON_TEXT_SIZE + BUTTON_TEXT_SIZE / 2, COLOR16_RED, DISPLAY_BACKGROUND_COLOR);
}

void handleSpeak() {
    static int sSliderIndex = 0;
    char tSliderNameBuffer[16];
    copyPGMStringStoredInPGMVariable(tSliderNameBuffer, (void*) &sLeftSliderStaticInfoArray[sSliderIndex].SliderName);

    char tStringBuffer[44];
    snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("Value of slider %s is %d"), tSliderNameBuffer,
            sLeftSliderValues[sSliderIndex]);

//    BlueDisplay1.debug(tStringBuffer);
    BlueDisplay1.speakStringBlockingWait(tStringBuffer);
    sSliderIndex++;
}

void doButton(BDButton *aTheTouchedButton, int16_t aValue) {
    switch (aTheTouchedButton->mButtonIndex) {
    case LOAD_BUTTON:
        loadSliderValuesFromEEPROM();
        doPatternSlider(&sLeftSliderArray[PATTERN_SLIDER], sLeftSliderValues[PATTERN_SLIDER]); // Print value string after drawing slider
        break;
    case STORE_BUTTON:
        storeSliderValuesToEEPROM();
        break;
    case LED_BUTTON:
        digitalWrite(LED_BUILTIN, aValue); // Toggle LED
        break;
    case TEST_BUTTON:
        TestNewSliderAndButton();
        break;
    case BUTTON_AUTOREPEAT:
        handleAutoRepeat();
        break;
    case BUTTON_SPEAK:
        handleSpeak();
        break;
    default:
        BlueDisplay1.debug("Pressed button ", (uint8_t) (aTheTouchedButton->mButtonIndex + 1)); // Start with button number 1
        break;
    }
}

/**
 * Create new temporarily button and slider, and wait 10 seconds until deleting button and slider
 */
void TestNewSliderAndButton() {
    /*
     * Create new temporarily button
     */
    BDButton *tButton = new BDButton(); // allocates one byte on the heap
    tButton->init((DISPLAY_WIDTH - BUTTON_WIDTH_4) / 2, DISPLAY_HEIGHT - BUTTON_HEIGHT_10, BUTTON_WIDTH_4,
    BUTTON_HEIGHT_10, 0, "Start", 11, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, false, &doTestButton);
    tButton->setTextForValueTrue("Stop");
    tButton->drawButton();

    /*
     * Create new temporarily slider
     */
    BDSlider *tSlider = new BDSlider(); // allocates one byte on the heap
    tSlider->init(BUTTON_DEFAULT_SPACING_HALF, DISPLAY_HEIGHT - BUTTON_HEIGHT_10, SLIDER_BAR_WIDTH, SLIDER_BAR_LENGTH, 100,
    SLIDER_BAR_LENGTH / 2, SLIDER_BAR_BG_COLOR, SLIDER_BAR_COLOR, FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_SHOW_VALUE,
            &doTestSlider);
    tSlider->setCaption("Test");

    tSlider->drawSlider();

    /*
     * Wait 10 seconds until deleting button and slider
     */
    delayMillisWithCheckAndHandleEvents(6000);

    tButton->removeButton(DISPLAY_BACKGROUND_COLOR);
    tButton->deinit();
    delete (tButton);

    tSlider->removeSlider(DISPLAY_BACKGROUND_COLOR);
    tSlider->deinit();
    delete (tSlider);
}

void doTestSlider(BDSlider *aTheTouchedSlider __attribute__((unused)), int16_t aSliderValue) {
    BlueDisplay1.debug("Test slider value ", aSliderValue);
}

void doTestButton(BDButton *aTheTouchedButton __attribute__((unused)), int16_t aButtonValue) {
    BlueDisplay1.debug("Test button value ", aButtonValue);
}

