/*
 *
 * ThickLine.h
 *
 *  Copyright (C) 2013-2026  Armin Joachimsmeyer
 *
 *  This file is part of Arduino-BlueDisplay https://github.com/ArminJo/Arduino-BlueDisplay.
 *  This file is part of android-blue-display https://github.com/ArminJo/android-blue-display.
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

#ifndef _THICKLINE_H
#define _THICKLINE_H

#include <stdint.h>

/*
 * Overlap means drawing additional pixel when changing minor direction
 * Needed for drawThickLine, otherwise some pixels will be missing in the thick line
 */
#define LINE_OVERLAP_NONE 0 	// No line overlap, like in standard Bresenham
#define LINE_OVERLAP_MAJOR 0x01 // Overlap - first go major then minor direction. Pixel is drawn as extension after actual line
#define LINE_OVERLAP_MINOR 0x02 // Overlap - first go minor then major direction. Pixel is drawn as extension before next line
#define LINE_OVERLAP_BOTH 0x03  // Overlap - both

#define LINE_THICKNESS_MIDDLE 0                 // Start point is on the line at center of the thick line
#define LINE_THICKNESS_DRAW_CLOCKWISE 1         // Start point is on the counter clockwise border line
#define LINE_THICKNESS_DRAW_COUNTERCLOCKWISE 2  // Start point is on the clockwise border line

#ifdef __cplusplus
extern "C" {
#endif

void drawLineOverlap(unsigned int aXStart, unsigned int aYStart, unsigned int aXEnd, unsigned int aYEnd, uint8_t aOverlap, uint16_t aColor);
void drawThickLine(unsigned int aXStart, unsigned int aYStart, unsigned int aXEnd, unsigned int aYEnd, unsigned int aThickness, uint8_t aThicknessMode,
        uint16_t aColor);
void drawThickLineSimple(unsigned int aXStart, unsigned int aYStart, unsigned int aXEnd, unsigned int aYEnd, unsigned int aThickness, uint8_t aThicknessMode,
        uint16_t aColor);

#ifdef __cplusplus
}
#endif

#endif // _THICKLINE_H
