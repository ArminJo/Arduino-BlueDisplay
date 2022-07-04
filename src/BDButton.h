/*
 * BDButton.h
 *
 *  SUMMARY
 *  Blue Display is an Open Source Android remote Display for Arduino etc.
 *  It receives basic draw requests from Arduino etc. over Bluetooth and renders it.
 *  It also implements basic GUI elements as buttons and sliders.
 *  GUI callback, touch and sensor events are sent back to Arduino.
 *
 *  Copyright (C) 2015-2022  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay https://github.com/ArminJo/android-blue-display.
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

#ifndef _BDBUTTON_H
#define _BDBUTTON_H

#include <stdint.h>

#if defined(ARDUINO)
#  if ! defined(ESP32)
// For not AVR platforms this contains mapping defines (at least for STM32)
#include <avr/pgmspace.h>
#  endif
#include "WString.h"    // for __FlashStringHelper
#endif

#define BUTTON_AUTO_RED_GREEN_FALSE_COLOR COLOR16_RED
#define BUTTON_AUTO_RED_GREEN_TRUE_COLOR COLOR16_GREEN

// Flags for BUTTON_GLOBAL_SETTINGS
static const int FLAG_BUTTON_GLOBAL_USE_DOWN_EVENTS_FOR_BUTTONS = 0x00; // Default
static const int FLAG_BUTTON_GLOBAL_USE_UP_EVENTS_FOR_BUTTONS = 0x01;   // If swipe can start on a button, you require this.
static const int FLAG_BUTTON_GLOBAL_SET_BEEP_TONE = 0x02;   // Beep on button touch

// Flags for init
static const int FLAG_BUTTON_NO_BEEP_ON_TOUCH = 0x00;
static const int FLAG_BUTTON_DO_BEEP_ON_TOUCH = 0x01;  // Beep on this button touch
static const int FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN = 0x02; // Value true -> green, false -> red
static const int FLAG_BUTTON_TYPE_AUTOREPEAT = 0x04;
static const int FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN_MANUAL_REFRESH = 0x0A; // Button must be manually drawn after event to show new caption/color

#if defined(USE_BUTTON_POOL)
#define INTERNAL_FLAG_MASK 0x80
#define FLAG_IS_ALLOCATED 0x80 // For use with get and releaseButton
#endif

/**********************
 * BUTTON WIDTHS
 *********************/
// For documentation
#ifdef __cplusplus
#if ( false ) // It can only be used with compiler flag -std=gnu++11
constexpr int ButtonWidth ( int aNumberOfButtonsPerLine, int aDisplayWidth ) {return ((aDisplayWidth - ((aNumberOfButtonsPerLine-1)*aDisplayWidth/20))/aNumberOfButtonsPerLine);}
#endif
#endif

/**********************
 * BUTTON LAYOUTS
 *********************/
#define BUTTON_DEFAULT_SPACING 16
#define BUTTON_DEFAULT_SPACING_THREE_QUARTER 12
#define BUTTON_DEFAULT_SPACING_HALF 8
#define BUTTON_DEFAULT_SPACING_QUARTER 4

#define BUTTON_HORIZONTAL_SPACING_DYN (BlueDisplay1.mCurrentDisplaySize.XWidth/64)

// for 2 buttons horizontal - 19 characters
#define BUTTON_WIDTH_2 152
#define BUTTON_WIDTH_2_POS_2 (BUTTON_WIDTH_2 + BUTTON_DEFAULT_SPACING)
//
// for 3 buttons horizontal - 12 characters
#define BUTTON_WIDTH_3 96
#define BUTTON_WIDTH_3_POS_2 (BUTTON_WIDTH_3 + BUTTON_DEFAULT_SPACING)
#define BUTTON_WIDTH_3_POS_3 (LAYOUT_320_WIDTH - BUTTON_WIDTH_3)
//
// for 3 buttons horizontal - dynamic
#define BUTTON_WIDTH_3_DYN (BlueDisplay1.mCurrentDisplaySize.XWidth/3 - BUTTON_HORIZONTAL_SPACING_DYN)
#define BUTTON_WIDTH_3_DYN_POS_2 (BlueDisplay1.mCurrentDisplaySize.XWidth/3 + (BUTTON_HORIZONTAL_SPACING_DYN / 2))
#define BUTTON_WIDTH_3_DYN_POS_3 (BlueDisplay1.mCurrentDisplaySize.XWidth - BUTTON_WIDTH_3_DYN)

// width 3.5
#define BUTTON_WIDTH_3_5 82
#define BUTTON_WIDTH_3_5_POS_2 (BUTTON_WIDTH_3_5 + BUTTON_DEFAULT_SPACING)
#define BUTTON_WIDTH_3_5_POS_3 (2*(BUTTON_WIDTH_3_5 + BUTTON_DEFAULT_SPACING))
//
// for 4 buttons horizontal - 8 characters
#define BUTTON_WIDTH_4 68
#define BUTTON_WIDTH_4_POS_2 (BUTTON_WIDTH_4 + BUTTON_DEFAULT_SPACING)
#define BUTTON_WIDTH_4_POS_3 (2*(BUTTON_WIDTH_4 + BUTTON_DEFAULT_SPACING))
#define BUTTON_WIDTH_4_POS_4 (LAYOUT_320_WIDTH - BUTTON_WIDTH_4)
//
// for 4 buttons horizontal - dynamic
#define BUTTON_WIDTH_4_DYN (BlueDisplay1.mCurrentDisplaySize.XWidth/4 - BUTTON_HORIZONTAL_SPACING_DYN)
#define BUTTON_WIDTH_4_DYN_POS_2 (BlueDisplay1.mCurrentDisplaySize.XWidth/4)
#define BUTTON_WIDTH_4_DYN_POS_3 (BlueDisplay1.mCurrentDisplaySize.XWidth/2)
#define BUTTON_WIDTH_4_DYN_POS_4 (BlueDisplay1.mCurrentDisplaySize.XWidth - BUTTON_WIDTH_4_DYN)
//
// for 5 buttons horizontal 51,2  - 6 characters
#define BUTTON_WIDTH_5 51
#define BUTTON_WIDTH_5_POS_2 (BUTTON_WIDTH_5 + BUTTON_DEFAULT_SPACING)
#define BUTTON_WIDTH_5_POS_3 (2*(BUTTON_WIDTH_5 + BUTTON_DEFAULT_SPACING))
#define BUTTON_WIDTH_5_POS_4 (3*(BUTTON_WIDTH_5 + BUTTON_DEFAULT_SPACING))
#define BUTTON_WIDTH_5_POS_5 (LAYOUT_320_WIDTH - BUTTON_WIDTH_5)
//
//  for 2 buttons horizontal plus one small with BUTTON_WIDTH_5 (118,5)- 15 characters
#define BUTTON_WIDTH_2_5 120
#define BUTTON_WIDTH_2_5_POS_2   (BUTTON_WIDTH_2_5 + BUTTON_DEFAULT_SPACING -1)
#define BUTTON_WIDTH_2_5_POS_2_5 (LAYOUT_320_WIDTH - BUTTON_WIDTH_5)
//
// for 6 buttons horizontal
#define BUTTON_WIDTH_6 40
#define BUTTON_WIDTH_6_POS_2 (BUTTON_WIDTH_6 + BUTTON_DEFAULT_SPACING) // 56
#define BUTTON_WIDTH_6_POS_3 (2*(BUTTON_WIDTH_6 + BUTTON_DEFAULT_SPACING)) // 112
#define BUTTON_WIDTH_6_POS_4 (3*(BUTTON_WIDTH_6 + BUTTON_DEFAULT_SPACING)) // 168
#define BUTTON_WIDTH_6_POS_5 (4*(BUTTON_WIDTH_6 + BUTTON_DEFAULT_SPACING)) // 224
#define BUTTON_WIDTH_6_POS_6 (LAYOUT_320_WIDTH - BUTTON_WIDTH_6) // 280
//
// for 6 buttons horizontal - dynamic
#define BUTTON_WIDTH_6_DYN (BlueDisplay1.mCurrentDisplaySize.XWidth/6 - BUTTON_HORIZONTAL_SPACING_DYN)
#define BUTTON_WIDTH_6_DYN_POS_2 (BlueDisplay1.mCurrentDisplaySize.XWidth/6)
#define BUTTON_WIDTH_6_DYN_POS_3 (BlueDisplay1.mCurrentDisplaySize.XWidth/3)
#define BUTTON_WIDTH_6_DYN_POS_4 (BlueDisplay1.mCurrentDisplaySize.XWidth/2)
#define BUTTON_WIDTH_6_DYN_POS_5 ((2*BlueDisplay1.mCurrentDisplaySize.XWidth)/3)
#define BUTTON_WIDTH_6_DYN_POS_6 (BlueDisplay1.mCurrentDisplaySize.XWidth - BUTTON_WIDTH_6_DYN)
//
// for 8 buttons horizontal
#define BUTTON_WIDTH_8 33
#define BUTTON_WIDTH_8_POS_2 (BUTTON_WIDTH_8 + BUTTON_DEFAULT_SPACING_HALF)
#define BUTTON_WIDTH_8_POS_3 (2*(BUTTON_WIDTH_8 + BUTTON_DEFAULT_SPACING_HALF))
#define BUTTON_WIDTH_8_POS_4 (3*(BUTTON_WIDTH_8 + BUTTON_DEFAULT_SPACING_HALF))
#define BUTTON_WIDTH_8_POS_5 (4*(BUTTON_WIDTH_8 + BUTTON_DEFAULT_SPACING_HALF))
#define BUTTON_WIDTH_8_POS_6 (5*(BUTTON_WIDTH_8 + BUTTON_DEFAULT_SPACING_HALF))
#define BUTTON_WIDTH_8_POS_7 (6*(BUTTON_WIDTH_8 + BUTTON_DEFAULT_SPACING_HALF))
#define BUTTON_WIDTH_8_POS_8 (LAYOUT_320_WIDTH - BUTTON_WIDTH_0)
//
// for 10 buttons horizontal
#define BUTTON_WIDTH_10 28
#define BUTTON_WIDTH_10_POS_2 (BUTTON_WIDTH_10 + BUTTON_DEFAULT_SPACING_QUARTER)
#define BUTTON_WIDTH_10_POS_3 (2*(BUTTON_WIDTH_10 + BUTTON_DEFAULT_SPACING_QUARTER))
#define BUTTON_WIDTH_10_POS_4 (3*(BUTTON_WIDTH_10 + BUTTON_DEFAULT_SPACING_QUARTER))
#define BUTTON_WIDTH_10_POS_5 (4*(BUTTON_WIDTH_10 + BUTTON_DEFAULT_SPACING_QUARTER))
#define BUTTON_WIDTH_10_POS_6 (5*(BUTTON_WIDTH_10 + BUTTON_DEFAULT_SPACING_QUARTER))
#define BUTTON_WIDTH_10_POS_7 (6*(BUTTON_WIDTH_10 + BUTTON_DEFAULT_SPACING_QUARTER))
#define BUTTON_WIDTH_10_POS_8 (7*(BUTTON_WIDTH_10 + BUTTON_DEFAULT_SPACING_QUARTER))
#define BUTTON_WIDTH_10_POS_9 (8*(BUTTON_WIDTH_10 + BUTTON_DEFAULT_SPACING_QUARTER))
#define BUTTON_WIDTH_10_POS_10 (LAYOUT_320_WIDTH - BUTTON_WIDTH_0)

#define BUTTON_WIDTH_12 23 // 12*23 + 11*4 = 276 + 44 = 320 :-)

#define BUTTON_WIDTH_14 19 // 19*14 + 13*4 = 266 + 52 = 318

#define BUTTON_WIDTH_16 16
#define BUTTON_WIDTH_16_POS_2 (BUTTON_WIDTH_16 + BUTTON_DEFAULT_SPACING_QUARTER)

/**********************
 * HEIGHTS
 *********************/
#define BUTTON_VERTICAL_SPACING_DYN (BlueDisplay1.mCurrentDisplaySize.YHeight/32)

// for 4 buttons vertical
#define BUTTON_HEIGHT_4 48
#define BUTTON_HEIGHT_4_LINE_2 (BUTTON_HEIGHT_4 + BUTTON_DEFAULT_SPACING) // 64
#define BUTTON_HEIGHT_4_LINE_3 (2*(BUTTON_HEIGHT_4 + BUTTON_DEFAULT_SPACING)) // 128 -> next delta is 64 :-)
#define BUTTON_HEIGHT_4_LINE_4 (LAYOUT_240_HEIGHT - BUTTON_HEIGHT_4) // 192
//
// for 4 buttons vertical and DISPLAY_HEIGHT 256
#define BUTTON_HEIGHT_4_256 52
#define BUTTON_HEIGHT_4_256_LINE_2 (BUTTON_HEIGHT_4_256 + BUTTON_DEFAULT_SPACING) // 68
#define BUTTON_HEIGHT_4_256_LINE_3 (2*(BUTTON_HEIGHT_4_256 + BUTTON_DEFAULT_SPACING))  // 136 -> next delta is 68 :-)
#define BUTTON_HEIGHT_4_256_LINE_4 (LAYOUT_256_HEIGHT - BUTTON_HEIGHT_4_256) // 204
//
// for 4 buttons vertical and variable display height
#define BUTTON_HEIGHT_4_DYN (BlueDisplay1.mCurrentDisplaySize.YHeight/4 - BUTTON_VERTICAL_SPACING_DYN)
#define BUTTON_HEIGHT_4_DYN_LINE_2 (BlueDisplay1.mCurrentDisplaySize.YHeight/4)
#define BUTTON_HEIGHT_4_DYN_LINE_3 (BlueDisplay1.mCurrentDisplaySize.YHeight/2)
#define BUTTON_HEIGHT_4_DYN_LINE_4 (BlueDisplay1.mCurrentDisplaySize.YHeight - BUTTON_HEIGHT_4_DYN)
//
// for 5 buttons vertical
#define BUTTON_HEIGHT_5 38
#define BUTTON_HEIGHT_5_DELTA (BUTTON_HEIGHT_5 + BUTTON_DEFAULT_SPACING_THREE_QUARTER) // 50
#define BUTTON_HEIGHT_5_LINE_2 BUTTON_HEIGHT_5_DELTA
#define BUTTON_HEIGHT_5_LINE_3 (2 * BUTTON_HEIGHT_5_DELTA) // 100
#define BUTTON_HEIGHT_5_LINE_4 ((3 * BUTTON_HEIGHT_5_DELTA) + 1) // 151 -> next delta is 51
#define BUTTON_HEIGHT_5_LINE_5 (LAYOUT_240_HEIGHT - BUTTON_HEIGHT_5) // 202
//
// for 5 buttons vertical and DISPLAY_HEIGHT 256
#define BUTTON_HEIGHT_5_256 40
#define BUTTON_HEIGHT_5_256_DELTA (BUTTON_HEIGHT_5_256 + BUTTON_DEFAULT_SPACING - 2) // 54
#define BUTTON_HEIGHT_5_256_LINE_2 BUTTON_HEIGHT_5_256_DELTA
#define BUTTON_HEIGHT_5_256_LINE_3 (2 * BUTTON_HEIGHT_5_256_DELTA) // 108
#define BUTTON_HEIGHT_5_256_LINE_4 (3 * BUTTON_HEIGHT_5_256_DELTA - 1) // 162 -> next delta is 54
#define BUTTON_HEIGHT_5_256_LINE_5 (LAYOUT_256_HEIGHT - BUTTON_HEIGHT_5_256) // 216
//
// for 5 buttons vertical and variable display height
#define BUTTON_HEIGHT_5_DYN (BlueDisplay1.mCurrentDisplaySize.YHeight/5 - BUTTON_VERTICAL_SPACING_DYN)
#define BUTTON_HEIGHT_5_DYN_LINE_2 (BlueDisplay1.mCurrentDisplaySize.YHeight/5)
#define BUTTON_HEIGHT_5_DYN_LINE_3 ((BlueDisplay1.mCurrentDisplaySize.YHeight/5)*2)
#define BUTTON_HEIGHT_5_DYN_LINE_4 ((BlueDisplay1.mCurrentDisplaySize.YHeight/5)*3)
#define BUTTON_HEIGHT_5_DYN_LINE_5 (BlueDisplay1.mCurrentDisplaySize.YHeight - BUTTON_HEIGHT_5_DYN)
//
// for 6 buttons vertical
#define BUTTON_HEIGHT_6 30
#define BUTTON_HEIGHT_6_DELTA (BUTTON_HEIGHT_6 + BUTTON_DEFAULT_SPACING_THREE_QUARTER) // 42
#define BUTTON_HEIGHT_6_LINE_2 BUTTON_HEIGHT_6_DELTA
#define BUTTON_HEIGHT_6_LINE_3 (2 * BUTTON_HEIGHT_6_DELTA)
#define BUTTON_HEIGHT_6_LINE_4 (3 * BUTTON_HEIGHT_6_DELTA)
#define BUTTON_HEIGHT_6_LINE_5 (4 * BUTTON_HEIGHT_6_DELTA) // 168 -> next delta is 42
#define BUTTON_HEIGHT_6_LINE_6 (LAYOUT_240_HEIGHT - BUTTON_HEIGHT_6) // 210

// for 8 buttons vertical and font size 22
#define BUTTON_HEIGHT_8 24
#define BUTTON_HEIGHT_8_DELTA (BUTTON_HEIGHT_8 + (BUTTON_DEFAULT_SPACING_HALF - 1)) // 31
#define BUTTON_HEIGHT_8_LINE_2 BUTTON_HEIGHT_8_DELTA
#define BUTTON_HEIGHT_8_LINE_3 (2 * BUTTON_HEIGHT_8_DELTA)
#define BUTTON_HEIGHT_8_LINE_4 (3 * BUTTON_HEIGHT_8_DELTA)
#define BUTTON_HEIGHT_8_LINE_5 (4 * BUTTON_HEIGHT_8_DELTA) // 124
#define BUTTON_HEIGHT_8_LINE_6 (5 * BUTTON_HEIGHT_8_DELTA) // 155
#define BUTTON_HEIGHT_8_LINE_7 (6 * BUTTON_HEIGHT_8_DELTA) // 186 -> next delta is 30
#define BUTTON_HEIGHT_8_LINE_8 (LAYOUT_240_HEIGHT - BUTTON_HEIGHT_8) // 216

#define BUTTON_HEIGHT_10 20

#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
#include "TouchButton.h"
class TouchButton;
#endif
#  if defined(AVR)
typedef uint8_t BDButtonHandle_t;
#  else
typedef uint16_t BDButtonHandle_t;
#  endif


extern BDButtonHandle_t sLocalButtonIndex; // local button index counter used by BDButton.init() and BlueDisplay.createButton()

#include "Colors.h"

#ifdef __cplusplus
class BDButton {
public:

    static void resetAllButtons(void);
    static void activateAllButtons(void);
    static void deactivateAllButtons(void);
    static void setButtonsTouchTone(uint8_t aToneIndex, uint16_t aToneDuration);
    static void setButtonsTouchTone(uint8_t aToneIndex, uint16_t aToneDuration, uint8_t aToneVolume);
    static void setGlobalFlags(uint16_t aFlags);

    // Constructors
    BDButton();
    BDButton(BDButtonHandle_t aButtonHandle);
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    BDButton(BDButtonHandle_t aButtonHandle, TouchButton *aLocalButtonPtr);
#endif
    BDButton(const BDButton &aButton);
    // Operators
    bool operator==(const BDButton &aButton);
    //
    bool operator!=(const BDButton &aButton);

    void init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor,
            const char *aCaption, uint16_t aCaptionSize, uint8_t aFlags, int16_t aValue,
            void (*aOnTouchHandler)(BDButton*, int16_t));
//    void init(const struct ButtonInit *aButtonInfo, const __FlashStringHelper *aPGMCaption);
//    void init(const struct ButtonInit *aButtonInfo, const __FlashStringHelper *aPGMCaption, int16_t aValue);

    void setPosition(int16_t aPositionX, int16_t aPositionY);

    void drawButton(void);
    void removeButton(color16_t aBackgroundColor);
    void drawCaption(void);
    void setCaption(const char *aCaption, bool doDrawButton = false);
    void setCaptionFromStringArray(const char *const aCaptionStringArrayPtr[], uint8_t aStringIndex, bool doDrawButton);
    void setCaptionForValueTrue(const char *aCaption);
    void setValue(int16_t aValue, bool doDrawButton = false);
    void setValueAndDraw(int16_t aValue);
    void setButtonColor(color16_t aButtonColor);
    void setButtonColorAndDraw(color16_t aButtonColor);
    void setButtonAutorepeatTiming(uint16_t aMillisFirstDelay, uint16_t aMillisFirstRate, uint16_t aFirstCount,
            uint16_t aMillisSecondRate);

    void activate(void);
    void deactivate(void);

#if defined(ARDUINO)
    void initPGM(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor,
            const char *aPGMCaption, uint8_t aCaptionSize, uint8_t aFlags, int16_t aValue,
            void (*aOnTouchHandler)(BDButton*, int16_t));

    void setCaptionPGMForValueTrue(const char *aCaption);
    void setCaptionPGM(const char *aPGMCaption, bool doDrawButton = false);
    void setCaptionFromStringArrayPGM(const char *const aPGMCaptionStringArrayPtr[], uint8_t aStringIndex,
            bool doDrawButton = false);

    void init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor,
            const __FlashStringHelper *aPGMCaption, uint8_t aCaptionSize, uint8_t aFlags, int16_t aValue,
            void (*aOnTouchHandler)(BDButton*, int16_t));
    void init(uint16_t aPositionX, uint16_t aPositionY, const __FlashStringHelper *aPGMCaption, int16_t aValue,
            void (*aOnTouchHandler)(BDButton*, int16_t));
    void setCaptionForValueTrue(const __FlashStringHelper *aCaption);
    void setCaption(const __FlashStringHelper *aPGMCaption, bool doDrawButton = false);
#endif // defined(ARDUINO)

    BDButtonHandle_t mButtonHandle; // Index for BlueDisplay button functions. Taken in init() from sLocalButtonIndex.

#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    void deinit(void);
    TouchButton *mLocalButtonPtr;
#endif

private:
};

///**
// * @brief Button Init Structure definition
// * This uses around 200 bytes and saves 8 to 24 bytes per button
// */
//struct ButtonInit {
//    uint16_t PositionX;
//    uint16_t PositionY;
//    uint16_t WidthX;
//    uint16_t HeightY;
//    color16_t ButtonColor;
//    uint16_t CaptionSize;
//    uint16_t Flags;
//    int16_t Value;
//    void (*aOnTouchHandler)(BDButton*, int16_t);
////    const __FlashStringHelper *PGMCaption;
//};
#endif // #ifdef __cplusplus

#endif //_BDBUTTON_H
