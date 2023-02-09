/*
 * LayoutHelper.h
 *
 * Defines for different button layouts
 *
 *  Copyright (C) 2023  Armin Joachimsmeyer
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

#ifndef _LAYOUT_HELPER_H
#define _LAYOUT_HELPER_H


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

#endif //_LAYOUT_HELPER_H
