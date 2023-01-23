/**
 * thickLine.hpp
 *
 *  @brief Draw a solid line with thickness using a modified Bresenhams algorithm.
 *
 *  Copyright (C) 2013-2022  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of STMF3-Discovery-Demos https://github.com/ArminJo/STMF3-Discovery-Demos/blob/master/lib/graphics/src/thickLine.cpp.
 *                                             https://gist.github.com/ArminJo/8dc4e61847a693e99bdde919cc7005cc
 *
 *  STMF3-Discovery-Demos is free software: you can redistribute it and/or modify
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
 */

#ifndef _THICKLINE_HPP
#define _THICKLINE_HPP

#include "LocalGUI/ThickLine.h"

/*
 * The graphic primitives used by thickline.
 * fillRect() is exclusively used for horizontal and vertical lines, because is implementation (on my system) is way faster than drawLine().
 * drawLine() is exclusively used for the start / base line of a thick line and can be replaced by drawLineOverlap(..., LINE_OVERLAP_NONE, aColor)
 */
void drawPixel(uint16_t aXPos, uint16_t aYPos, uint16_t aColor);
void drawLine(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor);
void fillRect(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor);

/** @addtogroup Graphic_Library
 * @{
 */

/**
 * Modified Bresenham draw(line) with optional overlap. Required for drawThickLine().
 * Overlap draws additional pixel when changing minor direction. For standard bresenham overlap, choose LINE_OVERLAP_NONE (0).
 *
 *  Sample line:
 *
 *    00+
 *     -0000+
 *         -0000+
 *             -00
 *
 *  0 pixels are drawn for normal line without any overlap LINE_OVERLAP_NONE
 *  + pixels are drawn if LINE_OVERLAP_MAJOR
 *  - pixels are drawn if LINE_OVERLAP_MINOR
 */

#if !defined(DISPLAY_HEIGHT)
#define DISPLAY_HEIGHT  240
#define DISPLAY_WIDTH   320
#endif
/**
 * Draws a line from aXStart/aYStart to aXEnd/aYEnd including both ends
 * @param aOverlap One of LINE_OVERLAP_NONE, LINE_OVERLAP_MAJOR, LINE_OVERLAP_MINOR, LINE_OVERLAP_BOTH
 */
void drawLineOverlap(unsigned int aXStart, unsigned int aYStart, unsigned int aXEnd, unsigned int aYEnd, uint8_t aOverlap,
        uint16_t aColor) {
    int16_t tDeltaX, tDeltaY, tDeltaXTimes2, tDeltaYTimes2, tError, tStepX, tStepY;

    /*
     * Clip to display size
     */
    if (aXStart >= DISPLAY_WIDTH) {
        aXStart = DISPLAY_WIDTH - 1;
    }

    if (aXEnd >= DISPLAY_WIDTH) {
        aXEnd = DISPLAY_WIDTH - 1;
    }

    if (aYStart >= DISPLAY_HEIGHT) {
        aYStart = DISPLAY_HEIGHT - 1;
    }

    if (aYEnd >= DISPLAY_HEIGHT) {
        aYEnd = DISPLAY_HEIGHT - 1;
    }

    if ((aXStart == aXEnd) || (aYStart == aYEnd)) {
        // horizontal or vertical line -> fillRect() is faster than drawLine()
        fillRect(aXStart, aYStart, aXEnd, aYEnd, aColor); // you can remove the check and this line if you have no fillRect() or drawLine() available.
    } else {
        // calculate direction
        tDeltaX = aXEnd - aXStart;
        tDeltaY = aYEnd - aYStart;
        if (tDeltaX < 0) {
            tDeltaX = -tDeltaX;
            tStepX = -1;
        } else {
            tStepX = +1;
        }
        if (tDeltaY < 0) {
            tDeltaY = -tDeltaY;
            tStepY = -1;
        } else {
            tStepY = +1;
        }
        tDeltaXTimes2 = tDeltaX << 1;
        tDeltaYTimes2 = tDeltaY << 1;
        // draw start pixel
        drawPixel(aXStart, aYStart, aColor);
        if (tDeltaX > tDeltaY) {
            // start value represents a half step in Y direction
            tError = tDeltaYTimes2 - tDeltaX;
            while (aXStart != aXEnd) {
                // step in main direction
                aXStart += tStepX;
                if (tError >= 0) {
                    if (aOverlap & LINE_OVERLAP_MAJOR) {
                        // draw pixel in main direction before changing
                        drawPixel(aXStart, aYStart, aColor);
                    }
                    // change Y
                    aYStart += tStepY;
                    if (aOverlap & LINE_OVERLAP_MINOR) {
                        // draw pixel in minor direction before changing
                        drawPixel(aXStart - tStepX, aYStart, aColor);
                    }
                    tError -= tDeltaXTimes2;
                }
                tError += tDeltaYTimes2;
                drawPixel(aXStart, aYStart, aColor);
            }
        } else {
            tError = tDeltaXTimes2 - tDeltaY;
            while (aYStart != aYEnd) {
                aYStart += tStepY;
                if (tError >= 0) {
                    if (aOverlap & LINE_OVERLAP_MAJOR) {
                        // draw pixel in main direction before changing
                        drawPixel(aXStart, aYStart, aColor);
                    }
                    aXStart += tStepX;
                    if (aOverlap & LINE_OVERLAP_MINOR) {
                        // draw pixel in minor direction before changing
                        drawPixel(aXStart, aYStart - tStepY, aColor);
                    }
                    tError -= tDeltaYTimes2;
                }
                tError += tDeltaXTimes2;
                drawPixel(aXStart, aYStart, aColor);
            }
        }
    }
}

/**
 * Bresenham with thickness
 * No pixel missed and every pixel only drawn once!
 * The code is bigger and more complicated than drawThickLineSimple() but it tends to be faster, since drawing a pixel is often a slow operation.
 * aThicknessMode can be one of LINE_THICKNESS_MIDDLE, LINE_THICKNESS_DRAW_CLOCKWISE, LINE_THICKNESS_DRAW_COUNTERCLOCKWISE
 */
void drawThickLine(unsigned int aXStart, unsigned int aYStart, unsigned int aXEnd, unsigned int aYEnd, unsigned int aThickness,
        uint8_t aThicknessMode, uint16_t aColor) {
    int16_t i, tDeltaX, tDeltaY, tDeltaXTimes2, tDeltaYTimes2, tError, tStepX, tStepY;

    if (aThickness <= 1) {
        drawLineOverlap(aXStart, aYStart, aXEnd, aYEnd, LINE_OVERLAP_NONE, aColor);
    }
    /*
     * Clip to display size
     */
    if (aXStart >= DISPLAY_WIDTH) {
        aXStart = DISPLAY_WIDTH - 1;
    }

    if (aXEnd >= DISPLAY_WIDTH) {
        aXEnd = DISPLAY_WIDTH - 1;
    }

    if (aYStart >= DISPLAY_HEIGHT) {
        aYStart = DISPLAY_HEIGHT - 1;
    }

    if (aYEnd >= DISPLAY_HEIGHT) {
        aYEnd = DISPLAY_HEIGHT - 1;
    }

    /**
     * For coordinate system with 0.0 top left
     * Swap X and Y delta and calculate clockwise (new delta X inverted)
     * or counterclockwise (new delta Y inverted) rectangular direction.
     * The right rectangular direction for LINE_OVERLAP_MAJOR toggles with each octant
     */
    tDeltaY = aXEnd - aXStart;
    tDeltaX = aYEnd - aYStart;
    // mirror 4 quadrants to one and adjust deltas and stepping direction
    bool tSwap = true; // count effective mirroring
    if (tDeltaX < 0) {
        tDeltaX = -tDeltaX;
        tStepX = -1;
        tSwap = !tSwap;
    } else {
        tStepX = +1;
    }
    if (tDeltaY < 0) {
        tDeltaY = -tDeltaY;
        tStepY = -1;
        tSwap = !tSwap;
    } else {
        tStepY = +1;
    }
    tDeltaXTimes2 = tDeltaX << 1;
    tDeltaYTimes2 = tDeltaY << 1;
    bool tOverlap;
    // adjust for right direction of thickness from line origin
    int tDrawStartAdjustCount = aThickness / 2;
    if (aThicknessMode == LINE_THICKNESS_DRAW_COUNTERCLOCKWISE) {
        tDrawStartAdjustCount = aThickness - 1;
    } else if (aThicknessMode == LINE_THICKNESS_DRAW_CLOCKWISE) {
        tDrawStartAdjustCount = 0;
    }

    /*
     * Now tDelta* are positive and tStep* define the direction
     * tSwap is false if we mirrored only once
     */
    // which octant are we now
    if (tDeltaX >= tDeltaY) {
        // Octant 1, 3, 5, 7 (between 0 and 45, 90 and 135, ... degree)
        if (tSwap) {
            tDrawStartAdjustCount = (aThickness - 1) - tDrawStartAdjustCount;
            tStepY = -tStepY;
        } else {
            tStepX = -tStepX;
        }
        /*
         * Vector for draw direction of the starting points of lines is rectangular and counterclockwise to main line direction
         * Therefore no pixel will be missed if LINE_OVERLAP_MAJOR is used on change in minor rectangular direction
         */
        // adjust draw start point
        tError = tDeltaYTimes2 - tDeltaX;
        for (i = tDrawStartAdjustCount; i > 0; i--) {
            // change X (main direction here)
            aXStart -= tStepX;
            aXEnd -= tStepX;
            if (tError >= 0) {
                // change Y
                aYStart -= tStepY;
                aYEnd -= tStepY;
                tError -= tDeltaXTimes2;
            }
            tError += tDeltaYTimes2;
        }
        // draw start line. We can alternatively use drawLineOverlap(aXStart, aYStart, aXEnd, aYEnd, LINE_OVERLAP_NONE, aColor) here.
        drawLine(aXStart, aYStart, aXEnd, aYEnd, aColor);
        // draw aThickness number of lines
        tError = tDeltaYTimes2 - tDeltaX;
        for (i = aThickness; i > 1; i--) {
            // change X (main direction here)
            aXStart += tStepX;
            aXEnd += tStepX;
            tOverlap = LINE_OVERLAP_NONE;
            if (tError >= 0) {
                // change Y
                aYStart += tStepY;
                aYEnd += tStepY;
                tError -= tDeltaXTimes2;
                /*
                 * Change minor direction reverse to line (main) direction
                 * because of choosing the right (counter)clockwise draw vector
                 * Use LINE_OVERLAP_MAJOR to fill all pixel
                 *
                 * EXAMPLE:
                 * 1,2 = Pixel of first 2 lines
                 * 3 = Pixel of third line in normal line mode
                 * - = Pixel which will additionally be drawn in LINE_OVERLAP_MAJOR mode
                 *           33
                 *       3333-22
                 *   3333-222211
                 * 33-22221111
                 *  221111                     /\
                 *  11                          Main direction of start of lines draw vector
                 *  -> Line main direction
                 *  <- Minor direction of counterclockwise of start of lines draw vector
                 */
                tOverlap = LINE_OVERLAP_MAJOR;
            }
            tError += tDeltaYTimes2;
            drawLineOverlap(aXStart, aYStart, aXEnd, aYEnd, tOverlap, aColor);
        }
    } else {
        // the other octant 2, 4, 6, 8 (between 45 and 90, 135 and 180, ... degree)
        if (tSwap) {
            tStepX = -tStepX;
        } else {
            tDrawStartAdjustCount = (aThickness - 1) - tDrawStartAdjustCount;
            tStepY = -tStepY;
        }
        // adjust draw start point
        tError = tDeltaXTimes2 - tDeltaY;
        for (i = tDrawStartAdjustCount; i > 0; i--) {
            aYStart -= tStepY;
            aYEnd -= tStepY;
            if (tError >= 0) {
                aXStart -= tStepX;
                aXEnd -= tStepX;
                tError -= tDeltaYTimes2;
            }
            tError += tDeltaXTimes2;
        }
        //draw start line
        drawLine(aXStart, aYStart, aXEnd, aYEnd, aColor);
        // draw aThickness number of lines
        tError = tDeltaXTimes2 - tDeltaY;
        for (i = aThickness; i > 1; i--) {
            aYStart += tStepY;
            aYEnd += tStepY;
            tOverlap = LINE_OVERLAP_NONE;
            if (tError >= 0) {
                aXStart += tStepX;
                aXEnd += tStepX;
                tError -= tDeltaYTimes2;
                tOverlap = LINE_OVERLAP_MAJOR;
            }
            tError += tDeltaXTimes2;
            drawLineOverlap(aXStart, aYStart, aXEnd, aYEnd, tOverlap, aColor);
        }
    }
}
/**
 * The same as before, but no clipping to display range, some pixel are drawn twice (because of using LINE_OVERLAP_BOTH)
 * and direction of thickness changes for each octant (except for LINE_THICKNESS_MIDDLE and aThickness value is odd)
 * aThicknessMode can be LINE_THICKNESS_MIDDLE or any other value
 *
 */
void drawThickLineSimple(unsigned int aXStart, unsigned int aYStart, unsigned int aXEnd, unsigned int aYEnd,
        unsigned int aThickness, uint8_t aThicknessMode, uint16_t aColor) {
    int16_t i, tDeltaX, tDeltaY, tDeltaXTimes2, tDeltaYTimes2, tError, tStepX, tStepY;

    tDeltaY = aXStart - aXEnd;
    tDeltaX = aYEnd - aYStart;
    // mirror 4 quadrants to one and adjust deltas and stepping direction
    if (tDeltaX < 0) {
        tDeltaX = -tDeltaX;
        tStepX = -1;
    } else {
        tStepX = +1;
    }
    if (tDeltaY < 0) {
        tDeltaY = -tDeltaY;
        tStepY = -1;
    } else {
        tStepY = +1;
    }
    tDeltaXTimes2 = tDeltaX << 1;
    tDeltaYTimes2 = tDeltaY << 1;
    bool tOverlap;
    // which octant are we now
    if (tDeltaX > tDeltaY) {
        if (aThicknessMode == LINE_THICKNESS_MIDDLE) {
            // adjust draw start point
            tError = tDeltaYTimes2 - tDeltaX;
            for (i = aThickness / 2; i > 0; i--) {
                // change X (main direction here)
                aXStart -= tStepX;
                aXEnd -= tStepX;
                if (tError >= 0) {
                    // change Y
                    aYStart -= tStepY;
                    aYEnd -= tStepY;
                    tError -= tDeltaXTimes2;
                }
                tError += tDeltaYTimes2;
            }
        }
        // draw start line. We can alternatively use drawLineOverlap(aXStart, aYStart, aXEnd, aYEnd, LINE_OVERLAP_NONE, aColor) here.
        drawLine(aXStart, aYStart, aXEnd, aYEnd, aColor);
        // draw aThickness lines
        tError = tDeltaYTimes2 - tDeltaX;
        for (i = aThickness; i > 1; i--) {
            // change X (main direction here)
            aXStart += tStepX;
            aXEnd += tStepX;
            tOverlap = LINE_OVERLAP_NONE;
            if (tError >= 0) {
                // change Y
                aYStart += tStepY;
                aYEnd += tStepY;
                tError -= tDeltaXTimes2;
                tOverlap = LINE_OVERLAP_BOTH;
            }
            tError += tDeltaYTimes2;
            drawLineOverlap(aXStart, aYStart, aXEnd, aYEnd, tOverlap, aColor);
        }
    } else {
        // adjust draw start point
        if (aThicknessMode == LINE_THICKNESS_MIDDLE) {
            tError = tDeltaXTimes2 - tDeltaY;
            for (i = aThickness / 2; i > 0; i--) {
                aYStart -= tStepY;
                aYEnd -= tStepY;
                if (tError >= 0) {
                    aXStart -= tStepX;
                    aXEnd -= tStepX;
                    tError -= tDeltaYTimes2;
                }
                tError += tDeltaXTimes2;
            }
        }
        // draw start line. We can alternatively use drawLineOverlap(aXStart, aYStart, aXEnd, aYEnd, LINE_OVERLAP_NONE, aColor) here.
        drawLine(aXStart, aYStart, aXEnd, aYEnd, aColor);
        tError = tDeltaXTimes2 - tDeltaY;
        for (i = aThickness; i > 1; i--) {
            aYStart += tStepY;
            aYEnd += tStepY;
            tOverlap = LINE_OVERLAP_NONE;
            if (tError >= 0) {
                aXStart += tStepX;
                aXEnd += tStepX;
                tError -= tDeltaYTimes2;
                tOverlap = LINE_OVERLAP_BOTH;
            }
            tError += tDeltaXTimes2;
            drawLineOverlap(aXStart, aYStart, aXEnd, aYEnd, tOverlap, aColor);
        }
    }
}

// Includes for implementation of drawPixel(), drawLine() and fillRect()
#if defined(SUPPORT_LOCAL_DISPLAY)
#include "BlueDisplay.h"
#endif

void drawPixel(uint16_t aXPos, uint16_t aYPos, uint16_t aColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    BlueDisplay1.drawPixel(aXPos, aYPos, aColor); // this in turn also calls LocalDisplay.drawPixel()
#else
    LocalDisplay.drawPixel(aXPos, aYPos, aColor);
#endif
}

void drawLine(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    BlueDisplay1.drawLine(aXStart, aYStart, aXEnd, aYEnd, aColor); // this in turn also calls LocalDisplay.drawLine()
#else
    LocalDisplay.drawLine(aXStart, aYStart, aXEnd, aYEnd, aColor);
#endif
}

void fillRect(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor){
#if defined(SUPPORT_LOCAL_DISPLAY)
    BlueDisplay1.fillRect(aXStart, aYStart, aXEnd, aYEnd, aColor); // this in turn also calls LocalDisplay.fillRect()
#else
    LocalDisplay.fillRect(aXStart, aYStart, aXEnd, aYEnd, aColor);
#endif
}
/**
 * @}
 */
#endif // _THICKLINE_HPP
