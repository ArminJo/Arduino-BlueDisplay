/**
 * Chart.hpp
 *
 * Draws charts for LCD screen.
 * Charts have axes and a data area
 * Data can be printed as pixels or line or area
 * Labels and grid are optional
 * Origin is the Parameter PositionX + PositionY and overlaps with the axes
 *
 *  Local display interface used:
 *      - getHeight()
 *      - getDisplayWidth()
 *      - fillRect()
 *      - fillRectRel()
 *      - drawText()
 *      - drawPixel()
 *      - drawLineFastOneX()
 *      - TEXT_SIZE_11_WIDTH
 *      - TEXT_SIZE_11_HEIGHT
 *
 *  Copyright (C) 2012-2024  Armin Joachimsmeyer
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
 */

#ifndef _CHART_HPP
#define _CHART_HPP

#if !defined(_BLUEDISPLAY_HPP)
#include "BlueDisplay.hpp"
#endif
#if !defined(_GUI_HELPER_HPP)
#include "GUIHelper.hpp" // for getTextDecend() etc.
#endif

#if defined(DEBUG)
#define LOCAL_DEBUG
#else
//#define LOCAL_DEBUG // This enables debug output only for this file
#endif

//#define LOCAL_TEST
#if defined(LOCAL_TEST)
#include "AVRUtils.h"       // For initStackFreeMeasurement(), printRAMInfo()
#endif

#if defined(SUPPORT_LOCAL_DISPLAY) && defined(DISABLE_REMOTE_DISPLAY)
#define DisplayForChart    LocalDisplay
#else
#define DisplayForChart    BlueDisplay1
#endif

#include "Chart.h"

#if !defined(ARDUINO)
#include "AssertErrorAndMisc.h"
#include <string.h> // for strlen
#include <stdlib.h> // for srand
#endif
//#include <stdio.h> // for snprintf in

/** @addtogroup Graphic_Library
 * @{
 */
/** @addtogroup Chart
 * @{
 */
Chart::Chart() { // @suppress("Class members should be properly initialized")
    mBackgroundColor = CHART_DEFAULT_BACKGROUND_COLOR;
    mAxesColor = CHART_DEFAULT_AXES_COLOR;
    mGridColor = CHART_DEFAULT_GRID_COLOR;
    mLabelColor = CHART_DEFAULT_LABEL_COLOR;
    mFlags = 0;
    mXLabelScaleFactor = CHART_X_AXIS_SCALE_FACTOR_1;
    mXDataScaleFactor = CHART_X_AXIS_SCALE_FACTOR_1;
    mXLabelAndGridStartValueOffset = 0.0;
    mXLabelDistance = 1;
    mXTitleText = NULL;
    mYTitleText = NULL;
}

void Chart::initChartColors(const color16_t aDataColor, const color16_t aAxesColor, const color16_t aGridColor,
        const color16_t aLabelColor, const color16_t aBackgroundColor) {
    mDataColor = aDataColor;
    mAxesColor = aAxesColor;
    mGridColor = aGridColor;
    mLabelColor = aLabelColor;
    mBackgroundColor = aBackgroundColor;
}

void Chart::setDataColor(color16_t aDataColor) {
    mDataColor = aDataColor;
}

void Chart::setBackgroundColor(color16_t aBackgroundColor) {
    mBackgroundColor = aBackgroundColor;
}

void Chart::setLabelColor(color16_t aLabelColor) {
    mLabelColor = aLabelColor;
}

/**
 * aPositionX and aPositionY are the 0 coordinates of the grid and part of the axes
 */
uint8_t Chart::initChart(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX, const uint16_t aHeightY,
        const uint8_t aAxesSize, const uint8_t aLabelTextSize, const bool aHasGrid, const uint16_t aGridOrLabelXPixelSpacing,
        const uint16_t aGridOrLabelYPixelSpacing) {
    mPositionX = aPositionX;
    mPositionY = aPositionY;
    mWidthX = aWidthX;
    mHeightY = aHeightY;
    mAxesSize = aAxesSize;
    mLabelTextSize = aLabelTextSize;
    mTitleTextSize = aLabelTextSize;
    mGridXPixelSpacing = aGridOrLabelXPixelSpacing;
    mGridYPixelSpacing = aGridOrLabelYPixelSpacing;

    if (aHasGrid) {
        mFlags |= CHART_HAS_GRID;
    } else {
        mFlags &= ~CHART_HAS_GRID;
    }

    return checkParameterValues();
}

uint8_t Chart::checkParameterValues(void) {
    uint8_t tRetValue = 0;
    // also check for zero :-)
    if (mAxesSize - 1 > CHART_MAX_AXES_SIZE) {
        mAxesSize = CHART_MAX_AXES_SIZE;
        tRetValue = CHART_ERROR_AXES_SIZE;
    }
    uint16_t t2AxesSize = 2 * mAxesSize;
    if (mPositionX < t2AxesSize - 1) {
        mPositionX = t2AxesSize - 1;
        mWidthX = 100;
        tRetValue = CHART_ERROR_POS_X;
    }
    if (mPositionY > DisplayForChart.getDisplayHeight() - t2AxesSize) {
        mPositionY = DisplayForChart.getDisplayHeight() - t2AxesSize;
        tRetValue = CHART_ERROR_POS_Y;
    }
    if (mPositionX + mWidthX > DisplayForChart.getDisplayWidth()) {
        mPositionX = 0;
        mWidthX = 100;
        tRetValue = CHART_ERROR_WIDTH;
    }

    if (mHeightY > mPositionY + 1) {
        mHeightY = mPositionY + 1;
        tRetValue = CHART_ERROR_HEIGHT;
    }

    if (mGridXPixelSpacing > mWidthX) {
        mGridXPixelSpacing = mWidthX / 2;
        tRetValue = CHART_ERROR_GRID_X_SPACING;
    }
    return tRetValue;
}

/**
 * @param aXLabelStartValue
 * @param aXLabelIncrementValue Value relates to CHART_X_AXIS_SCALE_FACTOR_1 / identity. long is especially useful for long time increments
 * @param aXLabelScaleFactor
 * @param aXMinStringWidth
 */
void Chart::initXLabelInteger(const int aXLabelStartValue, const long aXLabelIncrementValue, const uint8_t aXLabelScaleFactor,
        const uint8_t aXMinStringWidth) {
    mXLabelStartValue.IntValue = aXLabelStartValue;
    mXLabelBaseIncrementValue.LongValue = aXLabelIncrementValue;
    mXLabelScaleFactor = aXLabelScaleFactor;
    mXMinStringWidth = aXMinStringWidth;
    mFlags |= CHART_X_LABEL_INT | CHART_X_LABEL_USED;
}

/**
 * @param aXLabelStartValue
 * @param aXLabelIncrementValue
 * @param aIntegerScaleFactor
 * @param aXMinStringWidthIncDecimalPoint
 * @param aXNumVarsAfterDecimal
 */
void Chart::initXLabelFloat(const float aXLabelStartValue, const float aXLabelIncrementValue, const uint8_t aXLabelScaleFactor,
        uint8_t aXMinStringWidthIncDecimalPoint, uint8_t aXNumVarsAfterDecimal) {
    mXLabelStartValue.FloatValue = aXLabelStartValue;
    mXLabelBaseIncrementValue.FloatValue = aXLabelIncrementValue;
    mXLabelScaleFactor = aXLabelScaleFactor;
    mXNumVarsAfterDecimal = aXNumVarsAfterDecimal;
    mXMinStringWidth = aXMinStringWidthIncDecimalPoint;
    mFlags &= ~CHART_X_LABEL_INT;
    if (aXMinStringWidthIncDecimalPoint != 0) {
        mFlags |= CHART_X_LABEL_USED;
    }
}

/**
 *
 * @param aYLabelStartValue
 * @param aYLabelIncrementValue increment for one grid line
 * @param aYFactor factor for input to chart value - e.g. (3.0 / 4096) for adc reading of 4096 for 3 (Volt)
 * @param aYMinStringWidth for y axis label
 */
void Chart::initYLabelInt(const int aYLabelStartValue, const int aYLabelIncrementValue, const float aYFactor,
        const uint8_t aYMinStringWidth) {
    mYLabelStartValue.IntValue = aYLabelStartValue;
    mYLabelIncrementValue.IntValue = aYLabelIncrementValue;
    mYMinStringWidth = aYMinStringWidth;
    mFlags |= CHART_Y_LABEL_INT | CHART_Y_LABEL_USED;
    mYDataFactor = aYFactor;
}

/**
 *
 * @param aYLabelStartValue
 * @param aYLabelIncrementValue
 * @param aYFactor factor for input to chart value - e.g. (3.0 / 4096) for adc reading of 4096 for 3 Volt
 * @param aYMinStringWidthIncDecimalPoint for y axis label
 * @param aYNumVarsAfterDecimal for y axis label
 */
void Chart::initYLabelFloat(const float aYLabelStartValue, const float aYLabelIncrementValue, const float aYFactor,
        const uint8_t aYMinStringWidthIncDecimalPoint, const uint8_t aYNumVarsAfterDecimal) {
    mYLabelStartValue.FloatValue = aYLabelStartValue;
    mYLabelIncrementValue.FloatValue = aYLabelIncrementValue;
    mYMinStringWidth = aYMinStringWidthIncDecimalPoint;
    mYNumVarsAfterDecimal = aYNumVarsAfterDecimal;
    mYDataFactor = aYFactor;
    mFlags &= ~CHART_Y_LABEL_INT;
    mFlags |= CHART_Y_LABEL_USED;
}

/**
 * Render the chart on the lcd
 */
void Chart::drawAxesAndGrid(void) {
    drawAxesAndLabels();
    drawGrid();
}

void Chart::drawGrid(void) {
    if (!(mFlags & CHART_HAS_GRID)) {
        return;
    }
    int16_t tXPixelOffsetOfCurrentLine = 0;
    if (mXLabelAndGridStartValueOffset != 0.0) {
        if (mFlags & CHART_X_LABEL_INT) {
            /*
             * If mXLabelAndGridStartValueOffset == mXLabelBaseIncrementValue then we start with 2. grid at the original start position
             * Pixel offset is (mXLabelAndGridStartValueOffset / mXLabelBaseIncrementValue) * mGridXPixelSpacing
             * or  (xScaleAdjusted(mXLabelAndGridStartValueOffset) / mXLabelBaseIncrementValue) * mGridXPixelSpacing
             * Offset positive -> grid is shifted left
             */
            tXPixelOffsetOfCurrentLine = -(mXLabelAndGridStartValueOffset * mGridXPixelSpacing)
                    / reduceLongWithIntegerScaleFactor(mXLabelBaseIncrementValue.LongValue);
        } else {
            tXPixelOffsetOfCurrentLine = -(mXLabelAndGridStartValueOffset * mGridXPixelSpacing)
                    / reduceFloatWithIntegerScaleFactor(mXLabelBaseIncrementValue.FloatValue);
        }
    }
#if defined(LOCAL_DEBUG)
    Serial.print(F("PixelOffset of first grid="));
    Serial.print(tXPixelOffsetOfCurrentLine);
    Serial.print(F(" gridSpacing="));
    Serial.print(mGridXPixelSpacing);
    Serial.print(F(" PositionX="));
    Serial.println(mPositionX);
#endif
    tXPixelOffsetOfCurrentLine += mGridXPixelSpacing; // Do not render first line, it is on Y axis

// draw vertical lines, X scale
    do {
        if (tXPixelOffsetOfCurrentLine > 0) {
            DisplayForChart.drawLineRel(mPositionX + tXPixelOffsetOfCurrentLine, mPositionY - (mHeightY - 1), 0, mHeightY - 1,
                    mGridColor);
        }
        tXPixelOffsetOfCurrentLine += mGridXPixelSpacing;
    } while (tXPixelOffsetOfCurrentLine <= (int16_t) mWidthX);

// draw horizontal lines, Y scale
    for (uint16_t tYOffset = mGridYPixelSpacing; tYOffset <= mHeightY; tYOffset += mGridYPixelSpacing) {
        DisplayForChart.drawLineRel(mPositionX + 1, mPositionY - tYOffset, mWidthX - 1, 0, mGridColor);
    }
}

/**
 * render axes
 * renders indicators if labels but no grid are specified
 */
void Chart::drawAxesAndLabels() {
    drawXAxisAndLabels();
    drawYAxisAndLabels();
}

/**
 * draw x title
 * Use label color, because it is the legend for the X axis label
 */
void Chart::drawXAxisTitle() const {
    if (mXTitleText != NULL) {
        /**
         * draw axis title
         */
        uint16_t tTextLenPixel = strlen(mXTitleText) * getTextWidth(mTitleTextSize);
        DisplayForChart.drawText(mPositionX + mWidthX - tTextLenPixel - 1, mPositionY - getTextDecend(mTitleTextSize), mXTitleText,
                mTitleTextSize, mLabelColor, mBackgroundColor);
    }
}

/**
 * Enlarge value if scale factor is negative / compression
 * Reduce value, if scale factor is positive / expansion
 *
 * aIntegerScaleFactor > 1 : expansion by factor aIntegerScaleFactor. I.e. value -> (value / factor)
 * aIntegerScaleFactor == 1 : expansion by 1.5
 * aIntegerScaleFactor == 0 : identity
 * aIntegerScaleFactor == -1 : compression by 1.5
 * aIntegerScaleFactor < -1 : compression by factor -aIntegerScaleFactor -> (value * factor)
 * multiplies value with factor if aIntegerScaleFactor is < 0 (compression) or divide if aIntegerScaleFactor is > 0 (expansion)
 */
long Chart::reduceLongWithIntegerScaleFactor(long aValue) {
    return reduceLongWithIntegerScaleFactor(aValue, mXLabelScaleFactor);
}

float Chart::reduceFloatWithIntegerScaleFactor(float aValue) {
    return reduceFloatWithIntegerScaleFactor(aValue, mXLabelScaleFactor);
}

/**
 * Enlarge value if scale factor is expansion
 * Reduce value, if scale factor is compression
 */
int Chart::enlargeLongWithIntegerScaleFactor(int aValue) {
    return reduceLongWithIntegerScaleFactor(aValue, -mXLabelScaleFactor);
}

float Chart::enlargeFloatWithIntegerScaleFactor(float aValue) {
    return reduceFloatWithIntegerScaleFactor(aValue, -mXLabelScaleFactor);
}

/**
 * Draw X line with indicators and labels
 * Render indicators if labels enabled, but no grid is specified
 * Label increment value is adjusted with scale factor
 */
void Chart::drawXAxisAndLabels() {

    char tLabelStringBuffer[10];
    uint16_t tPositionY = mPositionY;
    /*
     * Draw X axis line
     */
    DisplayForChart.fillRectRel(mPositionX - (mAxesSize - 1), tPositionY, mWidthX + (mAxesSize - 1), mAxesSize, mAxesColor);

    if (mFlags & CHART_X_LABEL_USED) {
        /*
         * Draw indicator and label numbers
         */
        if (!(mFlags & CHART_HAS_GRID)) {
            /*
             * Draw indicators with the same size the axis has
             */
            for (uint16_t tGridOffset = 0; tGridOffset <= mWidthX; tGridOffset += mGridXPixelSpacing) {
                DisplayForChart.fillRectRel(mPositionX + tGridOffset, tPositionY + mAxesSize, 1, mAxesSize, mGridColor);
            }
            tPositionY += mAxesSize;
        }
        /*
         * Now draw labels, (2 * mAxesSize) leaves a gap of axes size
         */
        uint16_t tNumberYTop = tPositionY + 2 * mAxesSize;
#if !defined(ARDUINO)
        assertParamMessage((tNumberYTop <= (DisplayForChart.getDisplayHeight() - getTextDecend(mLabelTextSize))), tNumberYTop,
                "no space for x labels");
#endif

        /*
         * Clear label space before
         */
        uint8_t tTextWidth = getTextWidth(mLabelTextSize);
        int16_t tXStringPixelOffset = ((tTextWidth * mXMinStringWidth) / 2) + 1;
        DisplayForChart.fillRect(mPositionX - tXStringPixelOffset, tNumberYTop, mPositionX + mWidthX + tXStringPixelOffset + 1,
                tNumberYTop + getTextHeight(mLabelTextSize), mBackgroundColor);

        // initialize both variables to avoid compiler warnings
        int tValue = mXLabelStartValue.IntValue;
        float tValueFloat = mXLabelStartValue.FloatValue;

        /*
         * Compute effective label distance
         * effective distance can be greater than 1 only if distance is > 1 and we have a integer expansion of scale
         */
        uint8_t tEffectiveXLabelDistance = 1;
        if (mXLabelDistance > 1
                && (mXLabelScaleFactor == CHART_X_AXIS_SCALE_FACTOR_1 || mXLabelScaleFactor >= CHART_X_AXIS_SCALE_FACTOR_EXPANSION_2)) {
            tEffectiveXLabelDistance = enlargeLongWithIntegerScaleFactor(mXLabelDistance);
        }

        long tIncrementValue = reduceLongWithIntegerScaleFactor(mXLabelBaseIncrementValue.LongValue) * tEffectiveXLabelDistance;
        float tIncrementValueFloat = reduceFloatWithIntegerScaleFactor(mXLabelBaseIncrementValue.FloatValue)
                * tEffectiveXLabelDistance;
        int16_t tXPixelOffsetOfCurrentLabel = 0;
        if (mXLabelAndGridStartValueOffset != 0.0) {
            if (mFlags & CHART_X_LABEL_INT) {
                // Use enlarge on dividend instead of reduce on divisor to avoid possible division by zero
                tXPixelOffsetOfCurrentLabel -= (enlargeFloatWithIntegerScaleFactor(mXLabelAndGridStartValueOffset)
                        * mGridXPixelSpacing) / mXLabelBaseIncrementValue.LongValue;
            } else {
                tXPixelOffsetOfCurrentLabel -= (mXLabelAndGridStartValueOffset * mGridXPixelSpacing)
                        / reduceFloatWithIntegerScaleFactor(mXLabelBaseIncrementValue.FloatValue);
            }
        }
#if defined(LOCAL_DEBUG)
        Serial.print(F("PixelOffset of first label="));
        Serial.print(tXPixelOffsetOfCurrentLabel);
        Serial.print(F(" gridSpacing="));
        Serial.println(mGridXPixelSpacing);
#endif
        /*
         * loop for drawing labels at X axis
         */
        do {
            uint8_t tStringLength;
            if (mFlags & CHART_X_LABEL_INT) {
                tStringLength = snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%d", tValue);
                tValue += tIncrementValue;
            } else {
#if defined(__AVR__)
                dtostrf(tValueFloat, mXMinStringWidth, mXNumVarsAfterDecimal, tLabelStringBuffer);
                tStringLength =
                        strlen(tLabelStringBuffer); // do not know, if it works here ...
#else
                tStringLength = snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%*.*f", mXMinStringWidth, mXNumVarsAfterDecimal,
                        tValueFloat);
#endif
                tValueFloat += tIncrementValueFloat;
            }
            /*
             * Compute offset to place it at the middle
             */
            tXStringPixelOffset = (tTextWidth * tStringLength) / 2; // strlen(tLabelStringBuffer) does not work for time label :-( 1.12.24

            if (tXPixelOffsetOfCurrentLabel >= 0) {
                DisplayForChart.drawText(mPositionX + tXPixelOffsetOfCurrentLabel - tXStringPixelOffset,
                        tNumberYTop + getTextAscend(mLabelTextSize), tLabelStringBuffer, mLabelTextSize, mLabelColor,
                        mBackgroundColor);
            }
            tXPixelOffsetOfCurrentLabel += mGridXPixelSpacing * tEffectiveXLabelDistance; // skip labels

        } while (tXPixelOffsetOfCurrentLabel <= (int16_t) mWidthX);
    }
}

/**
 * draw x line with main labels at mXLabelDistance and intermediate label at the same distance.
 * aStartTimestamp  Timestamp of first label and intermediate labels too!!! It may not be rendered!
 * mXLabelDistance  Distance between 2 labels at scale factor 1
 */
void Chart::drawXAxisAndDateLabels(time_t aStartTimestamp, drawXAxisTimeDateSettingsStruct *aDrawXAxisTimeDateSettings) {

    char tLabelStringBuffer[8]; // 12:15 are 6 characters 12/2024 are 8 character

    /*
     * Draw X axis line
     */
    BlueDisplay1.fillRectRel(mPositionX - (mAxesSize - 1), mPositionY, mWidthX + (mAxesSize - 1), mAxesSize, mAxesColor);

    /*
     * Now draw date labels <day>.<month>
     */
    uint16_t tNumberYTop = mPositionY + 2 * mAxesSize;
#if !defined(ARDUINO)
        assertParamMessage((tNumberYTop <= (DisplayForChart.getDisplayHeight() - getTextDecend(mLabelTextSize))), tNumberYTop,
                "no space for x labels");
#endif

    // initialize both variables to avoid compiler warnings
    time_t tTimeStampForLabel = aStartTimestamp;

    /*
     * Clear label space
     */
    int16_t tXStringPixelOffset = ((getTextWidth(mLabelTextSize) * aDrawXAxisTimeDateSettings->maximumCharactersOfLabel) / 2) + 1;
    BlueDisplay1.fillRect(mPositionX - tXStringPixelOffset, mPositionY + mAxesSize + 1,
            mPositionX + mWidthX + (tXStringPixelOffset + 1), tNumberYTop + getTextHeight(mLabelTextSize), mBackgroundColor);

    /*
     * Compute effective label distance, as multiple of grid lines
     * effective distance can be greater than 1 only if distance is > 1 and we have a integer expansion of scale
     */
    uint8_t tEffectiveXLabelDistance = enlargeLongWithIntegerScaleFactor(mXLabelDistance);
    if (tEffectiveXLabelDistance < mXLabelDistance) {
        tEffectiveXLabelDistance = mXLabelDistance;
    }

    /*
     * If mXLabelAndGridStartValueOffset == mXLabelBaseIncrementValue then label starts with 2. major value
     * Pixel offset is (mXLabelAndGridStartValueOffset / mXLabelBaseIncrementValue) * mGridXPixelSpacing
     * Offset positive -> grid is shifted left
     * e.g. Offset is 1/2 tEffectiveXLabelDistance pixel smaller or left, if mXLabelAndGridStartValueOffset is 1/2 of mXLabelBaseIncrementValue
     */
    int16_t tXPixelOffsetOfCurrentLabel = 0;
    if (mXLabelAndGridStartValueOffset != 0.0) {
        // mXLabelAndGridStartValueOffset is float, so we do not have a overflow here
        tXPixelOffsetOfCurrentLabel = -(mXLabelAndGridStartValueOffset * mGridXPixelSpacing)
                / reduceLongWithIntegerScaleFactor(mXLabelBaseIncrementValue.LongValue); // grid is based on long value
    }

#if defined(LOCAL_DEBUG)
    Serial.print(F("PixelOffset of first label="));
    Serial.print(tXPixelOffsetOfCurrentLabel);
    Serial.print(F(" gridSpacing="));
    Serial.print(mGridXPixelSpacing);
    Serial.print(F(" PositionX="));
    Serial.println(mPositionX);
#endif

    /*
     * Draw indicators
     */
    if (!(mFlags & CHART_HAS_GRID)) {
        /*
         * Draw indicators with the same size the axis has
         */
        int16_t tGridOffset = tXPixelOffsetOfCurrentLabel;
        do {
            BlueDisplay1.fillRectRel(mPositionX + tGridOffset, mPositionY + mAxesSize, 1, mAxesSize, mGridColor);
            tGridOffset += mGridXPixelSpacing;
        } while (tGridOffset <= (int16_t) mWidthX);
    }

    /*
     * Draw a label every mXLabelDistance
     * Start with a major label and draw it at every tEffectiveXLabelDistance
     */
    uint8_t tGridIndex = 0;
    do {
        uint8_t tCurrentTextSize = mLabelTextSize;
        uint8_t tCurrentTextWidth = getTextWidth(mLabelTextSize);
        uint8_t tStringLength;
        if (tGridIndex % tEffectiveXLabelDistance == 0) {
            /*
             * Generate string for major label
             */
            tStringLength = snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%d.%d",
                    aDrawXAxisTimeDateSettings->firstTokenFunction(tTimeStampForLabel),
                    aDrawXAxisTimeDateSettings->secondTokenFunction(tTimeStampForLabel));
        } else {
            /*
             * Generate string for intermediate label
             */
            tStringLength = snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%d",
                    aDrawXAxisTimeDateSettings->intermediateTokenFunction(tTimeStampForLabel));
            tCurrentTextSize -= tCurrentTextSize / 8; // 8 is an experimental value, which looks good for me :-)
            tCurrentTextWidth -= tCurrentTextWidth / 8;
        }
        /*
         * Compute offset to place it at the middle
         */
        tXStringPixelOffset = (tCurrentTextWidth * tStringLength) / 2; // strlen(tLabelStringBuffer) does not work here e.g. I get constant 4 :-( 1.12.24
#if defined(LOCAL_TRACE)
        Serial.print(F("Width="));
        Serial.print(tCurrentTextWidth);
        Serial.print(F(" length="));
        Serial.print(tStringLength);
        Serial.print(F(" strlength="));
        Serial.print(strlen(tLabelStringBuffer));
        Serial.print(F(" Offset="));
        Serial.println(tXStringPixelOffset);
#endif
        /*
         * Draw label. It may not be rendered, because of its negative offset
         */
        if (tXPixelOffsetOfCurrentLabel >= 0) {
            BlueDisplay1.drawText(mPositionX + tXPixelOffsetOfCurrentLabel - tXStringPixelOffset,
                    tNumberYTop + getTextAscend(tCurrentTextSize), tLabelStringBuffer, tCurrentTextSize, mLabelColor,
                    mBackgroundColor);
        }

        // set values for next loop
        tTimeStampForLabel += mXLabelDistance * reduceLongWithIntegerScaleFactor(mXLabelBaseIncrementValue.LongValue);
        tXPixelOffsetOfCurrentLabel += mXLabelDistance * mGridXPixelSpacing;
        tGridIndex += mXLabelDistance;

    } while (tXPixelOffsetOfCurrentLabel <= (int16_t) mWidthX);
}

/**
 * Set x label start to index.th value - start not with first but with startIndex label
 * Used for horizontal scrolling
 */
void Chart::setXLabelIntStartValueByIndex(const int aNewXStartIndex, const bool doRedrawXAxis) {
    mXLabelStartValue.LongValue = mXLabelBaseIncrementValue.LongValue * aNewXStartIndex;
    if (doRedrawXAxis) {
        drawXAxisAndLabels();
    }
}

/**
 * If aDoIncrement = true increment XLabelStartValue , else decrement
 * redraw Axis
 * @retval true if X value was not clipped
 */
bool Chart::stepXLabelStartValueInt(const bool aDoIncrement, const int aMinValue, const int aMaxValue) {
    bool tRetval = true;
    if (aDoIncrement) {
        mXLabelStartValue.IntValue += mXLabelBaseIncrementValue.LongValue;
        if (mXLabelStartValue.IntValue > aMaxValue) {
            mXLabelStartValue.IntValue = aMaxValue;
            tRetval = false;
        }
    } else {
        mXLabelStartValue.IntValue -= mXLabelBaseIncrementValue.LongValue;
        if (mXLabelStartValue.IntValue < aMinValue) {
            mXLabelStartValue.IntValue = aMinValue;
            tRetval = false;
        }
    }
    drawXAxisAndLabels();
    return tRetval;
}

/**
 * Increments or decrements the start value by one increment value (one grid line)
 * and redraws Axis
 * does not decrement below 0
 */
float Chart::stepXLabelStartValueFloat(const bool aDoIncrement) {
    if (aDoIncrement) {
        mXLabelStartValue.FloatValue += mXLabelBaseIncrementValue.FloatValue;
    } else {
        mXLabelStartValue.FloatValue -= mXLabelBaseIncrementValue.FloatValue;
    }
    if (mXLabelStartValue.FloatValue < 0) {
        mXLabelStartValue.FloatValue = 0;
    }
    drawXAxisAndLabels();
    return mXLabelStartValue.FloatValue;
}

/**
 * draw y title starting at the Y axis
 * Use data color, because it is the legend for the data
 * @param aYOffset the offset in pixel below the top of the Y line
 *
 */
void Chart::drawYAxisTitle(const int aYOffset) const {
    if (mYTitleText != NULL) {
        /**
         * draw axis title - use data color
         */
        DisplayForChart.drawText(mPositionX + mAxesSize + 1, mPositionY - mHeightY + aYOffset + getTextAscend(mTitleTextSize),
                mYTitleText, mTitleTextSize, mDataColor, mBackgroundColor);
    }
}

/**
 * draw y line with indicators and labels
 * renders indicators if labels but no grid are specified
 */
void Chart::drawYAxisAndLabels() {

    char tLabelStringBuffer[10];
    int16_t tPositionX = mPositionX;

    /*
     * Draw Y axis line, such that 0 is on the line and 1 is beneath it.
     */
    DisplayForChart.fillRectRel(tPositionX - (mAxesSize - 1), mPositionY - (mHeightY - 1), mAxesSize, (mHeightY - 1), mAxesColor);

    if (mFlags & CHART_Y_LABEL_USED) {
        /*
         * Draw indicator and label numbers
         */
        uint16_t tOffset;
        if (!(mFlags & CHART_HAS_GRID)) {
            /*
             * Draw indicators with the same size the axis has
             */
            for (tOffset = 0; tOffset <= mHeightY; tOffset += mGridYPixelSpacing) {
                DisplayForChart.fillRectRel(tPositionX - (2 * mAxesSize) + 1, mPositionY - tOffset, mAxesSize, 1, mGridColor);
            }
            tPositionX -= mAxesSize; // position labels more left by indicator size
        }

        /*
         * Draw labels (numbers)
         */
        int16_t tYNumberXStart = tPositionX - ((2 * mAxesSize) + 1 + (mYMinStringWidth * getTextWidth(mLabelTextSize)));
#if !defined(ARDUINO)
        assertParamMessage((tYNumberXStart >= 0), tYNumberXStart, "no space for y labels");
#endif

        // First offset is half of character height
        uint8_t tTextHeight = getTextHeight(mLabelTextSize);
        // clear label space before
        DisplayForChart.fillRect(tYNumberXStart, mPositionY - (mHeightY - 1), tPositionX - mAxesSize,
                mPositionY + getTextDecend(tTextHeight), mBackgroundColor);

        // convert to string
        // initialize both variables to avoid compiler warnings
        long tValue = mYLabelStartValue.IntValue;
        float tValueFloat = mYLabelStartValue.FloatValue;
        /*
         * draw loop
         */
        uint16_t tYOffsetForLabel = 0;
        do {
            if (mFlags & CHART_Y_LABEL_INT) {
#if defined(__AVR__)
                snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%ld", tValue);
#else
                snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%*ld", mYMinStringWidth, tValue);
#endif
                tValue += mYLabelIncrementValue.IntValue;
            } else {
#if defined(__AVR__)
                dtostrf(tValueFloat, mYMinStringWidth, mYNumVarsAfterDecimal, tLabelStringBuffer);
#else
                snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%*.*f", mYMinStringWidth, mYNumVarsAfterDecimal,
                            tValueFloat);
#endif
                tValueFloat += mYLabelIncrementValue.FloatValue;

            }
            DisplayForChart.drawText(tYNumberXStart, mPositionY - tYOffsetForLabel + getTextMiddleCorrection(tTextHeight),
                    tLabelStringBuffer, mLabelTextSize, mLabelColor, mBackgroundColor);
            tYOffsetForLabel += mGridYPixelSpacing;
        } while (tYOffsetForLabel <= mHeightY);
    }
}

bool Chart::stepYLabelStartValueInt(const bool aDoIncrement, const int aMinValue, const int aMaxValue) {
    bool tRetval = true;
    if (aDoIncrement) {
        mYLabelStartValue.IntValue += mYLabelIncrementValue.IntValue;
        if (mYLabelStartValue.IntValue > aMaxValue) {
            mYLabelStartValue.IntValue = aMaxValue;
            tRetval = false;
        }
    } else {
        mYLabelStartValue.IntValue -= mYLabelIncrementValue.IntValue;
        if (mYLabelStartValue.IntValue < aMinValue) {
            mYLabelStartValue.IntValue = aMinValue;
            tRetval = false;
        }
    }
    drawYAxisAndLabels();
    return tRetval;
}

/**
 * increments or decrements the start value by value (one grid line)
 * and redraws Axis
 * does not decrement below 0
 */
float Chart::stepYLabelStartValueFloat(const int aSteps) {
    mYLabelStartValue.FloatValue += mYLabelIncrementValue.FloatValue * aSteps;
    if (mYLabelStartValue.FloatValue < 0) {
        mYLabelStartValue.FloatValue = 0;
    }
    drawYAxisAndLabels();
    return mYLabelStartValue.FloatValue;
}

/**
 * Clears chart area and redraws axes lines
 */
void Chart::clear(void) {
// (mHeightY - 1) and mHeightY as parameter aHeight left some spurious pixel on my pixel 8
    DisplayForChart.fillRectRel(mPositionX + 1, mPositionY - (mHeightY - 1), mWidthX, mHeightY - 1, mBackgroundColor);
// draw X line
    DisplayForChart.fillRectRel(mPositionX - (mAxesSize - 1), mPositionY, mWidthX + (mAxesSize - 1), mAxesSize, mAxesColor);
//draw y line
    DisplayForChart.fillRectRel(mPositionX - (mAxesSize - 1), mPositionY - (mHeightY - 1), mAxesSize, mHeightY - 1, mAxesColor);
}

/**
 * Draws a chart - If mYDataFactor is 1, then pixel position matches y scale.
 * mYDataFactor Factor for uint16_t values to chart value (mYFactor) is used to compute display values
 * e.g. (3.0 / 4096) for ADC reading of 4096 for 3 (Volt)
 * @param aDataPointer pointer to raw data array
 * @param aDataEndPointer pointer to first element after data
 * @param aMode CHART_MODE_PIXEL, CHART_MODE_LINE or CHART_MODE_AREA
 * @return false if clipping occurs
 */
void Chart::drawChartDataFloat(float *aDataPointer, const uint16_t aLengthOfValidData, const uint8_t aMode) {

    float tInputValue;

// used only in line mode
    int tLastValue = 0;

// Factor for Float -> Display value
    float tYDisplayFactor = 1;
    float tYOffset = 0;

    if (mFlags & CHART_Y_LABEL_INT) {
        // mGridYPixelSpacing / mYLabelIncrementValue.IntValue is factor float -> pixel e.g. 40 pixel for 200 value
        tYDisplayFactor = (mYDataFactor * mGridYPixelSpacing) / mYLabelIncrementValue.IntValue;
        tYOffset = mYLabelStartValue.IntValue / mYDataFactor;
    } else {
        tYDisplayFactor = (mYDataFactor * mGridYPixelSpacing) / mYLabelIncrementValue.FloatValue;
        tYOffset = mYLabelStartValue.FloatValue / mYDataFactor;
    }

    uint16_t tXpos = mPositionX;
    bool tFirstValue = true;

    const float *tDataEndPointer = aDataPointer + aLengthOfValidData;
    int tXScaleCounter = mXDataScaleFactor;
    if (mXDataScaleFactor < CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
        tXScaleCounter = -mXDataScaleFactor;
    }

    for (int i = mWidthX; i > 0; i--) {
        /*
         *  get data and perform X scaling
         */
        if (mXDataScaleFactor == CHART_X_AXIS_SCALE_FACTOR_1) { // == 0
            tInputValue = *aDataPointer++;
        } else if (mXDataScaleFactor == CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) { // == -1
            // compress by factor 1.5 - every second value is the average of the next two values
            tInputValue = *aDataPointer++;
            tXScaleCounter--;                // starts with 1
            if (tXScaleCounter < 0) {
                // get average of actual and next value
                tInputValue += *aDataPointer++;
                tInputValue /= 2;
                tXScaleCounter = 1;
            }
        } else if (mXDataScaleFactor <= CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
            // compress - get average of multiple values
            tInputValue = 0;
            for (int j = 0; j < tXScaleCounter; ++j) {
                tInputValue += *aDataPointer++;
            }
            tInputValue /= tXScaleCounter;
        } else if (mXDataScaleFactor == CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5) { // ==1
            // expand by factor 1.5 - every second value will be shown 2 times
            tInputValue = *aDataPointer++;
            tXScaleCounter--;                // starts with 1
            if (tXScaleCounter < 0) {
                aDataPointer--;
                tXScaleCounter = 2;
            }
        } else {
            // expand - show value several times
            tInputValue = *aDataPointer;
            tXScaleCounter--;
            if (tXScaleCounter == 0) {
                aDataPointer++;
                tXScaleCounter = mXDataScaleFactor;
            }
        }
        // check for data pointer still in data buffer area
        if (aDataPointer >= tDataEndPointer) {
            break;
        }

        uint16_t tDisplayValue = tYDisplayFactor * (tInputValue - tYOffset);
        // clip to bottom line
        if (tYOffset > tInputValue) {
            tDisplayValue = 0;
        }
        // clip to top value
        if (tDisplayValue > mHeightY - 1) {
            tDisplayValue = mHeightY - 1;
        }
        if (aMode == CHART_MODE_AREA) {
            //since we draw a 1 pixel line for value 0
            tDisplayValue += 1;
            DisplayForChart.fillRectRel(tXpos, mPositionY - tDisplayValue, 1, tDisplayValue, mDataColor);
        } else if (aMode == CHART_MODE_PIXEL || tFirstValue) { // even for line draw first value as pixel only
            tFirstValue = false;
            DisplayForChart.drawPixel(tXpos, mPositionY - tDisplayValue, mDataColor);
        } else if (aMode == CHART_MODE_LINE) {
            DisplayForChart.drawLineFastOneX(tXpos - 1, mPositionY - tLastValue, mPositionY - tDisplayValue, mDataColor);
        }
        tLastValue = tDisplayValue;
        tXpos++;
    }
}

/**
 * Draws a chart - If mYDataFactor is 1, then pixel position matches y scale.
 * mYDataFactor Factor for uint16_t values to chart value (mYFactor) is used to compute display values
 * e.g. (3.0 / 4096) for ADC reading of 4096 for 3 (Volt)
 * @param aDataPointer pointer to input data array
 * @param aDataEndPointer pointer to first element after data
 * @param aMode CHART_MODE_PIXEL, CHART_MODE_LINE or CHART_MODE_AREA
 * @return false if clipping occurs
 */
void Chart::drawChartData(int16_t *aDataPointer, const uint16_t aLengthOfValidData, const uint8_t aMode) {

// Factor for Input -> Display value
    float tYDisplayFactor;
    int tYDisplayOffset;

    /*
     * Compute display factor and offset, so that pixel matches the y scale
     */
    if (mFlags & CHART_Y_LABEL_INT) {
        // mGridYPixelSpacing / mYLabelIncrementValue.IntValue is factor input -> pixel e.g. 40 pixel for 200 value
        tYDisplayFactor = (mYDataFactor * mGridYPixelSpacing) / mYLabelIncrementValue.IntValue;
        tYDisplayOffset = mYLabelStartValue.IntValue / mYDataFactor;
    } else {
        // Label is float
        tYDisplayFactor = (mYDataFactor * mGridYPixelSpacing) / mYLabelIncrementValue.FloatValue;
        tYDisplayOffset = mYLabelStartValue.FloatValue / mYDataFactor;
    }

    uint16_t tXpos = mPositionX;
    bool tFirstValue = true;

    int tXScaleCounter = mXDataScaleFactor;
    if (mXDataScaleFactor < CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
        tXScaleCounter = -mXDataScaleFactor;
    }

    int16_t *tDataEndPointer = aDataPointer + aLengthOfValidData;
    int tDisplayValue;
    int tLastValue = 0; // used only in line mode
    for (int i = mWidthX; i > 0; i--) {
        /*
         *  get data and perform X scaling
         */
        if (mXDataScaleFactor == CHART_X_AXIS_SCALE_FACTOR_1) {
            tDisplayValue = *aDataPointer++;
        } else if (mXDataScaleFactor == CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
            // compress by factor 1.5 - every second value is the average of the next two values
            tDisplayValue = *aDataPointer++;
            tXScaleCounter--; // starts with 1
            if (tXScaleCounter < 0) {
                // get average of actual and next value
                tDisplayValue += *aDataPointer++;
                tDisplayValue /= 2;
                tXScaleCounter = 1;
            }
        } else if (mXDataScaleFactor <= CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
            // compress - get average of multiple values
            tDisplayValue = 0;
            for (int j = 0; j < tXScaleCounter; ++j) {
                tDisplayValue += *aDataPointer++;
            }
            tDisplayValue /= tXScaleCounter;
        } else if (mXDataScaleFactor == CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5) {
            // expand by factor 1.5 - every second value will be shown 2 times
            tDisplayValue = *aDataPointer++;
            tXScaleCounter--; // starts with 1
            if (tXScaleCounter < 0) {
                aDataPointer--;
                tXScaleCounter = 2;
            }
        } else {
            // expand - show value several times
            tDisplayValue = *aDataPointer;
            tXScaleCounter--;
            if (tXScaleCounter == 0) {
                aDataPointer++;
                tXScaleCounter = mXDataScaleFactor;
            }
        }
        // check for data pointer still in data buffer
        if (aDataPointer >= tDataEndPointer) {
            break;
        }

        tDisplayValue = tYDisplayFactor * (tDisplayValue - tYDisplayOffset);

        // clip to bottom line
        if (tDisplayValue < 0) {
            tDisplayValue = 0;
        }
        // clip to top value
        if (tDisplayValue > (int) mHeightY - 1) {
            tDisplayValue = mHeightY - 1;
        }
        // draw first value as pixel only
        if (aMode == CHART_MODE_PIXEL || tFirstValue) {
            tFirstValue = false;
            DisplayForChart.drawPixel(tXpos, mPositionY - tDisplayValue, mDataColor);
        } else if (aMode == CHART_MODE_LINE) {
            DisplayForChart.drawLineFastOneX(tXpos - 1, mPositionY - tLastValue, mPositionY - tDisplayValue, mDataColor);
        } else if (aMode == CHART_MODE_AREA) {
            //since we draw a 1 pixel line for value 0
            tDisplayValue += 1;
            DisplayForChart.fillRectRel(tXpos, mPositionY - tDisplayValue, 1, tDisplayValue, mDataColor);
        }
        tLastValue = tDisplayValue;
        tXpos++;
    }

}

/*
 * Draw 8 bit unsigned (compressed) data with Y offset, i.e. value 0 is on X axis independent of mYLabelStartValue
 * Data is uncompressed on the display with mYDataFactor to get chart value and then with the factor from chart value to chart pixel
 */
void Chart::drawChartDataWithYOffset(uint8_t *aDataPointer, uint16_t aLengthOfValidData, const uint8_t aMode) {

// Factor for Input -> Display value
    float tYDisplayFactor;
    /*
     * Compute display factor and offset, so that pixel matches the y scale
     */
    if (mFlags & CHART_Y_LABEL_INT) {
        /*
         * mGridYPixelSpacing / mYLabelIncrementValue.IntValue is factor from chart value to chart pixel
         * e.g. 40/200 for 40 pixel for value 200
         */
        tYDisplayFactor = (mYDataFactor * mGridYPixelSpacing) / mYLabelIncrementValue.IntValue;
    } else {
        // Label is float
        tYDisplayFactor = (mYDataFactor * mGridYPixelSpacing) / mYLabelIncrementValue.FloatValue;
    }

    /*
     * Draw to chart index 0 and do not clear last drawn chart line
     * -tYDisplayFactor, because origin is at upper left and therefore Y values must be subtracted from Y position
     * If we have scale factor -2 for compression we require 2 times as much data
     * If we have scale factor 2 for expansion we require half as much data
     */
    uint16_t tMaximumRequiredData = reduceLongWithIntegerScaleFactor(mWidthX);
    if (aLengthOfValidData > tMaximumRequiredData) {
        aLengthOfValidData = tMaximumRequiredData;
    }
    BlueDisplay1.drawChartByteBufferScaled(mPositionX, mPositionY, mXDataScaleFactor, -tYDisplayFactor, mAxesSize, aMode,
            mDataColor, COLOR16_NO_DELETE, 0, true, aDataPointer, aLengthOfValidData);

#if defined(SUPPORT_LOCAL_DISPLAY)
        uint16_t tXpos = mPositionX;
        bool tFirstValue = true;

        int tXScaleCounter = mXDataScaleFactor;
        if (mXDataScaleFactor < -1) {
            tXScaleCounter = -mXDataScaleFactor;
        }

        const uint8_t *tDataEndPointer = aDataPointer + aLengthOfValidData;
        int tDisplayValue;
        int tLastValue = 0; // used only in line mode
        for (int i = mWidthX; i > 0; i--) {
            /*
             *  get data and perform X scaling
             */
            if (mXDataScaleFactor == 0) {
                tDisplayValue = *aDataPointer++;
            } else if (mXDataScaleFactor == -1) {
                // compress by factor 1.5 - every second value is the average of the next two values
                tDisplayValue = *aDataPointer++;
                tXScaleCounter--;// starts with 1
                if (tXScaleCounter < 0) {
                    // get average of actual and next value
                    tDisplayValue += *aDataPointer++;
                    tDisplayValue /= 2;
                    tXScaleCounter = 1;
                }
            } else if (mXDataScaleFactor <= -1) {
                // compress - get average of multiple values
                tDisplayValue = 0;
                for (int j = 0; j < tXScaleCounter; ++j) {
                    tDisplayValue += *aDataPointer++;
                }
                tDisplayValue /= tXScaleCounter;
            } else if (mXDataScaleFactor == 1) {
                // expand by factor 1.5 - every second value will be shown 2 times
                tDisplayValue = *aDataPointer++;
                tXScaleCounter--;// starts with 1
                if (tXScaleCounter < 0) {
                    aDataPointer--;
                    tXScaleCounter = 2;
                }
            } else {
                // expand - show value several times
                tDisplayValue = *aDataPointer;
                tXScaleCounter--;
                if (tXScaleCounter == 0) {
                    aDataPointer++;
                    tXScaleCounter = mXDataScaleFactor;
                }
            }
            // check for data pointer still in data buffer
            if (aDataPointer >= tDataEndPointer) {
                break;
            }

            tDisplayValue = tYDisplayFactor * tDisplayValue;

            // clip to top value
            if (tDisplayValue > (int) mHeightY - 1) {
                tDisplayValue = mHeightY - 1;
            }
            // draw first value as pixel only
            if (aMode == CHART_MODE_PIXEL || tFirstValue) {
                tFirstValue = false;
                LocalDisplay.drawPixel(tXpos, mPositionY - tDisplayValue, mDataColor);
            } else if (aMode == CHART_MODE_LINE) {
                LocalDisplay.drawLineFastOneX(tXpos - 1, mPositionY - tLastValue, mPositionY - tDisplayValue, mDataColor);
            } else if (aMode == CHART_MODE_AREA) {
                //since we draw a 1 pixel line for value 0
                tDisplayValue += 1;
                LocalDisplay.fillRectRel(tXpos, mPositionY - tDisplayValue, 1, tDisplayValue, mDataColor);
            }
            tLastValue = tDisplayValue;
            tXpos++;
        }
#endif
}

/**
 * Draws a chart of values of the uint8_t data array pointed to by aDataPointer.
 * Do not apply scale values etc.
 * @param aDataPointer
 * @param aDataLength
 * @param aMode
 * @return false if clipping occurs
 */
bool Chart::drawChartDataDirect(const uint8_t *aDataPointer, const uint16_t aLengthOfValidData, const uint8_t aMode) {

    bool tRetValue = true;
    uint8_t tValue;
    uint16_t tDataLength = aLengthOfValidData;
    if (tDataLength > mWidthX) {
        tDataLength = mWidthX;
        tRetValue = false;
    }

// used only in line mode
    uint8_t tLastValue = *aDataPointer;
    if (tLastValue > mHeightY - 1) {
        tLastValue = mHeightY - 1;
        tRetValue = false;
    }

    uint16_t tXpos = mPositionX;

    for (; tDataLength > 0; tDataLength--) {
        tValue = *aDataPointer++;
        if (tValue > mHeightY - 1) {
            tValue = mHeightY - 1;
            tRetValue = false;
        }
        if (aMode == CHART_MODE_PIXEL) {
            tXpos++;
            DisplayForChart.drawPixel(tXpos, mPositionY - tValue, mDataColor);
        } else if (aMode == CHART_MODE_LINE) {
//          Should we use drawChartByteBuffer() instead?
            DisplayForChart.drawLineFastOneX(tXpos, mPositionY - tLastValue, mPositionY - tValue, mDataColor);
//          drawLine(tXpos, mPositionY - tLastValue, tXpos + 1, mPositionY - tValue,
//                  aDataColor);
            tXpos++;
            tLastValue = tValue;
        } else {
            // aMode == CHART_MODE_AREA
            tXpos++;
            //since we draw a 1 pixel line for value 0
            tValue += 1;
            DisplayForChart.fillRectRel(tXpos, mPositionY - tValue, 1, tValue, mDataColor);
        }
    }
    return tRetValue;
}

uint16_t Chart::getHeightY(void) const {
    return mHeightY;
}

uint16_t Chart::getPositionX(void) const {
    return mPositionX;
}

uint16_t Chart::getPositionY(void) const {
    return mPositionY;
}

uint16_t Chart::getWidthX(void) const {
    return mWidthX;
}

void Chart::setHeightY(uint16_t heightY) {
    mHeightY = heightY;
}

void Chart::setPositionX(uint16_t positionX) {
    mPositionX = positionX;
}

void Chart::setPositionY(uint16_t positionY) {
    mPositionY = positionY;
}

void Chart::setWidthX(uint16_t widthX) {
    mWidthX = widthX;
}

void Chart::setXLabelDistance(uint8_t aXLabelDistance) {
    mXLabelDistance = aXLabelDistance;
}

void Chart::setGridXPixelSpacing(uint8_t aGridXPixelSpacing) {
    mGridXPixelSpacing = aGridXPixelSpacing;
}

void Chart::setGridYPixelSpacing(uint8_t aGridYPixelSpacing) {
    mGridYPixelSpacing = aGridYPixelSpacing;
}

void Chart::setGridPixelSpacing(uint8_t aGridXPixelSpacing, uint8_t aGridYPixelSpacing) {
    mGridXPixelSpacing = aGridXPixelSpacing;
    mGridYPixelSpacing = aGridYPixelSpacing;
}

uint8_t Chart::getGridXPixelSpacing(void) const {
    return mGridXPixelSpacing;
}

uint8_t Chart::getGridYPixelSpacing(void) const {
    return mGridYPixelSpacing;
}

void Chart::setXLabelAndGridOffset(float aXLabelAndGridOffset) {
    mXLabelAndGridStartValueOffset = aXLabelAndGridOffset;
}

void Chart::setXLabelScaleFactor(int aXLabelScaleFactor) {
    mXLabelScaleFactor = aXLabelScaleFactor;
}

int Chart::getXLabelScaleFactor(void) const {
    return mXLabelScaleFactor;
}

void Chart::setXLabelAndXDataScaleFactor(int aXScaleFactor) {
    mXLabelScaleFactor = aXScaleFactor;
    mXDataScaleFactor = aXScaleFactor;
}

void Chart::setXDataScaleFactor(int8_t aIntegerScaleFactor) {
    mXDataScaleFactor = aIntegerScaleFactor;
}

int8_t Chart::getXDataScaleFactor(void) const {
    return mXDataScaleFactor;
}

/*
 * Label
 */
void Chart::setXLabelStartValue(int xLabelStartValue) {
    mXLabelStartValue.IntValue = xLabelStartValue;
}

void Chart::setXLabelStartValueFloat(float xLabelStartValueFloat) {
    mXLabelStartValue.FloatValue = xLabelStartValueFloat;
}

void Chart::setYLabelStartValue(int yLabelStartValue) {
    mYLabelStartValue.IntValue = yLabelStartValue;
}

void Chart::setYLabelStartValueFloat(float yLabelStartValueFloat) {
    mYLabelStartValue.FloatValue = yLabelStartValueFloat;
}

void Chart::setYDataFactor(float aYDataFactor) {
    mYDataFactor = aYDataFactor;
}

/**
 * not tested
 * @retval (YStartValue / mYFactor)
 */
uint16_t Chart::getYLabelStartValueRawFromFloat(void) {
    return (mYLabelStartValue.FloatValue / mYDataFactor);
}

/**
 * not tested
 * @retval (YEndValue = YStartValue + (scale * (mHeightY / GridYSpacing))  / mYFactor
 */
uint16_t Chart::getYLabelEndValueRawFromFloat(void) {
    return ((mYLabelStartValue.FloatValue + mYLabelIncrementValue.FloatValue * (mHeightY / mGridYPixelSpacing)) / mYDataFactor);
}

void Chart::setXLabelBaseIncrementValue(long xLabelBaseIncrementValue) {
    mXLabelBaseIncrementValue.LongValue = xLabelBaseIncrementValue;
}

void Chart::setXLabelBaseIncrementValueFloat(float xLabelBaseIncrementValueFloat) {
    mXLabelBaseIncrementValue.FloatValue = xLabelBaseIncrementValueFloat;
}

void Chart::setYLabelBaseIncrementValue(int yLabelBaseIncrementValue) {
    mYLabelIncrementValue.IntValue = yLabelBaseIncrementValue;
}

void Chart::setYLabelBaseIncrementValueFloat(float yLabelBaseIncrementValueFloat) {
    mYLabelIncrementValue.FloatValue = yLabelBaseIncrementValueFloat;
}

int_long_float_union Chart::getXLabelStartValue(void) const {
    return mXLabelStartValue;
}

int_float_union Chart::getYLabelStartValue(void) const {
    return mYLabelStartValue;
}

void Chart::disableXLabel(void) {
    mFlags &= ~CHART_X_LABEL_USED;
}

void Chart::disableYLabel(void) {
    mFlags &= ~CHART_Y_LABEL_USED;
}

void Chart::setTitleTextSize(const uint8_t aTitleTextSize) {
    mTitleTextSize = aTitleTextSize;
}

void Chart::setXTitleText(const char *aTitleText) {
    mXTitleText = aTitleText;
}

void Chart::setYTitleText(const char *aTitleText) {
    mYTitleText = aTitleText;
}

void Chart::setXAxisTimeDateSettings(drawXAxisTimeDateSettingsStruct *aDrawXAxisTimeDateSettingsStructToFill,
        int (*aFirstTokenFunction)(time_t t), int (*aSecondTokenFunction)(time_t t), char aTokenSeparatorChar,
        int (*aIntermediateTokenFunction)(time_t t), uint8_t aMaximumCharactersOfLabel) {
    aDrawXAxisTimeDateSettingsStructToFill->TokenSeparatorChar = aTokenSeparatorChar;
    aDrawXAxisTimeDateSettingsStructToFill->intermediateTokenFunction = aIntermediateTokenFunction;
    aDrawXAxisTimeDateSettingsStructToFill->firstTokenFunction = aFirstTokenFunction;
    aDrawXAxisTimeDateSettingsStructToFill->secondTokenFunction = aSecondTokenFunction;
    aDrawXAxisTimeDateSettingsStructToFill->maximumCharactersOfLabel = aMaximumCharactersOfLabel;
}

/**
 * Reduce value, if scale factor is expansion
 * Enlarge value if scale factor is compression
 * aIntegerScaleFactor > 1 : expansion by factor aIntegerScaleFactor. I.e. value -> (value / factor)
 * aIntegerScaleFactor == 1 : expansion by 1.5
 * aIntegerScaleFactor == 0 : identity
 * aIntegerScaleFactor == -1 : compression by 1.5
 * aIntegerScaleFactor < -1 : compression by factor -aIntegerScaleFactor -> (value * factor)
 * multiplies value with factor if aIntegerScaleFactor is < 0 (compression) or divide if aIntegerScaleFactor is > 0 (expansion)
 */
long Chart::reduceLongWithIntegerScaleFactor(long aValue, int aIntegerScaleFactor) {
    if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_1) {
        return aValue;
    }
    int tRetValue = aValue;
    if (aIntegerScaleFactor > CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5) {
        // scale factor is >= 2 | expansion -> reduce value
        tRetValue = aValue / aIntegerScaleFactor;
    } else if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5) {
        // value * 2/3
        tRetValue = (aValue * 2) / 3;
    } else if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
        // value * 3/2
        tRetValue = (aValue * 3) / 2;
    } else {
        // scale factor is <= -2 | compression -> enlarge value
        tRetValue = aValue * -aIntegerScaleFactor;
    }
    return tRetValue;
}

/*
 * Compute xDataFactor such, that data fills X axis
 */
int8_t Chart::computeXFactor(uint16_t aDataLength) {
    int8_t tNewScale;
    if (mWidthX > aDataLength) {
        /*
         * Expand data -> positive scale values
         */
        tNewScale = mWidthX / aDataLength;
    } else {
        /*
         * Compress data -> negative scale values
         */
        tNewScale = -(aDataLength / mWidthX);
    }
    if (tNewScale == 1) {
        tNewScale = CHART_X_AXIS_SCALE_FACTOR_1;
    }
    return tNewScale;
}

void Chart::computeAndSetXLabelAndXDataScaleFactor(uint16_t aDataLength) {
    setXLabelAndXDataScaleFactor(computeXFactor(aDataLength));
}

void Chart::getIntegerScaleFactorAsString(char *tStringBuffer, int aIntegerScaleFactor) {
    if (aIntegerScaleFactor >= CHART_X_AXIS_SCALE_FACTOR_1) {
        *tStringBuffer++ = '*';
        aIntegerScaleFactor = -aIntegerScaleFactor; // for adjustIntWithIntegerScaleFactor() down below scaleFactor must be negative
    } else {
        *tStringBuffer++ = '\xF7'; // division
    }
    if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
        *tStringBuffer++ = '1';
        *tStringBuffer++ = '.';
        *tStringBuffer++ = '5';
        *tStringBuffer++ = '\0';
    } else {
        snprintf(tStringBuffer, 5, "%-3ld", reduceLongWithIntegerScaleFactor(1, aIntegerScaleFactor));
    }

}

/**
 * multiplies value with aIntegerScaleFactor if aIntegerScaleFactor is < -1 or divide if aIntegerScaleFactor is > 1
 */
float Chart::reduceFloatWithIntegerScaleFactor(float aValue, int aIntegerScaleFactor) {

    if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_1) {
        return aValue;
    }
    float tRetValue = aValue;
    if (aIntegerScaleFactor > CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5) {
        // scale factor is >= 2 | expansion -> reduce value
        tRetValue = aValue / aIntegerScaleFactor;
    } else if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5) {
        // value * 2/3
        tRetValue = aValue * 0.666666666;
    } else if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
        // value * 1.5
        tRetValue = aValue * 1.5;
    } else {
        // scale factor is <= -2 | compression -> enlarge value
        tRetValue = aValue * -aIntegerScaleFactor;
    }
    return tRetValue;
}

/*
 * Show charts features
 */
#define CHART_1_LENGTH 120
#define CHART_2_LENGTH 140
#define CHART_3_LENGTH 180
#if !defined(DISPLAY_HEIGHT)
#define DISPLAY_HEIGHT_IS_DEFINED_LOCALLY
#define DISPLAY_HEIGHT 240
#endif
void showChartDemo(void) {
    Chart ChartExample;

    /*
     * allocate memory for 180 int16_t values
     */
    int16_t *tChartBufferPtr = (int16_t*) malloc(sizeof(int16_t) * CHART_3_LENGTH);
    if (tChartBufferPtr == NULL) {
#if !defined(ARDUINO)
        failParamMessage(sizeof(int16_t) * CHART_3_LENGTH, "malloc failed");
#else
        DisplayForChart.drawText(0, 2 * TEXT_SIZE_11, "malloc of 360 byte buffer failed", TEXT_SIZE_11, COLOR16_RED,
        COLOR16_WHITE);
#  if defined(LOCAL_TEST)
        printRAMInfo(&Serial); // Stack used is 126 bytes
#  endif

#endif
        return;
    }

    /*
     * 1. Chart: 120 8-bit values, pixel with grid, no labels, height = 90, axes size = 2, no grid
     */
    ChartExample.disableXLabel();
    ChartExample.disableYLabel();
    ChartExample.initChartColors(COLOR16_RED, COLOR16_RED, CHART_DEFAULT_GRID_COLOR, COLOR16_RED, COLOR16_WHITE);
    ChartExample.initChart(5, DISPLAY_HEIGHT - 20, CHART_1_LENGTH, 90, 2, TEXT_SIZE_11, CHART_DISPLAY_GRID, 0, 0);
    ChartExample.setGridPixelSpacing(20, 20);
    ChartExample.drawAxesAndGrid();

    char *tRandomByteFillPointer = (char*) tChartBufferPtr;
//generate random data
#if defined(ARDUINO)
    for (unsigned int i = 0; i < CHART_1_LENGTH; i++) {
        *tRandomByteFillPointer++ = 30 + random(31);
    }
#else
    srand(120);
    for (unsigned int i = 0; i < CHART_1_LENGTH; i++) {
        *tRandomByteFillPointer++ = 30 + (rand() >> 27);
    }
#endif
    ChartExample.drawChartDataDirect((uint8_t*) tChartBufferPtr, CHART_1_LENGTH, CHART_MODE_PIXEL);

    delay(1000);
    /*
     * 2. Chart: 140 16-bit values, with grid, with (negative) integer Y labels and X label offset 5 and 2 labels spacing
     */
// new random data
    int16_t *tRandomShortFillPointer = tChartBufferPtr;
    int16_t tDataValue = -15;
#if defined(ARDUINO)
    for (unsigned int i = 0; i < CHART_2_LENGTH; i++) {
        tDataValue += random(-3, 6);
        *tRandomShortFillPointer++ = tDataValue;
    }
#else
    for (unsigned int i = 0; i < CHART_2_LENGTH; i++) {
        tDataValue += rand() >> 29;
        *tRandomShortFillPointer++ = tDataValue;
    }
#endif

    ChartExample.initXLabelInteger(0, 10, CHART_X_AXIS_SCALE_FACTOR_1, 2);
    ChartExample.setXLabelAndGridOffset(5);
    ChartExample.setXLabelDistance(2);
    ChartExample.initYLabelInt(-20, 20, 20 / 15, 3);
    ChartExample.initChart(170, DISPLAY_HEIGHT - 20, CHART_2_LENGTH, 88, 2, TEXT_SIZE_11, CHART_DISPLAY_GRID, 15, 15);
    ChartExample.drawAxesAndGrid();
    ChartExample.initChartColors(COLOR16_RED, COLOR16_BLUE, COLOR16_GREEN, COLOR16_BLACK, COLOR16_WHITE);
    ChartExample.drawChartData(tChartBufferPtr, CHART_2_LENGTH, CHART_MODE_LINE);

    /*
     * 3. Chart: 140 16-bit values, without grid, with float labels, area mode
     */
// new random data
    tRandomShortFillPointer = tChartBufferPtr;
    tDataValue = 0;
#if defined(ARDUINO)
    for (unsigned int i = 0; i < CHART_3_LENGTH; i++) {
        tDataValue += random(-2, 4);
        *tRandomShortFillPointer++ = tDataValue;
    }
#else
    for (unsigned int i = 0; i < CHART_3_LENGTH; i++) {
        tDataValue += rand() >> 30;
        *tRandomShortFillPointer++ = tDataValue;
    }
#endif

    ChartExample.initXLabelFloat(0, 0.5, CHART_X_AXIS_SCALE_FACTOR_1, 3, 1);
    ChartExample.initYLabelFloat(0, 0.3, 1.3 / 60, 3, 1); // display 1.3 for raw value of 60
    ChartExample.initChart(30, 100, CHART_3_LENGTH, 90, 2, TEXT_SIZE_11, CHART_DISPLAY_NO_GRID, 30, 16);
    ChartExample.drawAxesAndGrid();
    ChartExample.drawChartData(tChartBufferPtr, CHART_3_LENGTH, CHART_MODE_AREA);

    free(tChartBufferPtr);
}

#if defined(DISPLAY_HEIGHT_IS_DEFINED_LOCALLY)
#undef DISPLAY_HEIGHT
#endif

/** @} */
/** @} */
#undef DisplayForChart
#if defined(LOCAL_DEBUG)
#undef LOCAL_DEBUG
#endif
#endif // _CHART_HPP
