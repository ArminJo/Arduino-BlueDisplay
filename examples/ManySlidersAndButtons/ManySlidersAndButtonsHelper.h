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
#define SLIDER_BAR_WIDTH        BUTTON_DEFAULT_SPACING_THREE_QUARTER
#define SLIDER_BAR_LENGTH       128
#define SLIDER_CAPTION_SIZE     7
#define SLIDER_CAPTION_MARGIN   2
#define SLIDER_BAR_COLOR        COLOR16_GREEN
#define SLIDER_BAR_BG_COLOR     COLOR16_YELLOW
#define SLIDER_CAPTION_COLOR    COLOR16_BLACK
#define SLIDER_VALUE_COLOR      COLOR16_BLUE
#define SLIDER_CAPTION_BG_COLOR DISPLAY_BACKGROUND_COLOR

/*
 * PROGMEM caption strings for sliders
 */
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
#define BUTTONS_START_X         ((DISPLAY_WIDTH - BUTTON_WIDTH) / 2)
#define BUTTON_HEIGHT           (SLIDER_BAR_WIDTH + SLIDER_CAPTION_SIZE)
#define BUTTON_TEXT_SIZE        7

// PROGMEM text strings for buttons
const char sString_Load[] PROGMEM = "Load\nvalues";
const char sString_Store[] PROGMEM = "Store\nvalues";
const char sString_Button1[] PROGMEM = "Button_1";
const char sString_Button2[] PROGMEM = "Button_2";
const char sString_Button3[] PROGMEM = "Button_3";
const char sString_Button4[] PROGMEM = "Button_4";
const char sString_Button5[] PROGMEM = "Button_5";
const char sString_Button6[] PROGMEM = "Button_6";
const char sString_Button7[] PROGMEM = "Button_7";
const char sString_Button8[] PROGMEM = "Button_8";

void initSlidersAndButtons(const SliderStaticInfoStruct *aLeftSliderStaticPGMInfoPtr,const SliderStaticInfoStruct *aRightSliderStaticPGMInfoPtr);
void drawSlidersAndButtons(void);
void storeSliderValuesToEEPROM();
void loadSliderValuesFromEEPROM();

#if !defined(STR)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif


#endif // _MANY_SLIDER_AND_BUTTONS_HELPER_H
