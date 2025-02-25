/*
 * ManySlidersAndButtons.cpp
 *
 * Example for 8 / 16 sliders and 8 buttons
 * to control 8 / 16 analog values and 6 functions of an arduino application.
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

//#define USE_ONLY_LEFT_SLIDERS // Do not show and use right sliders
#define NUMBER_OF_SLIDER_AND_BUTTON_LINES 8
#define DISPLAY_BACKGROUND_COLOR COLOR16_WHITE

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
#define ONLY_CONNECT_EVENT_REQUIRED       // Disables reorientation, redraw and SensorChange events
//#define BD_USE_SIMPLE_SERIAL                 // Do not use the Serial object. Saves up to 1250 bytes program memory and 185 bytes RAM, if Serial is not used otherwise
//#define BD_USE_USB_SERIAL                    // Activate it, if you want to force using Serial instead of Serial1 for direct USB cable connection* to your smartphone / tablet.
#include "BlueDisplay.hpp"

/********************
 *      SLIDERS
 ********************/
#define SLIDER_BAR_WIDTH        BUTTON_DEFAULT_SPACING_THREE_QUARTER
#define SLIDER_BAR_LENGTH       128
#define SLIDER_CAPTION_SIZE     7
#define SLIDER_CAPTION_MARGIN   2
#define SLIDER_BAR_COLOR        COLOR16_GREEN
#define SLIDER_BAR_BG_COLOR     COLOR16_YELLOW
#define SLIDER_CAPTION_COLOR    COLOR16_BLACK
#define SLIDER_VALUE_COLOR      COLOR16_BLUE
#define SLIDER_CAPTION_BG_COLOR DISPLAY_BACKGROUND_COLOR

BDSlider sLeftSliderArray[NUMBER_OF_SLIDER_AND_BUTTON_LINES];
BDSlider sRightSliderArray[NUMBER_OF_SLIDER_AND_BUTTON_LINES];

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
const char sString_AnalogValue1[] PROGMEM = "Analog value 1";
const char sString_AnalogValue2[] PROGMEM = "Analog value 2";
const char sString_AnalogValue3[] PROGMEM = "Analog value 3";
const char sString_AnalogValue4[] PROGMEM = "Analog value 4";
const char sString_AnalogValue5[] PROGMEM = "Analog value 5";
const char sString_AnalogValue6[] PROGMEM = "Analog value 6";
const char sString_AnalogValue7[] PROGMEM = "Analog value 7";
const char sString_AnalogValue8[] PROGMEM = "Analog value 8";
const char sString_AnalogValue9[] PROGMEM = "Analog value 9";
const char sString_AnalogValue10[] PROGMEM = "Analog value 10";
const char sString_AnalogValue11[] PROGMEM = "Analog value 11";
const char sString_AnalogValue12[] PROGMEM = "Analog value 12";
const char sString_AnalogValue13[] PROGMEM = "Analog value 13";
const char sString_AnalogValue14[] PROGMEM = "Analog value 14";
const char sString_AnalogValue15[] PROGMEM = "Analog value 15";
const char sString_AnalogValue16[] PROGMEM = "Analog value 16";

/**
 * This structure holds the values, which are different for each slider created
 */
struct SliderStaticInfoStruct {
    const char *SliderName; // String in PROGMEM
    int16_t MinValue;
    int16_t MaxValue;
    int16_t Threshold;
} sTemporarySliderStaticInfo; // The PROGMEM values to be processed are copied to this instance at runtime
/*
 * PROGMEM array of PROGMEM structures for left sliders
 */
#if defined(ARDUINO) && defined(__AVR__)
static const SliderStaticInfoStruct sLeftSliderStaticInfoArray[NUMBER_OF_SLIDER_AND_BUTTON_LINES] PROGMEM = {
#else
static const SliderStaticInfoStruct sLeftSliderStaticInfoArray[NUMBER_OF_SLIDER_AND_BUTTON_LINES]= {
#endif
        { sString_Brightness, BD_SCREEN_BRIGHTNESS_MIN, BD_SCREEN_BRIGHTNESS_MAX, BD_SCREEN_BRIGHTNESS_MAX }, /*Brightness*/
        { sString_LoopDelay, 0, 1000, 500 }, /*Loop delay*/
        { sString_Variation, 0, 255, 255 }, /*Variation*/
        { sString_PlusMinus, -1000, 1000, 0 }, /*Plus Minus*/
        { sString_Minus, -255, 0, 0 }, /*Minus*/
        { sString_Pattern, 0, 3, 3 }, /*Pattern 0 - 3*/
        { sString_PositiveRange, 100, 200, 200 }, /*PositiveRange*/
        { sString_AnalogValue8, 0, 255, 200 } /*Analog value 8*/
};

/**
 * Current values of the sliders and EEpron storage for them
 */
int16_t sLeftSliderValues[NUMBER_OF_SLIDER_AND_BUTTON_LINES];
#  if defined(EEMEM)
int16_t EEPROMLeftSliderValues[NUMBER_OF_SLIDER_AND_BUTTON_LINES] EEMEM;
#  endif

#if !defined(USE_ONLY_LEFT_SLIDERS)
// PROGMEM array of PROGMEM structures for right sliders

#if defined(ARDUINO) && defined(__AVR__)
static const SliderStaticInfoStruct sRightSliderStaticInfoArray[NUMBER_OF_SLIDER_AND_BUTTON_LINES] PROGMEM = {
#else
static const SliderStaticInfoStruct sRightSliderStaticInfoArray[NUMBER_OF_SLIDER_AND_BUTTON_LINES]= {
#endif
        { sString_AnalogValue9, 0, 255, 200 }, /*Analog value 9*/
        { sString_AnalogValue10, 0, 255, 200 }, /*Analog value */
        { sString_AnalogValue11, 0, 255, 200 }, /*Analog value */
        { sString_AnalogValue12, 0, 255, 200 }, /*Analog value */
        { sString_AnalogValue13, 0, 255, 200 }, /*Analog value */
        { sString_AnalogValue14, 0, 255, 200 }, /*Analog value */
        { sString_AnalogValue15, 0, 255, 200 }, /*Analog value */
        { sString_AnalogValue16, 0, 255, 200 } /*Analog value 16*/
};

// Current values of the sliders and EEprom storage for them
int16_t sRightSliderValues[NUMBER_OF_SLIDER_AND_BUTTON_LINES];
#  if defined(EEMEM)
int16_t EEPROMRightSliderValues[NUMBER_OF_SLIDER_AND_BUTTON_LINES] EEMEM;
#  endif
#endif

/**
 * An enum consisting of indexes of all sliders, used in doSlider() to determine the individual function for each slider.
 * !!!Must be the same order as used in SliderStaticInfoStruct!!!
 */
typedef enum {
    BRIGHTNESS_SLIDER = 0, DELAY_SLIDER, VARIATION_SLIDER, PLUS_MINUS_SLIDER, MINUS_SLIDER, PATTERN_SLIDER, EFFECT_MAX_LENGTH_SLIDER
} slider_Index_t;

/********************
 *     BUTTONS
 ********************/
#define BUTTON_WIDTH            BUTTON_WIDTH_4
#define BUTTONS_START_X         ((DISPLAY_WIDTH - BUTTON_WIDTH) / 2)
#define BUTTON_HEIGHT           (SLIDER_BAR_WIDTH + SLIDER_CAPTION_SIZE)
#define BUTTON_TEXT_SIZE        7

BDButton sButtonArray[NUMBER_OF_SLIDER_AND_BUTTON_LINES];

// PROGMEM text strings for buttons
const char sString_Load[] PROGMEM = "Load\nvalues";
const char sString_Store[] PROGMEM = "Store\nvalues";
const char sString_LedOff[] PROGMEM = "Led off"; // Text for false / red button
const char sString_LedOn[] PROGMEM = "Led on"; // Text for true / green button
const char sString_Test[] PROGMEM = "Test";
const char sString_Button5[] PROGMEM = "Button_5";
const char sString_Button6[] PROGMEM = "Button_6";
const char sString_Button7[] PROGMEM = "Button_7";
const char sString_Button8[] PROGMEM = "Button_8";
/*
 * PROGMEM array of PROGMEM strings for buttons
 */
#if defined(ARDUINO) && defined(__AVR__)
static const char *const sButtonsTextArray[NUMBER_OF_SLIDER_AND_BUTTON_LINES] PROGMEM = {
#else
static const char *const sButtonsTextArray[NUMBER_OF_SLIDER_AND_BUTTON_LINES] = {
#endif
        sString_Load, /*Load values*/
        sString_Store, /*Store values*/
        sString_LedOff, /*Led off*/
        sString_Test, /*Test*/
        sString_Button5, sString_Button6, sString_Button7, sString_Button8 };

/**
 * An enum consisting of indexes of all buttons, used in doButton() to determine the individual function for each button.
 * !!!Must be the same order as used in sButtonsTextArray!!!
 */
typedef enum {
    LOAD_BUTTON = 0, STORE_BUTTON, LED_BUTTON, TEST_BUTTON, BUTTON_5, BUTTON_6, BUTTON_7, BUTTON_8
} button_Index_t;

void initDisplay(void);
void drawDisplay(void);
void doSlider(BDSlider *aTheTouchedSlider, int16_t aSliderValue);
void doButton(BDButton *aTheTouchedButton, int16_t aValue);
void TestNewSliderAndButton();
void doTestSlider(BDSlider *aTheTouchedSlider, int16_t aSliderValue);
void doTestButton(BDButton *aTheTouchedButton, int16_t aValue);
void storeSliderValuesToEEPROM();
void loadSliderValuesFromEEPROM();

#if !defined(STR)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    BlueDisplay1.initCommunication(&Serial, &initDisplay); // initializes Serial and introduces up to 1.5 seconds delay

    Serial.begin(115200);
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/ \
    || defined(SERIALUSB_PID)  || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_attiny3217)
delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#endif
// Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing BD library version " VERSION_BLUE_DISPLAY));

    initDisplay();

    Serial.println(F("Setup finished"));
}

void loop() {
    checkAndHandleEvents(); // check for new data

    /*
     * Your code here
     */

    delay(sLeftSliderValues[DELAY_SLIDER]); // Using value of DELAY_SLIDER
}

void InitSliderFromSliderStaticInfoStruct(BDSlider *aSlider, const SliderStaticInfoStruct *aSliderStaticPGMInfoPtr, int aXPosition,
        int aYPosition) {
    const SliderStaticInfoStruct *tTemporarySliderStaticInfoPtr;

#if defined(ARDUINO) && defined(__AVR__)
// copy PGM data to RAM
    memcpy_P(&sTemporarySliderStaticInfo, aSliderStaticPGMInfoPtr, sizeof(sTemporarySliderStaticInfo));
    tTemporarySliderStaticInfoPtr = &sTemporarySliderStaticInfo;
#else
    tTemporarySliderStaticInfoPtr = aSliderStaticPGMInfoPtr;
#endif

// Initialize slider
    aSlider->init(aXPosition, aYPosition, SLIDER_BAR_WIDTH, SLIDER_BAR_LENGTH, tTemporarySliderStaticInfoPtr->Threshold,
    SLIDER_BAR_LENGTH / 2, SLIDER_BAR_BG_COLOR, SLIDER_BAR_COLOR, FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_SHOW_VALUE, &doSlider);

    /*
     * Set additional values, like min, max, and value and caption size and position
     */
    aSlider->setMinMaxValue(tTemporarySliderStaticInfoPtr->MinValue, tTemporarySliderStaticInfoPtr->MaxValue);

    aSlider->setCaptionProperties(SLIDER_CAPTION_SIZE, FLAG_SLIDER_CAPTION_ALIGN_LEFT_BELOW, SLIDER_CAPTION_MARGIN,
    SLIDER_CAPTION_COLOR, SLIDER_CAPTION_BG_COLOR);
    aSlider->setPrintValueProperties(SLIDER_CAPTION_SIZE, FLAG_SLIDER_VALUE_CAPTION_ALIGN_RIGHT, SLIDER_CAPTION_MARGIN,
    SLIDER_VALUE_COLOR, SLIDER_CAPTION_BG_COLOR);

    aSlider->setCaption(reinterpret_cast<const __FlashStringHelper*>(tTemporarySliderStaticInfoPtr->SliderName));
}

/*
 * Function used as callback handler for connect too
 */
void initDisplay(void) {
// Initialize display size and flags
    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    BDSlider::setDefaultBarThresholdColor(COLOR16_RED);

    /*
     * Set button common parameters
     */
    BDButton::BDButtonPGMTextParameterStruct tBDButtonParameterStruct; // Saves 480 Bytes for all 5 buttons
    BDButton::setInitParameters(&tBDButtonParameterStruct, BUTTONS_START_X, 0, BUTTON_WIDTH, BUTTON_HEIGHT, COLOR16_GREEN, nullptr,
    BUTTON_TEXT_SIZE, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doButton);

// Upper start position of sliders and buttons
    int tYPosition = BUTTON_HEIGHT_6;

    /*
     * Initialize 8 left and optional 8 right sliders and position the 8 button in the middle between the sliders
     */
    for (uint8_t i = 0; i < NUMBER_OF_SLIDER_AND_BUTTON_LINES; i++) {
        // Left + right sliders
        InitSliderFromSliderStaticInfoStruct(&sLeftSliderArray[i], &sLeftSliderStaticInfoArray[i], BUTTON_DEFAULT_SPACING_HALF,
                tYPosition);
#if !defined(USE_ONLY_LEFT_SLIDERS)
        InitSliderFromSliderStaticInfoStruct(&sRightSliderArray[i], &sRightSliderStaticInfoArray[i],
        DISPLAY_WIDTH - (BUTTON_DEFAULT_SPACING_HALF + SLIDER_BAR_LENGTH), tYPosition);
#endif

        // Middle buttons
        tBDButtonParameterStruct.aPositionY = tYPosition;
        tBDButtonParameterStruct.aValue = i;
#if defined(__AVR__)
        tBDButtonParameterStruct.aPGMText = reinterpret_cast<const __FlashStringHelper*>pgm_read_word(&sButtonsTextArray[i]);
#else
        tBDButtonParameterStruct.aPGMText = reinterpret_cast<const __FlashStringHelper*>(sButtonsTextArray[i]);
#endif
        sButtonArray[i].init(&tBDButtonParameterStruct);

        // Prepare for next line
        tYPosition += BUTTON_HEIGHT_8;
    }

    /*
     * Set all slider values to initial position
     */
    loadSliderValuesFromEEPROM();

    /*
     * Convert LED button to a red green toggling one and start with led off
     */
    sButtonArray[LED_BUTTON].setValue(0); // start with led off
    sButtonArray[LED_BUTTON].setTextForValueTrue(reinterpret_cast<const __FlashStringHelper*>(sString_LedOn));

    drawDisplay();
}

void drawDisplay(void) {
    BlueDisplay1.clearDisplay(DISPLAY_BACKGROUND_COLOR);
#if !defined(USE_ONLY_LEFT_SLIDERS)
    BlueDisplay1.drawText(STRING_ALIGN_MIDDLE_XPOS, 16, F("16 Sliders 8 buttons demo"), 16, COLOR16_BLUE, DISPLAY_BACKGROUND_COLOR);
#else
    BlueDisplay1.drawText(STRING_ALIGN_MIDDLE_XPOS, 16, F("8 Sliders 8 buttons demo"), 16, COLOR16_BLUE, DISPLAY_BACKGROUND_COLOR);
#endif

    for (uint8_t i = 0; i < NUMBER_OF_SLIDER_AND_BUTTON_LINES; i++) {
        sLeftSliderArray[i].drawSlider();
        sButtonArray[i].drawButton();
        sRightSliderArray[i].drawSlider();
    }
}

/*
 * Is called by touch or move on a slider and sets the new value
 */
void doSlider(BDSlider *aTheTouchedSlider, int16_t aSliderValue) {
    /*
     * If we use left and right sliders, even slider indexes are from sliders in sLeftSliderStaticInfoArray, odd from sRightSliderStaticInfoArray
     */
    BDSliderIndex_t tSliderIndex = aTheTouchedSlider->mSliderIndex;
#if defined(USE_ONLY_LEFT_SLIDERS)
    sLeftSliderValues[tSliderIndex] = aSliderValue;
    SliderStaticInfoStruct tSliderStaticInfoStructPtr = &sLeftSliderStaticInfoArray[tSliderIndex];
#else
    bool tIsRightSlider = tSliderIndex % 2;
    if (tIsRightSlider) {
        // Odd -> Right sliders
        tSliderIndex /= 2;
        sRightSliderValues[tSliderIndex] = aSliderValue;
    } else {
        // Even -> Left sliders
        tSliderIndex /= 2;
        sLeftSliderValues[tSliderIndex] = aSliderValue;
    }
#endif

    /*
     * alternative search solution if order is unknown
     */
//    for (uint8_t i = 0; i < NUMBER_OF_SLIDER_AND_BUTTON_LINES; i++) {
//        if (sLeftSliderArray[i] == *aTheTouchedSlider) {
//            BlueDisplay1.debug("LeftSliderArray index=", i);
//        }
//    }
#if !defined(USE_ONLY_LEFT_SLIDERS)
    if (!tIsRightSlider) {
#endif
        // Only check left sliders here
        switch (tSliderIndex) {
        case BRIGHTNESS_SLIDER:
            BlueDisplay1.setScreenBrightness(aSliderValue);
            break;
        default:
            char tStringBuffer[28];
            snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("Left slider %2u value=%d"), tSliderIndex, aSliderValue);
            BlueDisplay1.debug(tStringBuffer);
            break;
        }
#if !defined(USE_ONLY_LEFT_SLIDERS)
    } else {
        char tStringBuffer[27];
        snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("Right slider %2u value=%d"), tSliderIndex, aSliderValue);
        BlueDisplay1.debug(tStringBuffer);
    }
#endif
}

void doButton(BDButton *aTheTouchedButton, int16_t aValue) {
    switch (aTheTouchedButton->mButtonIndex) {
    case LOAD_BUTTON:
        loadSliderValuesFromEEPROM();
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
    default:
        BlueDisplay1.debug("Pressed button ", aTheTouchedButton->mButtonIndex);
        break;
    }
}

/*
 * Functions used by Reset and Store buttons
 * Dummy for CPU without EEprom
 */
void storeSliderValuesToEEPROM() {
#if defined(EEMEM)
    eeprom_update_block(sLeftSliderValues, EEPROMLeftSliderValues, sizeof(sLeftSliderValues));
#  if !defined(USE_ONLY_LEFT_SLIDERS)
    eeprom_update_block(sRightSliderValues, EEPROMRightSliderValues, sizeof(sRightSliderValues));
#  endif
#endif
}

void loadSliderValuesFromEEPROM() {
#if defined(EEMEM)
#  if !defined(USE_ONLY_LEFT_SLIDERS)
    eeprom_read_block(sRightSliderValues, EEPROMRightSliderValues, sizeof(sRightSliderValues));
#  endif
    eeprom_read_block(sLeftSliderValues, EEPROMLeftSliderValues, sizeof(sLeftSliderValues));
    for (uint8_t i = 0; i < NUMBER_OF_SLIDER_AND_BUTTON_LINES; i++) {
#  if !defined(USE_ONLY_LEFT_SLIDERS)
        sRightSliderArray[i].setValueAndDrawBar(sRightSliderValues[i]);
#  endif
        sLeftSliderArray[i].setValueAndDrawBar(sLeftSliderValues[i]);
    }
#endif
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
    free(tButton);

    tSlider->removeSlider(DISPLAY_BACKGROUND_COLOR);
    tSlider->deinit();
    free(tSlider);
}

void doTestSlider(BDSlider *aTheTouchedSlider __attribute__((unused)), int16_t aSliderValue) {
    BlueDisplay1.debug("Test slider value ", aSliderValue);
}

void doTestButton(BDButton *aTheTouchedButton __attribute__((unused)), int16_t aButtonValue) {
    BlueDisplay1.debug("Test button value ", aButtonValue);
}

