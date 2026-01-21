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
 *      - getRequestedDisplayWidth()
 *      - fillRect()
 *      - fillRectRel()
 *      - drawText()
 *      - drawPixel()
 *      - drawLineFastOneX()
 *      - TEXT_SIZE_11_WIDTH
 *      - TEXT_SIZE_11_HEIGH
 *
 *  Schema of the origin of a graph with axes size of 2
 *   |  |
 *   |  | 2 - x and y are 2 here
 *   |  |1____
 *   | 0
 *   |________
 *
 *  Copyright (C) 2012-2026  Armin Joachimsmeyer
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

#define CHART_LABEL_STRING_BUFFER_SIZE  10

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
    mYLabelColor = mXLabelColor = CHART_DEFAULT_LABEL_COLOR;
    mFlags = 0;
    mXDataScaleFactor = mXLabelScaleFactor = CHART_X_AXIS_SCALE_FACTOR_1;
    mXLabelAndGridStartValueOffset = 0.0;
    mXBigLabelDistance = mXLabelDistance = 1;
    mYTitleText = mXTitleText = nullptr;
    XLabelStringFunction = nullptr; // required
}

void Chart::initChartColors(const color16_t aDataColor, const color16_t aAxesColor, const color16_t aGridColor,
        const color16_t aXLabelColor, const color16_t aYLabelColor, const color16_t aBackgroundColor) {
    mDataColor = aDataColor;
    mAxesColor = aAxesColor;
    mGridColor = aGridColor;
    mXLabelColor = aXLabelColor;
    mYLabelColor = aYLabelColor;
    mBackgroundColor = aBackgroundColor;
}

void Chart::setDataColor(color16_t aDataColor) {
    mDataColor = aDataColor;
}

void Chart::setBackgroundColor(color16_t aBackgroundColor) {
    mBackgroundColor = aBackgroundColor;
}

void Chart::setLabelColor(color16_t aLabelColor) {
    mXLabelColor = aLabelColor;
    mYLabelColor = aLabelColor;
}

/**
 * aPositionX and aPositionY are the 0 coordinates of the grid and part of the axes
 */
uint8_t Chart::initChart(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX, const uint16_t aHeightY,
        const uint8_t aAxesSize, const uint8_t aLabelTextSize, const bool aHasGrid, const uint16_t aXGridOrLabelPixelSpacing,
        const uint16_t aYGridOrLabelPixelSpacing) {
    mPositionX = aPositionX;
    mPositionY = aPositionY;
    mWidthX = aWidthX;
    mHeightY = aHeightY;
    mAxesSize = aAxesSize;
    mLabelTextSize = aLabelTextSize;
    mTitleTextSize = aLabelTextSize;
    mXGridOrLabelPixelSpacing = aXGridOrLabelPixelSpacing;
    mYGridOrLabelPixelSpacing = aYGridOrLabelPixelSpacing;

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
    if (mPositionY > DisplayForChart.getRequestedDisplayHeight() - t2AxesSize) {
        mPositionY = DisplayForChart.getRequestedDisplayHeight() - t2AxesSize;
        tRetValue = CHART_ERROR_POS_Y;
    }
    if (mPositionX + mWidthX > DisplayForChart.getRequestedDisplayWidth()) {
        mPositionX = 0;
        mWidthX = 100;
        tRetValue = CHART_ERROR_WIDTH;
    }

    if (mHeightY > mPositionY + 1) {
        mHeightY = mPositionY + 1;
        tRetValue = CHART_ERROR_HEIGHT;
    }

    if (mXGridOrLabelPixelSpacing > mWidthX) {
        mXGridOrLabelPixelSpacing = mWidthX / 2;
        tRetValue = CHART_ERROR_GRID_X_SPACING;
    }
    return tRetValue;
}

/**
 * @param aXLabelStartValue
 * @param aXLabelIncrementValue Value relates to CHART_X_AXIS_SCALE_FACTOR_1 / identity. long is required for long time increments (1 year requires 25 bit)
 * @param aXLabelScaleFactor Current value to use for X scale
 * @param aXMinStringWidth
 */
void Chart::initXLabelTimestamp(const int aXLabelStartValue, const long aXLabelIncrementValue, const uint8_t aXLabelScaleFactor,
        const uint8_t aXMinStringWidth) {
    mXLabelStartValue.TimeValue = aXLabelStartValue;
    mXLabelBaseIncrementValue = aXLabelIncrementValue;
    mXLabelScaleFactor = aXLabelScaleFactor;
    mXMinStringWidth = aXMinStringWidth;
    mFlags |= CHART_X_LABEL_TIME | CHART_X_LABEL_USED;
}


/**
 * @param aXLabelStartValue
 * @param aXLabelIncrementValue Value relates to CHART_X_AXIS_SCALE_FACTOR_1 / identity. long is required for long time increments (1 year requires 25 bit)
 * @param aXMinStringWidth
 */
void Chart::initXLabelTimestampForLabelScaleIdentity(const int aXLabelStartValue, const long aXLabelIncrementValue, const uint8_t aXMinStringWidth) {
    mXLabelStartValue.TimeValue = aXLabelStartValue;
    mXLabelBaseIncrementValue = aXLabelIncrementValue;
    mXMinStringWidth = aXMinStringWidth;
    mFlags |= CHART_X_LABEL_TIME | CHART_X_LABEL_USED;
}

/**
 * @param aXLabelStartValue
 * @param aXLabelIncrementValue
 * @param aIntegerScaleFactor
 * @param aXMinStringWidthIncDecimalPoint
 * @param aXNumVarsAfterDecimal
 */
void Chart::initXLabel(const float aXLabelStartValue, const float aXLabelIncrementValue, const uint8_t aXLabelScaleFactor,
        uint8_t aXMinStringWidthIncDecimalPoint, uint8_t aXNumVarsAfterDecimal) {
    mXLabelStartValue.FloatValue = aXLabelStartValue;
    mXLabelBaseIncrementValue = aXLabelIncrementValue;
    mXLabelScaleFactor = aXLabelScaleFactor;
    mXNumVarsAfterDecimal = aXNumVarsAfterDecimal;
    if (aXMinStringWidthIncDecimalPoint >= CHART_LABEL_STRING_BUFFER_SIZE) {
        aXMinStringWidthIncDecimalPoint = CHART_LABEL_STRING_BUFFER_SIZE - 1;
    }
    mXMinStringWidth = aXMinStringWidthIncDecimalPoint;
    mFlags &= ~CHART_X_LABEL_TIME;
    if (aXMinStringWidthIncDecimalPoint != 0) {
        mFlags |= CHART_X_LABEL_USED;
    }
}

/**
 * @param aYLabelStartValue
 * @param aYLabelIncrementValue
 * @param aYFactor factor for input to chart value - e.g. (3.0 / 4096) for adc reading of 4096 for 3 Volt
 * @param aYMinStringWidthIncDecimalPoint for y axis label
 * @param aYNumVarsAfterDecimal for y axis label
 */
void Chart::initYLabel(const float aYLabelStartValue, const float aYLabelIncrementValue, const float aYFactor,
        const uint8_t aYMinStringWidthIncDecimalPoint, const uint8_t aYNumVarsAfterDecimal) {
    mYLabelStartValue = aYLabelStartValue;
    mYLabelIncrementValue = aYLabelIncrementValue;
    mYMinStringWidth = aYMinStringWidthIncDecimalPoint;
    mYNumVarsAfterDecimal = aYNumVarsAfterDecimal;
    mYDataFactor = aYFactor;
    mFlags |= CHART_Y_LABEL_USED;
}

/**
 * Render the chart on the lcd
 */
void Chart::drawAxesAndGrid(void) {
    drawAxesAndLabels();
    drawGrid();
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
    if (mXTitleText != nullptr) {
        /**
         * draw axis title
         */
        uint16_t tTextLenPixel = strlen(mXTitleText) * getTextWidth(mTitleTextSize);
        DisplayForChart.drawText(mPositionX + mWidthX - tTextLenPixel - 1, mPositionY - mTitleTextSize, mXTitleText, mTitleTextSize,
                mXLabelColor, mBackgroundColor);
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

void Chart::reduceWithXLabelScaleFactor(time_float_union *aValue) {
    if (mFlags & CHART_X_LABEL_TIME) {
        aValue->TimeValue = reduceLongWithIntegerScaleFactor(aValue->TimeValue, mXLabelScaleFactor);
    } else {
        aValue->FloatValue = reduceFloatWithIntegerScaleFactor(aValue->FloatValue, mXLabelScaleFactor);
    }
}

long Chart::reduceLongWithXLabelScaleFactor(long aValue) {
    return reduceLongWithIntegerScaleFactor(aValue, mXLabelScaleFactor);
}

float Chart::reduceFloatWithXLabelScaleFactor(float aValue) {
    return reduceFloatWithIntegerScaleFactor(aValue, mXLabelScaleFactor);
}

/**
 * Enlarge value if scale factor is expansion
 * Reduce value, if scale factor is compression
 */
long Chart::enlargeLongWithXLabelScaleFactor(long aValue) {
    return reduceLongWithIntegerScaleFactor(aValue, -mXLabelScaleFactor);
}

float Chart::enlargeFloatWithXLabelScaleFactor(float aValue) {
    return reduceFloatWithIntegerScaleFactor(aValue, -mXLabelScaleFactor);
}

void Chart::setLabelStringFunction(int (*aXLabelStringFunction)(char *aLabelStringBuffer, time_float_union aXvalue)) {
    XLabelStringFunction = aXLabelStringFunction;
}

/*
 * Here we take the long part of the value as minutes and return strings like "27" or "2:27"
 */
int Chart::convertMinutesToString(char *aLabelStringBuffer, time_float_union aXValueMinutes) {
    uint16_t tMinutes = (aXValueMinutes.FloatValue + 0.5);
//    uint16_t tMinutes = (aXvalue.FloatValue);
    uint8_t tHours = tMinutes / 60;
    tMinutes = tMinutes % 60;
#if defined(__AVR__)
    if (tHours == 0) {
        return snprintf_P(aLabelStringBuffer, CHART_LABEL_STRING_BUFFER_SIZE, PSTR("%u"), tMinutes);
    } else {
        return snprintf_P(aLabelStringBuffer, CHART_LABEL_STRING_BUFFER_SIZE, PSTR("%u:%02u"), tHours, tMinutes);
    }
#else
    if (tHours == 0) {
        return snprintf(aLabelStringBuffer, CHART_LABEL_STRING_BUFFER_SIZE, "%u", tMinutes);
    } else {
        return snprintf(aLabelStringBuffer, CHART_LABEL_STRING_BUFFER_SIZE, "%u:%02u", tHours, tMinutes);
    }
#endif
}

/*
 * Draw 1 pixel lines at each X and Y label position
 * Start not at axis but at mPositionX + 1 in order not to overwrite the axis
 */
void Chart::drawGrid(void) {
    if (!(mFlags & CHART_HAS_GRID)) {
        return;
    }
    int16_t tXPixelOffsetOfCurrentLine = 0;
    if (mXLabelAndGridStartValueOffset != 0.0) {
        /*
         * If mXLabelAndGridStartValueOffset == mXLabelBaseIncrementValue then we start with 2. grid at the original start position
         * Pixel offset is (mXLabelAndGridStartValueOffset / mXLabelBaseIncrementValue) * mXGridOrLabelPixelSpacing
         * or  (xScaleAdjusted(mXLabelAndGridStartValueOffset) / mXLabelBaseIncrementValue) * mXGridOrLabelPixelSpacing
         * If offset is positive -> 1. grid is left of origin and chart is shifted left.
         */
        tXPixelOffsetOfCurrentLine = -(mXLabelAndGridStartValueOffset * mXGridOrLabelPixelSpacing)
                / reduceFloatWithXLabelScaleFactor(mXLabelBaseIncrementValue);
    }
#if defined(LOCAL_DEBUG)
    Serial.println();
    Serial.print(F("PixelOffset of first grid="));
    Serial.print(tXPixelOffsetOfCurrentLine);
    Serial.print(F(" gridSpacing="));
    Serial.print(mXGridOrLabelPixelSpacing);
    Serial.print(F(" PositionX="));
    Serial.println(mPositionX);
#endif
    tXPixelOffsetOfCurrentLine += mXGridOrLabelPixelSpacing; // Do not render first line, it is on Y axis

// draw 1 pixel thick vertical lines at each X label position (even if the label will not rendered later)
    do {
        if (tXPixelOffsetOfCurrentLine > 0) {
            // -2 because it results in a line of length -1 and we do not start at origin, but at Y position 1 in order not to overwrite the X axis
            DisplayForChart.drawLineRel(mPositionX + tXPixelOffsetOfCurrentLine, mPositionY - (mHeightY - 1), 0, mHeightY - 2,
                    mGridColor);
        }
        tXPixelOffsetOfCurrentLine += mXGridOrLabelPixelSpacing;
    } while (tXPixelOffsetOfCurrentLine < (int16_t) mWidthX);

// draw 1 pixel thick horizontal lines at each Y label position
    for (uint16_t tYOffset = mYGridOrLabelPixelSpacing; tYOffset <= mHeightY; tYOffset += mYGridOrLabelPixelSpacing) {
        // -2 because it results in a line of length -1 and we do not start at origin, but at X position 1 in order not to overwrite the Y axis
        DisplayForChart.drawLineRel(mPositionX + 1, mPositionY - tYOffset, mWidthX - 2, 0, mGridColor);
    }
}

/**
 * Draw X line with indicators and labels
 * Render indicators if labels enabled, but no grid is specified
 * Label increment value is adjusted with scale factor
 */
void Chart::drawXAxisAndLabels() {

    char tLabelStringBuffer[CHART_LABEL_STRING_BUFFER_SIZE];

    uint16_t tPositionY = mPositionY; // saves unbelievable 100 bytes
    /*
     * Draw X axis thick line, take mAxesSize into account
     * Must be identical to the line in clear()
     * if mAxesSize is 1 then the term (mAxesSize - 1) is 0
     */
    DisplayForChart.fillRectRel(mPositionX - (mAxesSize - 1), tPositionY, mWidthX + (mAxesSize - 1), mAxesSize, mAxesColor);

    if (mFlags & CHART_X_LABEL_USED) {
        /*
         * Clear complete label space before
         */
        uint16_t tNumberYTop = tPositionY + 2 * mAxesSize;
#if !defined(ARDUINO)
        assertParamMessage((tNumberYTop <= (DisplayForChart.getRequestedDisplayHeight() - mLabelTextSize)),
                tNumberYTop, "no space for x labels");
#endif
        uint8_t tTextWidth = getTextWidth(mLabelTextSize);
        int16_t tXStringPixelOffset = ((tTextWidth * mXMinStringWidth) / 2) + 1;
        DisplayForChart.fillRect(mPositionX - tXStringPixelOffset, tNumberYTop, mPositionX + mWidthX + tXStringPixelOffset + 1,
                tNumberYTop + getTextHeight(mLabelTextSize), mBackgroundColor);

        /*
         * Compute pixel offset of first label
         *
         * If mXLabelAndGridStartValueOffset == mXLabelBaseIncrementValue then label starts with 2. major value
         * Pixel offset is (mXLabelAndGridStartValueOffset / mXLabelBaseIncrementValue) * mXGridOrLabelPixelSpacing
         * If offset is positive -> 1. label is left of origin and chart is shifted left.
         * e.g. Offset is 1/2 tEffectiveXLabelDistance pixel smaller or left, if mXLabelAndGridStartValueOffset is 1/2 of mXLabelBaseIncrementValue
         */
        int16_t tXPixelOffsetOfCurrentLabel = 0;
        if (mXLabelAndGridStartValueOffset != 0.0) {
            tXPixelOffsetOfCurrentLabel = -(mXLabelAndGridStartValueOffset * mXGridOrLabelPixelSpacing)
                    / reduceFloatWithXLabelScaleFactor(mXLabelBaseIncrementValue);
        }
#if defined(LOCAL_DEBUG)
        Serial.println();
        Serial.print(F("PixelOffset of first label="));
        Serial.print(tXPixelOffsetOfCurrentLabel);
        Serial.print(F(" XLabelAndGridStartValueOffset="));
        Serial.print(mXLabelAndGridStartValueOffset, 0);
        Serial.print(F(" reduced increment float="));
        Serial.print(reduceFloatWithXLabelScaleFactor(mXLabelBaseIncrementValue), 0);
        Serial.print(F(" gridSpacing="));
        Serial.println(mXGridOrLabelPixelSpacing);
#endif
        /*
         * Draw indicator and label numbers
         */
        if (!(mFlags & CHART_HAS_GRID)) {
            /*
             * Draw indicators with the same size the axis has, do not skip one
             */
            for (int16_t tXPixelOffsetOfCurrentIndicator = tXPixelOffsetOfCurrentLabel;
                    tXPixelOffsetOfCurrentIndicator < (int16_t) mWidthX; tXPixelOffsetOfCurrentIndicator +=
                            mXGridOrLabelPixelSpacing) {
                if (tXPixelOffsetOfCurrentIndicator >= 0) {
                    DisplayForChart.fillRectRel(mPositionX + tXPixelOffsetOfCurrentIndicator, tPositionY + mAxesSize, 1, mAxesSize,
                            mGridColor);
                }
            }
            tNumberYTop += mAxesSize; // move labels downwards, to see indicators
        }

        /*
         * loop for drawing labels at X axis
         */
        // initialize both variables to avoid compiler warnings
        time_float_union tValueForLabel;
        tValueForLabel = mXLabelStartValue;
        /*
         * Use float, because otherwise we get rounding errors for big scale factors
         * i.e. scale factor 8 and 30 minutes give 3,75 minutes per label -> this works only with float.
         * long is only for timestamps
         */
        float tIncrementValueForLabel;
        tIncrementValueForLabel = mXLabelDistance * reduceFloatWithXLabelScaleFactor(mXLabelBaseIncrementValue);

        do {
            uint8_t tStringLength;

            // Do increments -and string generation :-( - for every step. This saves around 50 bytes, but is slower

#if defined(__AVR__)
            if (XLabelStringFunction != nullptr) {
                tStringLength = XLabelStringFunction(tLabelStringBuffer, tValueForLabel);
            } else {
                dtostrf(tValueForLabel.FloatValue, mXMinStringWidth, mXNumVarsAfterDecimal, tLabelStringBuffer);
            }
            tStringLength = strlen(tLabelStringBuffer); // do not know, if it works here ...
#else
            if (XLabelStringFunction != nullptr) {
                tStringLength = XLabelStringFunction(tLabelStringBuffer, tValueForLabel);
            } else {
                tStringLength = snprintf(tLabelStringBuffer, sizeof(tLabelStringBuffer), "%*.*f", mXMinStringWidth,
                        mXNumVarsAfterDecimal, tValueForLabel.FloatValue);
            }

#endif
            tValueForLabel.FloatValue += tIncrementValueForLabel;

            if (tXPixelOffsetOfCurrentLabel >= 0) {
                /*
                 * Compute offset to place it at the middle
                 * tStringLength is without trailing \0
                 */
                tXStringPixelOffset = (tTextWidth * tStringLength) / 2;

                DisplayForChart.drawText(mPositionX + tXPixelOffsetOfCurrentLabel - tXStringPixelOffset, tNumberYTop,
                        tLabelStringBuffer, mLabelTextSize, mXLabelColor, mBackgroundColor);
            }

            tXPixelOffsetOfCurrentLabel += mXGridOrLabelPixelSpacing * mXLabelDistance; // skip labels, computing it here saves 12 bytes
        } while (tXPixelOffsetOfCurrentLabel < (int16_t) mWidthX);
    }
}

/**
 * Draws the x line with regular labels drawn at each mXLabelDistance grid position.
 * Big labels start at at mXLabelAndGridStartValueOffset and are drawn each mXBigLabelDistance,
 * enlarged by XLabelScaleFactor, to keep the time distance between them constant.
 *
 * @param aStartTimestamp  Timestamp of the big label, which has the offset mXLabelAndGridStartValueOffset.
 *  It may not be rendered, depending on mXLabelAndGridStartValueOffset.
 * @param mXLabelDistance  Distance between 2 labels at scale factor 1
 */
void Chart::drawXAxisAndDateLabels(time_t aStartTimestamp,
        int (*aXBigLabelStringFunction)(char *aLabelStringBuffer, time_float_union aXvalue)) {

    char tLabelStringBuffer[8]; // 12:15 are 6 characters 12/2024 are 8 character

    /*
     * Draw X axis line, take mAxesSize into account
     * Must be identical to the line in clear()
     */
    DisplayForChart.fillRectRel(mPositionX - (mAxesSize - 1), mPositionY, mWidthX + (mAxesSize - 1), mAxesSize, mAxesColor);

    /*
     * Clear complete label space
     */
    uint16_t tNumberYTop = mPositionY + 2 * mAxesSize;
#if !defined(ARDUINO)
    assertParamMessage((tNumberYTop <= (DisplayForChart.getRequestedDisplayHeight() - getTextDecend(mLabelTextSize))), tNumberYTop,
            "no space for x labels");
#endif
    int16_t tXStringPixelOffset = ((getTextWidth(mLabelTextSize) * mXMinStringWidth) / 2) + 1;
    BlueDisplay1.fillRect(mPositionX - tXStringPixelOffset, mPositionY + mAxesSize + 1,
            mPositionX + mWidthX + (tXStringPixelOffset + 1), tNumberYTop + getTextHeight(mLabelTextSize), mBackgroundColor);

    /*
     * Compute effective label distance, as multiple of grid lines
     * Effective distance can be greater than 1 only if distance is > 1 and we have a integer expansion of scale
     */
    uint8_t tBigXLabelDistance = enlargeLongWithXLabelScaleFactor(mXBigLabelDistance);
    if (tBigXLabelDistance < mXLabelDistance) {
        tBigXLabelDistance = mXLabelDistance;
    }

    /*
     * Compute pixel offset of first label
     *
     * If mXLabelAndGridStartValueOffset == mXLabelBaseIncrementValue then label starts with 2. main value
     * Pixel offset is (mXLabelAndGridStartValueOffset / mXLabelBaseIncrementValue) * mXGridOrLabelPixelSpacing
     * If offset is positive => 1. big label is left of origin and grid, starting at big label, is shifted left with part left of origin not printed.
     * e.g. Offset is 1/2 tEffectiveXLabelDistance pixel smaller or left, if mXLabelAndGridStartValueOffset is 1/2 of mXLabelBaseIncrementValue
     */
    int16_t tXPixelOffsetOfCurrentLabel = 0;
    if (mXLabelAndGridStartValueOffset != 0.0) {
// mXLabelAndGridStartValueOffset is float, so we do not have an overflow here
        tXPixelOffsetOfCurrentLabel = -(mXLabelAndGridStartValueOffset * mXGridOrLabelPixelSpacing)
                / reduceFloatWithXLabelScaleFactor(mXLabelBaseIncrementValue);
    }

#if defined(LOCAL_DEBUG)
    Serial.println();
    Serial.print(F("PixelOffset of first big label="));
    Serial.print(tXPixelOffsetOfCurrentLabel);
    Serial.print(F(" XLabelAndGridStartValueOffset="));
    Serial.print(mXLabelAndGridStartValueOffset, 0);
    Serial.print(F(" reduced increment("));
    Serial.print(mXLabelBaseIncrementValue);
    Serial.print(F(")="));
    Serial.print(reduceLongWithXLabelScaleFactor(mXLabelBaseIncrementValue));
    Serial.print(F(" time increment="));
    Serial.print(mXLabelDistance * reduceLongWithXLabelScaleFactor(mXLabelBaseIncrementValue));
    Serial.print(F(" BigXLabelDistance="));
    Serial.print(tBigXLabelDistance);
    Serial.print(F(" gridPixelSpacing="));
    Serial.print(mXGridOrLabelPixelSpacing);
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
        for (int16_t tGridOffset = tXPixelOffsetOfCurrentLabel; tGridOffset < (int16_t) mWidthX; tGridOffset +=
                mXGridOrLabelPixelSpacing) {
            if (tGridOffset >= 0) {
                // Only draw indicators at or right of origin
                BlueDisplay1.fillRectRel(mPositionX + tGridOffset, mPositionY + mAxesSize, 1, mAxesSize, mGridColor);
            }
        }
        tNumberYTop += mAxesSize; // move labels downwards, to see indicators
    }

    /*
     * Draw a label every mXLabelDistance
     * Always start with a big label and draw it at every tBigXLabelDistance grid line if right of origin
     */
    uint8_t tGridIndex = 0;
    time_float_union tTimeStampForLabel;
    tTimeStampForLabel.TimeValue = aStartTimestamp;
    long tIncrementValue = mXLabelDistance * reduceLongWithXLabelScaleFactor(mXLabelBaseIncrementValue);
    do {
        uint8_t tCurrentTextSize = mLabelTextSize;
        uint8_t tCurrentTextWidth = getTextWidth(mLabelTextSize);
        uint8_t tStringLength;

        if (tXPixelOffsetOfCurrentLabel >= 0) {
            // Only draw labels at or right of origin
            if ((tGridIndex % tBigXLabelDistance) == 0) {
                /*
                 * Big label here
                 */
                tStringLength = aXBigLabelStringFunction(tLabelStringBuffer, tTimeStampForLabel);
                tCurrentTextSize += tCurrentTextSize / CHART_DIVISOR_TO_ADD_FOR_BIG_LABEL_TEXT_SIZE;
                tCurrentTextWidth += tCurrentTextWidth / CHART_DIVISOR_TO_ADD_FOR_BIG_LABEL_TEXT_SIZE;
            } else {
                /*
                 * Regular label / small label here
                 */
                tStringLength = XLabelStringFunction(tLabelStringBuffer, tTimeStampForLabel);
            }
            /*
             * Compute pixel offset in order to place the label at the middle of the indicator
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
             * Draw label
             */
            BlueDisplay1.drawText(mPositionX + tXPixelOffsetOfCurrentLabel - tXStringPixelOffset, tNumberYTop, tLabelStringBuffer,
                    tCurrentTextSize, mXLabelColor, mBackgroundColor);
        }
#if defined(LOCAL_DEBUG)
        Serial.println();
        Serial.print(F("XPixelOffsetOfCurrentLabel="));
        Serial.print(tXPixelOffsetOfCurrentLabel);
        Serial.print(F(" GridIndex="));
        Serial.print(tGridIndex);
        Serial.print(F(" Timestamp="));
        Serial.println(tTimeStampForLabel.TimeValue);
#endif

// set values for next loop
        tTimeStampForLabel.TimeValue += tIncrementValue;
        tXPixelOffsetOfCurrentLabel += mXGridOrLabelPixelSpacing * mXLabelDistance; // computing it here saves 12 bytes
        tGridIndex += mXLabelDistance;

    } while (tXPixelOffsetOfCurrentLabel < (int16_t) mWidthX);
}

/**
 * Set x label start to index.th value - start not with first but with startIndex label
 * redraw Axis
 */
void Chart::setXLabelStartValueByIndex(const int aNewXStartIndex, const bool doDraw) {
    mXLabelStartValue.FloatValue = mXLabelBaseIncrementValue * aNewXStartIndex;
    if (doDraw) {
        drawXAxisAndLabels();
    }
}

/**
 * Increments or decrements the start value by one increment value (one grid line)
 * and redraws Axis
 * does not decrement below 0
 */
float Chart::stepXLabelStartValue(const bool aDoIncrement) {
    if (aDoIncrement) {
        mXLabelStartValue.FloatValue += mXLabelBaseIncrementValue;
    } else {
        mXLabelStartValue.FloatValue -= mXLabelBaseIncrementValue;
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
 */
void Chart::drawYAxisTitle(const int aYOffset) const {
    if (mYTitleText != nullptr) {
        /**
         * draw axis title - use data color
         */
        DisplayForChart.drawText(mPositionX + mAxesSize + 1, mPositionY - mHeightY + aYOffset, mYTitleText, mTitleTextSize,
                mYLabelColor, mBackgroundColor);
    }
}
/**
 * draw y title starting at the Y axis
 * Use data color, because it is the legend for the data
 * @param aYOffset the offset in pixel below the top of the Y line
 * @param aXOffset the offset in pixel left of the Y line
 */
void Chart::drawYAxisTitle(const int aYOffset, const int aXOffset) const {
    if (mYTitleText != nullptr) {
        /**
         * draw axis title - use data color
         */
        DisplayForChart.drawText(mPositionX + mAxesSize + 1 - aXOffset, mPositionY - mHeightY + aYOffset, mYTitleText,
                mTitleTextSize, mYLabelColor, mBackgroundColor);
    }
}

/**
 * draw Y thick line with indicators and labels
 * Draw Y thick line, such that 0 is on the line and 1 is beneath it.
 * renders indicators if labels but no grid are specified
 */
void Chart::drawYAxisAndLabels() {

    char tLabelStringBuffer[CHART_LABEL_STRING_BUFFER_SIZE];

    int16_t tPositionX = mPositionX;

    /*
     * Draw Y axis line, such that origin / 0 is on the line and 1 is beneath it.
     * Must be identical to the line in clear()
     * if mAxesSize is 1 then the term (mAxesSize - 1) is 0
     * (mHeightY - 1) because we do not overwrite the X axis
     */
    DisplayForChart.fillRectRel(tPositionX - (mAxesSize - 1), mPositionY - (mHeightY - 1), mAxesSize, mHeightY + (mAxesSize - 1),
            mAxesColor);

    if (mFlags & CHART_Y_LABEL_USED) {
        /*
         * Draw indicator and label numbers
         */
        uint16_t tOffset;
        if (!(mFlags & CHART_HAS_GRID)) {
            /*
             * Here we have no grid, so we do need the small marks for label
             * Draw indicators with the same size the axis has
             */
            for (tOffset = 0; tOffset <= mHeightY; tOffset += mYGridOrLabelPixelSpacing) {
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
// clear label space before. (tTextHeight / 2) - getTextDecend(tTextHeight) because we do not have text decent here, only ascend
        DisplayForChart.fillRect(tYNumberXStart, mPositionY - (mHeightY - 1), tPositionX - mAxesSize,
                mPositionY + (tTextHeight / 2) - getTextDecend(tTextHeight), mBackgroundColor);

// convert to string
// initialize both variables to avoid compiler warnings
        float tValueFloat = mYLabelStartValue;
        /*
         * draw loop
         */
        uint16_t tYOffsetForLabel = 0;
        do {
#if defined(__AVR__)
            dtostrf(tValueFloat, mYMinStringWidth, mYNumVarsAfterDecimal, tLabelStringBuffer);
#else
            snprintf(tLabelStringBuffer, sizeof(tLabelStringBuffer), "%*.*f", mYMinStringWidth, mYNumVarsAfterDecimal, tValueFloat);
#endif
            tValueFloat += mYLabelIncrementValue;
            DisplayForChart.drawText(tYNumberXStart, mPositionY - tYOffsetForLabel - (tTextHeight / 2), tLabelStringBuffer,
                    mLabelTextSize, mYLabelColor, mBackgroundColor);
            tYOffsetForLabel += mYGridOrLabelPixelSpacing;
        } while (tYOffsetForLabel <= mHeightY);
    }
}

/**
 * increments or decrements the start value by value (one grid line)
 * and redraws Axis
 * does not decrement below 0
 */
float Chart::stepYLabelStartValue(const int aSteps) {
    mYLabelStartValue += mYLabelIncrementValue * aSteps;
    if (mYLabelStartValue < 0) {
        mYLabelStartValue = 0;
    }
    drawYAxisAndLabels();
    return mYLabelStartValue;
}

/**
 * Clears chart area and redraws axes lines
 */
void Chart::clear(void) {
    /*
     * clear graph area plus 1 pixel outside, but do not need to clear axes lines
     */
    DisplayForChart.fillRectRel(mPositionX + 1, mPositionY - mHeightY, mWidthX, mHeightY, mBackgroundColor);
// draw X line
    DisplayForChart.fillRectRel(mPositionX - (mAxesSize - 1), mPositionY, mWidthX + (mAxesSize - 1), mAxesSize, mAxesColor);
//draw y line
    DisplayForChart.fillRectRel(mPositionX - (mAxesSize - 1), mPositionY - (mHeightY - 1), mAxesSize, mHeightY + (mAxesSize - 1),
            mAxesColor);
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

// mYGridOrLabelPixelSpacing / mYLabelIncrementValue.LongValue is factor float -> pixel e.g. 40 pixel for 200 value
    tYDisplayFactor = (mYDataFactor * mYGridOrLabelPixelSpacing) / mYLabelIncrementValue;
    tYOffset = mYLabelStartValue / mYDataFactor;

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

// check for data pointer still in data buffer area
        if (aDataPointer >= tDataEndPointer) {
            break;
        }
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
     * mYGridOrLabelPixelSpacing / mYLabelIncrementValue.LongValue is factor input -> pixel e.g. 40 pixel for 200 value
     */
    tYDisplayFactor = (mYDataFactor * mYGridOrLabelPixelSpacing) / mYLabelIncrementValue;
    tYDisplayOffset = mYLabelStartValue / mYDataFactor;

    uint16_t tXpos = mPositionX;
    bool tFirstValue = true; // saves 22 bytes compared to if i == 0 :-)

    int tXScaleCounter = mXDataScaleFactor;
    if (mXDataScaleFactor < CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
        tXScaleCounter = -mXDataScaleFactor;
    }

    int16_t *tDataEndPointer = aDataPointer + aLengthOfValidData;

#if defined(LOCAL_DEBUG)
    Serial.println();
    Serial.print(F("aLengthOfValidData="));
    Serial.print(aLengthOfValidData);
    Serial.print(F(" aDataPointer="));
    Serial.print((uint16_t) aDataPointer);
    Serial.print(F(" tDataEndPointer="));
    Serial.print((uint16_t) tDataEndPointer);
    Serial.print(F(" mXDataScaleFactor="));
    Serial.println(mXDataScaleFactor);
#endif

    int tDisplayValue;
    int tLastPixelValue = 0; // used only in line mode
    for (uint16_t i = 0; i < mWidthX; i++) {
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

        int tDisplayPixelValue = tYDisplayFactor * (tDisplayValue - tYDisplayOffset);

// clip to bottom line
        if (tDisplayPixelValue < 0) {
            tDisplayPixelValue = 0;
        }
// clip to top value
        if (tDisplayPixelValue > (int) mHeightY - 1) {
            tDisplayPixelValue = mHeightY - 1;
        }
// draw first line value as pixel only
        if (aMode == CHART_MODE_PIXEL || (tFirstValue && aMode == CHART_MODE_LINE)) {
            tFirstValue = false;
            DisplayForChart.drawPixel(tXpos, mPositionY - tDisplayPixelValue, mDataColor);
        } else if (aMode == CHART_MODE_LINE) {
            DisplayForChart.drawLineFastOneX(tXpos - 1, mPositionY - tLastPixelValue, mPositionY - tDisplayPixelValue, mDataColor);
        } else if (aMode == CHART_MODE_AREA) {
            DisplayForChart.fillRectRel(tXpos, mPositionY - tDisplayPixelValue, 1, tDisplayPixelValue, mDataColor);
        }
        tLastPixelValue = tDisplayPixelValue;
        tXpos++;

// check for data pointer still in data buffer
        if (aDataPointer >= tDataEndPointer) {
#if defined(LOCAL_DEBUG)
            Serial.println();
            Serial.print(F("DataPointer="));
            Serial.print((uint16_t) aDataPointer);
            Serial.print(F(" >= DataEndPointer="));
            Serial.print((uint16_t) tDataEndPointer);
            Serial.println(F(" Data buffer is exhausted"));
#endif
            break;
        }
    }
}

/*
 * Draw 8 bit unsigned (Y compressed) data with Y offset, i.e. 8 bit value 0 is on X axis independent of mYLabelStartValue.
 * Uses the BDFunction drawChartByteBufferScaled() which sends the buffer to the host, where it is scaled and rendered
 * Data is uncompressed on the display with mYDataFactor to get chart value and then with the factor from chart value to chart pixel
 * X compression or expansion is done at the host.
 * E.g.  we send 256 data values factor 2 compressed for display of 256 values displayed at the host screen
 * each at 1/2 X increment. Thus we have an increased resolution here :-).
 * @param aMode - One of CHART_MODE_PIXEL, CHART_MODE_LINE or CHART_MODE_AREA
 */
void Chart::drawChartDataWithYOffset(uint8_t *aDataPointer, const uint16_t aLengthOfValidData, const uint8_t aMode) {

// Factor for Input -> Display value
    float tYDisplayFactor;
    /*
     * Compute display factor and offset, so that pixel matches the y scale
     * mYGridOrLabelPixelSpacing / mYLabelIncrementValue.LongValue is factor from chart value to chart pixel
     * e.g. 40/200 for 40 pixel for value 200
     */
    tYDisplayFactor = (mYDataFactor * mYGridOrLabelPixelSpacing) / mYLabelIncrementValue;

    /*
     * Draw to chart index 0 and do not clear last drawn chart line
     * -tYDisplayFactor, because chart origin is at upper left and therefore Y values must be subtracted from Y position
     * If we have scale factor -2 for compression we require 2 times as much data
     * If we have scale factor 2 for expansion we require half as much data.
     *
     * Keep in mind that 4 data values give 3 lines from 0 to 3
     * and expanded with factor 2 we have 2 data values giving one line from 0 to 2
     * and compressed with factor 2 we have 8 data values resulting in 7 lines from 0 to 3.5
     */
    uint16_t tMaximumRequiredData = reduceLongWithIntegerScaleFactor(mWidthX, mXDataScaleFactor);
    auto tLengthOfRequiredData = aLengthOfValidData;
    if (aLengthOfValidData > tMaximumRequiredData) {
        tLengthOfRequiredData = tMaximumRequiredData;
    }
    DisplayForChart.drawChartByteBufferScaled(mPositionX, mPositionY, mXDataScaleFactor, -tYDisplayFactor, mAxesSize, aMode,
            mDataColor, COLOR16_NO_DELETE, 0, true, aDataPointer, tLengthOfRequiredData);

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
 * Uses the BDFunction drawLineFastOneX() after X scaling the  data points.
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
#if defined(LOCAL_DEBUG)
        Serial.println();
        Serial.print(F("Length of data="));
        Serial.print(aLengthOfValidData);
        Serial.print(F(" is greater than chart width="));
        Serial.println(mWidthX);
#endif
        tDataLength = mWidthX;
        tRetValue = false;
    }

    uint8_t tLastValue = *aDataPointer; // tLastValue is used only in line mode
    uint16_t tXPixelPosition = mPositionX;

    for (; tDataLength > 0; tDataLength--) {
        tValue = *aDataPointer;
        if (tValue > mHeightY - 1) {
            tValue = mHeightY - 1;
            tRetValue = false;
        }
        if (aMode == CHART_MODE_PIXEL) {
            DisplayForChart.drawPixel(tXPixelPosition, mPositionY - tValue, mDataColor); // 0 is on the X axis
        } else if (aMode == CHART_MODE_LINE) {
//          Should we use drawChartByteBuffer() instead?
            DisplayForChart.drawLineFastOneX(tXPixelPosition, mPositionY - tLastValue, mPositionY - tValue, mDataColor);
//          drawLine(tXpos, mPositionY - tLastValue, tXpos + 1, mPositionY - tValue,
//                  aDataColor);
            tLastValue = tValue;
        } else {
            // aMode == CHART_MODE_AREA
            if (tValue > 0) { // no bar for Zero value, because we do not draw over X axis
                DisplayForChart.fillRectRel(tXPixelPosition, mPositionY - tValue, 1, tValue, mDataColor);
            }
        }
        aDataPointer++;
        tXPixelPosition++;
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

/*
 * Draw label at every aXLabelDistance grid lines
 */
void Chart::setXLabelDistance(uint8_t aXLabelDistance) {
    mXLabelDistance = aXLabelDistance;
}

/*
 * Draw big label at every aXLabelDistance grid lines
 * If mXLabelDistance == mXBigLabelDistance no regular label is drawn
 */
void Chart::setXBigLabelDistance(uint8_t aXBigLabelDistance) {
    mXBigLabelDistance = aXBigLabelDistance;
}

void Chart::setXRegularAndBigLabelDistance(uint8_t aXLabelDistance) {
    mXBigLabelDistance = mXLabelDistance = aXLabelDistance;
}

void Chart::setXGridOrLabelPixelSpacing(uint8_t aXGridOrLabelPixelSpacing) {
    mXGridOrLabelPixelSpacing = aXGridOrLabelPixelSpacing;
}

void Chart::setYGridOrLabelPixelSpacing(uint8_t aYGridOrLabelPixelSpacing) {
    mYGridOrLabelPixelSpacing = aYGridOrLabelPixelSpacing;
}

void Chart::setGridOrLabelPixelSpacing(uint8_t aXGridOrLabelPixelSpacing, uint8_t aYGridOrLabelPixelSpacing) {
    mXGridOrLabelPixelSpacing = aXGridOrLabelPixelSpacing;
    mYGridOrLabelPixelSpacing = aYGridOrLabelPixelSpacing;
}

uint8_t Chart::getXGridOrLabelPixelSpacing(void) const {
    return mXGridOrLabelPixelSpacing;
}

uint8_t Chart::getYGridOrLabelPixelSpacing(void) const {
    return mYGridOrLabelPixelSpacing;
}

/*
 * Set left offset of first label
 */
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
void Chart::setXLabelStartValue(float xLabelStartValueFloat) {
    mXLabelStartValue.FloatValue = xLabelStartValueFloat;
}

void Chart::setYLabelStartValue(float yLabelStartValueFloat) {
    mYLabelStartValue = yLabelStartValueFloat;
}

void Chart::setYDataFactor(float aYDataFactor) {
    mYDataFactor = aYDataFactor;
}

void Chart::setXLabelBaseIncrementValue(float xLabelBaseIncrementValueFloat) {
    mXLabelBaseIncrementValue = xLabelBaseIncrementValueFloat;
}

void Chart::setYLabelBaseIncrementValue(float yLabelBaseIncrementValueFloat) {
    mYLabelIncrementValue = yLabelBaseIncrementValueFloat;
}

time_float_union Chart::getXLabelStartValue(void) const {
    return mXLabelStartValue;
}

float Chart::getYLabelStartValue(void) const {
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

void Chart::setXTitleTextAndSize(const char *aTitleText, const uint8_t aTitleTextSize) {
    mXTitleText = aTitleText;
    mTitleTextSize = aTitleTextSize;
}

void Chart::setYTitleTextAndSize(const char *aTitleText, const uint8_t aTitleTextSize) {
    mYTitleText = aTitleText;
    mTitleTextSize = aTitleTextSize;
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
    long tRetValue;
    if (aIntegerScaleFactor >= CHART_X_AXIS_SCALE_FACTOR_EXPANSION_2) {
        tRetValue = aValue / aIntegerScaleFactor; // scale factor is >= 2 | expansion -> reduce value
    } else if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5) {
        tRetValue = (aValue * 2) / 3; // value * 2/3
    } else if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
        tRetValue = (aValue * 3) / 2; // value * 3/2
    } else {
        tRetValue = aValue * -aIntegerScaleFactor; // scale factor is <= -2 | compression -> enlarge value
    }
    return tRetValue;
}

/**
 * multiplies value with aIntegerScaleFactor if aIntegerScaleFactor is < -1 or divide if aIntegerScaleFactor is > 1
 */
float Chart::reduceFloatWithIntegerScaleFactor(float aValue, int aIntegerScaleFactor) {

    if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_1) {
        return aValue;
    }
    float tRetValue;
    if (aIntegerScaleFactor >= CHART_X_AXIS_SCALE_FACTOR_EXPANSION_2) {
        tRetValue = aValue / aIntegerScaleFactor; // scale factor is >= 2 | expansion -> reduce value
    } else if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5) {
        tRetValue = aValue * 0.666666666; // value * 2/3
    } else if (aIntegerScaleFactor == CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5) {
        tRetValue = aValue * 1.5; // value * 1.5
    } else {
        tRetValue = aValue * -aIntegerScaleFactor; // scale factor is <= -2 | compression -> enlarge value
    }
    return tRetValue;
}

/*
 * Compute X Data and Label scale factor such, that data fills X axis
 */
int16_t Chart::computeXLabelAndXDataScaleFactor(uint16_t aDataLength) {
    int16_t tNewScale;
    if (aDataLength > mWidthX) {
        /*
         * Data is is longer than available chart length -> Compress data -> negative scale values
         */
        tNewScale = -(aDataLength / mWidthX);
        if (tNewScale == -1) {
            if (aDataLength >= (mWidthX + (mWidthX / 2))) {
                tNewScale = CHART_X_AXIS_SCALE_FACTOR_1;
            } // else tNewScale = CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5 (which is -1) :-)
        }
    } else {
        /*
         * Data is is shorter than available chart length -> Expand data -> positive scale values
         */
        tNewScale = mWidthX / aDataLength;
        if (tNewScale == 1) {
            if ((aDataLength + (aDataLength / 2)) >= mWidthX) {
                tNewScale = CHART_X_AXIS_SCALE_FACTOR_1;
            } // else tNewScale = CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5 (which is 1) :-)
        }
    }
    return tNewScale;
}

void Chart::computeAndSetXLabelAndXDataScaleFactor(uint16_t aDataLength, int8_t aMaxScaleFactor) {
    int16_t tXScaleFactor = computeXLabelAndXDataScaleFactor(aDataLength);
    if (tXScaleFactor > aMaxScaleFactor) {
        tXScaleFactor = aMaxScaleFactor;
    }
    setXLabelAndXDataScaleFactor(tXScaleFactor);
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

/*
 * Show charts features
 */
#define DEMO_CHART_1_LENGTH 120
#define DEMO_CHART_2_LENGTH 140
#define DEMO_CHART_3_LENGTH 180
#if !defined(DISPLAY_HEIGHT)
#define DISPLAY_HEIGHT_IS_DEFINED_LOCALLY
#define DISPLAY_HEIGHT 240
#endif
void showChartDemo(void) {
    static bool sChartHasNoGrid = false; // show markers instead of grid every 2. call
    Chart ChartExample;

    /*
     * allocate memory for 180 int16_t values
     */
    int16_t *tChartBufferPtr = (int16_t*) malloc(sizeof(int16_t) * DEMO_CHART_3_LENGTH);
    if (tChartBufferPtr == nullptr) {
#if !defined(ARDUINO)
        failParamMessage(sizeof(int16_t) * DEMO_CHART_3_LENGTH, "malloc failed");
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
    ChartExample.initChartColors(COLOR16_BLUE, COLOR16_RED, CHART_DEFAULT_GRID_COLOR, COLOR16_RED, COLOR16_RED, COLOR16_WHITE);
    ChartExample.initChart(5, DISPLAY_HEIGHT - 20, DEMO_CHART_1_LENGTH, 90, 2, TEXT_SIZE_11, !sChartHasNoGrid, 0, 0);
    ChartExample.setGridOrLabelPixelSpacing(20, 20);
    ChartExample.drawAxesAndGrid();

    char *tRandomByteFillPointer = (char*) tChartBufferPtr; // here we interpret it as byte array
    *tRandomByteFillPointer++ = 0;
    *tRandomByteFillPointer++ = 200; // chart height - 1 is maximum but this tests clipping
//generate random data
#if defined(ARDUINO)
    for (unsigned int i = 2; i < (DEMO_CHART_1_LENGTH - 2); i++) {
        *tRandomByteFillPointer++ = 30 + random(31);
    }
#else
    srand(120);
    for (unsigned int i = 2; i < (DEMO_CHART_1_LENGTH - 2); i++) {
        *tRandomByteFillPointer++ = 30 + (rand() >> 27); // +/-16
    }
#endif
    *tRandomByteFillPointer++ = 1; // for testing
    *tRandomByteFillPointer++ = 0; // Last element is 0 for testing
    ChartExample.drawChartDataDirect((uint8_t*) tChartBufferPtr, DEMO_CHART_1_LENGTH, CHART_MODE_PIXEL);

    delay(1000);
    /*
     * 2. Chart: 140 16-bit values, with grid, with (negative) integer Y labels
     * and X label rendered as minutes time with offset 5 and 2 labels spacing
     */
// new random data
    int16_t *tRandomShortFillPointer = tChartBufferPtr;
    int16_t tDataValue = -15;
    *tRandomShortFillPointer++ = 0; // for testing zero value with grid
    *tRandomShortFillPointer++ = -20; // for testing value with X axis
    *tRandomShortFillPointer++ = -20; // for testing value with X axis

    for (unsigned int i = 3; i < DEMO_CHART_2_LENGTH; i++) {
#if defined(ARDUINO)
        tDataValue += random(-3, 6);
#else
        tDataValue += rand() >> 29;
#endif
        *tRandomShortFillPointer++ = tDataValue;
    }

    ChartExample.initXLabelTimestampForLabelScaleIdentity(0, 10, 2);
    ChartExample.setLabelStringFunction(ChartExample.convertMinutesToString);

    if (sChartHasNoGrid) {
// increment X label scale factor for every second call
        ChartExample.setXLabelAndXDataScaleFactor(CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_2); // compression, label values increment is doubled
    }
    ChartExample.setXLabelAndGridOffset(5); // Left offset of first label: here "0"
    ChartExample.setXLabelDistance(2);
    ChartExample.setXTitleTextAndSize("Time", TEXT_SIZE_11);

    ChartExample.initYLabel(-20, 20, 20 / 15, 3, 0);
    ChartExample.initChartColors(COLOR16_GREEN, COLOR16_RED, CHART_DEFAULT_GRID_COLOR, COLOR16_YELLOW, COLOR16_GREEN,
    COLOR16_WHITE);
    ChartExample.initChart(170, DISPLAY_HEIGHT - 20, DEMO_CHART_2_LENGTH, 88, 2, TEXT_SIZE_11, !sChartHasNoGrid, 15, 15);
    ChartExample.setYTitleText("Count");

    ChartExample.drawAxesAndGrid();
    ChartExample.drawXAxisTitle();
    ChartExample.drawYAxisTitle(-TEXT_SIZE_11); // draw Y title above grid

    ChartExample.drawChartData(tChartBufferPtr, DEMO_CHART_2_LENGTH, CHART_MODE_LINE);

    /*
     * 3. Chart: 140 16-bit values, without grid, with float labels, area mode
     */
// new random data
    tRandomShortFillPointer = tChartBufferPtr;
    tDataValue = 0;

    for (unsigned int i = 0; i < DEMO_CHART_3_LENGTH; i++) {
#if defined(ARDUINO)
        tDataValue += random(-2, 4);
#else
        tDataValue += rand() >> 30;
#endif
        *tRandomShortFillPointer++ = tDataValue;
    }
    tChartBufferPtr[3] = 0; // 4. element is 0 for testing

    ChartExample.initXLabel(0, 0.5, CHART_X_AXIS_SCALE_FACTOR_1, 3, 1);
    ChartExample.setLabelStringFunction(nullptr);

    if (!sChartHasNoGrid) {
// increment X label scale factor for every second call
        ChartExample.setXLabelAndXDataScaleFactor(CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_2); // compression, label values increment is doubled
    }
    ChartExample.initYLabel(0, 0.3, 1.3 / 60, 3, 1); // display 1.3 for raw value of 60
    ChartExample.initChartColors(COLOR16_RED, COLOR16_BLUE, COLOR16_GREEN, COLOR16_BLACK, COLOR16_BLACK, COLOR16_WHITE);
    ChartExample.initChart(30, 100, DEMO_CHART_3_LENGTH, 90, 2, TEXT_SIZE_11, sChartHasNoGrid, 30, 16);
    ChartExample.drawAxesAndGrid();
    ChartExample.drawChartData(tChartBufferPtr, DEMO_CHART_3_LENGTH, CHART_MODE_AREA);

    free(tChartBufferPtr);
    sChartHasNoGrid = !sChartHasNoGrid; // switch grid display for next run, to see the markers, which are drawn instead of grid
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
