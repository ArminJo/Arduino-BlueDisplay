/*
 * GUIHelper.h
 *
 * Definitions for GUI layouts and text sizes
 *
 *
 *  Copyright (C) 2014-2022  Armin Joachimsmeyer
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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#ifndef _GUI_HELPER_H
#define _GUI_HELPER_H

#include <stdint.h>

#include "LayoutHelper.h"

/***************************
 * Origin 0.0 is upper left
 **************************/
#define DISPLAY_HALF_VGA_HEIGHT 240
#define DISPLAY_HALF_VGA_WIDTH  320
#define DISPLAY_VGA_HEIGHT      480
#define DISPLAY_VGA_WIDTH       640
#define DISPLAY_DEFAULT_HEIGHT  DISPLAY_HALF_VGA_HEIGHT // 240 - value to use if not connected
#define DISPLAY_DEFAULT_WIDTH   DISPLAY_HALF_VGA_WIDTH  // 320 - value to use if not connected
#define STRING_BUFFER_STACK_SIZE 32 // Size for buffer allocated on stack with "char tStringBuffer[STRING_BUFFER_STACK_SIZE]" for ...PGM() functions.
#define STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE 34 // Size for buffer allocated on stack with "char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG]" for debug(const char *aMessage,...) functions.

#if !defined(BACKGROUND_COLOR)
#define BACKGROUND_COLOR        COLOR16_WHITE
#endif

/*
 * Some useful text sizes constants
 */
#if defined(SUPPORT_ONLY_TEXT_SIZE_11_AND_22)
#define TEXT_SIZE_11 11
#define TEXT_SIZE_22 22
#else
#define TEXT_SIZE_8   8
#define TEXT_SIZE_9   9
#define TEXT_SIZE_10 10
#define TEXT_SIZE_11 11
#define TEXT_SIZE_12 12
#define TEXT_SIZE_13 13
#define TEXT_SIZE_14 14
#define TEXT_SIZE_16 16
#define TEXT_SIZE_18 18
#define TEXT_SIZE_20 20
#define TEXT_SIZE_22 22
#define TEXT_SIZE_26 26
// for factor 3 of 8*12 font
#define TEXT_SIZE_33 33
// for factor 4 of 8*12 font
#define TEXT_SIZE_44 44
#endif

// TextWidth = TextSize * 0.6
#if defined(SUPPORT_LOCAL_DISPLAY)
// 8/16 instead of 7/13 to be compatible with 8*12 font
#define TEXT_SIZE_11_WIDTH 8
#define TEXT_SIZE_22_WIDTH 16
#else
#define TEXT_SIZE_11_WIDTH 7
#define TEXT_SIZE_12_WIDTH 7
#define TEXT_SIZE_13_WIDTH 8
#define TEXT_SIZE_14_WIDTH 8
#define TEXT_SIZE_16_WIDTH 10
#define TEXT_SIZE_18_WIDTH 11
#define TEXT_SIZE_22_WIDTH 13
#define TEXT_SIZE_33_WIDTH 20
#define TEXT_SIZE_44_WIDTH 26
#endif

// TextSize * 1,125 ( 1 + 1/8)
// 12 instead of 11 to be compatible with 8*12 font and have a margin
#define TEXT_SIZE_10_HEIGHT 11
#define TEXT_SIZE_11_HEIGHT 12
#define TEXT_SIZE_12_HEIGHT 13
#define TEXT_SIZE_14_HEIGHT 15
#define TEXT_SIZE_16_HEIGHT 18
#define TEXT_SIZE_18_HEIGHT 20
#define TEXT_SIZE_20_HEIGHT 22
#define TEXT_SIZE_22_HEIGHT 24
#define TEXT_SIZE_33_HEIGHT 36
#define TEXT_SIZE_44_HEIGHT 48

// TextSize * 0.76
// TextSize * 0.855 to have ASCEND + DECEND = HEIGHT
// 9 instead of 8 to have ASCEND + DECEND = HEIGHT
#define TEXT_SIZE_11_ASCEND 9
#define TEXT_SIZE_12_ASCEND 9
#define TEXT_SIZE_13_ASCEND 10
#define TEXT_SIZE_14_ASCEND 11
#define TEXT_SIZE_16_ASCEND 12
#define TEXT_SIZE_18_ASCEND 14
// 18 instead of 17 to have ASCEND + DECEND = HEIGHT
#define TEXT_SIZE_22_ASCEND 18
#define TEXT_SIZE_33_ASCEND 28
#define TEXT_SIZE_44_ASCEND 37

// TextSize * 0.24
// TextSize * 0.27 to have ASCEND + DECEND = HEIGHT
#define TEXT_SIZE_11_DECEND 3
#define TEXT_SIZE_11_DECEND 3
// 6 instead of 5 to have ASCEND + DECEND = HEIGHT
#define TEXT_SIZE_22_DECEND 6
#define TEXT_SIZE_33_DECEND 8
#define TEXT_SIZE_44_DECEND 11

uint16_t getTextHeight(uint16_t aTextSize);
uint16_t getTextWidth(uint16_t aTextSize);
uint16_t getTextAscend(uint16_t aTextSize);
uint16_t getTextAscendMinusDescend(uint16_t aTextSize);
uint16_t getTextMiddle(uint16_t aTextSize);

uint16_t getFontScaleFactorFromTextSize(uint16_t aTextSize);

/*
 * Layout for 320 x 240 screen size
 */
#define LAYOUT_320_WIDTH 320
#define LAYOUT_240_HEIGHT 240
#define LAYOUT_256_HEIGHT 256

#endif // _GUI_HELPER_H
