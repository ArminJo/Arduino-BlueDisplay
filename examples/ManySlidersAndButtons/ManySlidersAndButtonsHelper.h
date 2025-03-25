/*
 * ManySlidersAndButtonsHelper.h
 *
 * Definitions for helper code for ManySlidersAndButtons example.
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

#ifndef _MANY_SLIDER_AND_BUTTONS_HELPER_H
#define _MANY_SLIDER_AND_BUTTONS_HELPER_H

/********************
 *      SLIDERS
 ********************/
#if !defined(SLIDER_BAR_WIDTH)
#define SLIDER_BAR_WIDTH        12
#endif
#if !defined(SLIDER_BAR_LENGTH)
#define SLIDER_BAR_LENGTH       128
#endif
#if !defined(SLIDER_CAPTION_SIZE)
#define SLIDER_CAPTION_SIZE     7
#endif
#if !defined(SLIDER_CAPTION_MARGIN)
#define SLIDER_CAPTION_MARGIN   2
#endif
#if !defined(SLIDER_BAR_COLOR)
#define SLIDER_BAR_COLOR        COLOR16_GREEN
#endif
#if !defined(SLIDER_BAR_BG_COLOR)
#define SLIDER_BAR_BG_COLOR     COLOR16_YELLOW
#endif
#if !defined(SLIDER_CAPTION_COLOR)
#define SLIDER_CAPTION_COLOR    COLOR16_BLACK
#endif
#if !defined(SLIDER_VALUE_COLOR)
#define SLIDER_VALUE_COLOR      COLOR16_BLUE
#endif
#if !defined(SLIDER_CAPTION_BG_COLOR)
#define SLIDER_CAPTION_BG_COLOR DISPLAY_BACKGROUND_COLOR
#endif

/*
 * PROGMEM caption strings for sliders
 */
const char sString_ParameterValue1[] PROGMEM = "Parameter value 1";
const char sString_ParameterValue2[] PROGMEM = "Parameter value 2";
const char sString_ParameterValue3[] PROGMEM = "Parameter value 3";
const char sString_ParameterValue4[] PROGMEM = "Parameter value 4";
const char sString_ParameterValue5[] PROGMEM = "Parameter value 5";
const char sString_ParameterValue6[] PROGMEM = "Parameter value 6";
const char sString_ParameterValue7[] PROGMEM = "Parameter value 7";
const char sString_ParameterValue8[] PROGMEM = "Parameter value 8";
const char sString_ParameterValue9[] PROGMEM = "Parameter value 9";
const char sString_ParameterValue10[] PROGMEM = "Parameter value 10";
const char sString_ParameterValue11[] PROGMEM = "Parameter value 11";
const char sString_ParameterValue12[] PROGMEM = "Parameter value 12";
const char sString_ParameterValue13[] PROGMEM = "Parameter value 13";
const char sString_ParameterValue14[] PROGMEM = "Parameter value 14";
const char sString_ParameterValue15[] PROGMEM = "Parameter value 15";
const char sString_ParameterValue16[] PROGMEM = "Parameter value 16";

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
};

/********************
 *     BUTTONS
 ********************/
#define BUTTON_WIDTH            BUTTON_WIDTH_4
#if NUMBER_OF_RIGHT_SLIDERS > 0
#define BUTTONS_START_X         ((DISPLAY_WIDTH - BUTTON_WIDTH) / 2)
#else
#define BUTTONS_START_X         (SLIDER_BAR_LENGTH + BUTTON_DEFAULT_SPACING)
#endif
#define BUTTON_HEIGHT           (SLIDER_BAR_WIDTH + SLIDER_CAPTION_SIZE)
#define BUTTON_TEXT_SIZE        7

// PROGMEM text strings for buttons
const char sString_Load[] PROGMEM = "Load\nvalues";
const char sString_Store[] PROGMEM = "Store\nvalues";
const char sString_LedOn[] PROGMEM = "LED On";
const char sString_LedOff[] PROGMEM = "LED Off";
const char sString_On[] PROGMEM = "On";
const char sString_Off[] PROGMEM = "Off";
const char sString_Button1[] PROGMEM = "Button_1";
const char sString_Button2[] PROGMEM = "Button_2";
const char sString_Button3[] PROGMEM = "Button_3";
const char sString_Button4[] PROGMEM = "Button_4";
const char sString_Button5[] PROGMEM = "Button_5";
const char sString_Button6[] PROGMEM = "Button_6";
const char sString_Button7[] PROGMEM = "Button_7";
const char sString_Button8[] PROGMEM = "Button_8";

struct ButtonStaticInfoStruct {
    int16_t Value;
    const char *ButtonText; // String in PROGMEM
    const char *ButtonTextForValueTrue; // if not 0 | NULL | nullptr a Red/Green toggle button is created
};

/********************
 *     COMMON
 ********************/
#if !defined(SLIDER_AND_BUTTON_START_Y)
#define SLIDER_AND_BUTTON_START_Y   BUTTON_HEIGHT_6
#endif
#if !defined(SLIDER_AND_BUTTON_DELTA_Y)
#define SLIDER_AND_BUTTON_DELTA_Y   (2 * SLIDER_BAR_WIDTH)
#endif

void initSlidersAndButtons(const SliderStaticInfoStruct *aLeftSliderStaticPGMInfoPtr,
        const SliderStaticInfoStruct *aRightSliderStaticPGMInfoPtr);
void drawSlidersAndButtons(void);
uint8_t copyPGMStringStoredInPGMVariable(char *aStringBuffer, void *aPGMStringPtrStoredInPGMVariable);
void storeSliderValuesToEEPROM();
void loadSliderValuesFromEEPROM();

#if !defined(STR)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif

#endif // _MANY_SLIDER_AND_BUTTONS_HELPER_H
