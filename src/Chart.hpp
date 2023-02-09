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
 *  Copyright (C) 2012-2023  Armin Joachimsmeyer
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
    mChartBackgroundColor = CHART_DEFAULT_BACKGROUND_COLOR;
    mAxesColor = CHART_DEFAULT_AXES_COLOR;
    mGridColor = CHART_DEFAULT_GRID_COLOR;
    mLabelColor = CHART_DEFAULT_LABEL_COLOR;
    mFlags = 0;
    mXScaleFactor = 0;
    mXTitleText = NULL;
    mYTitleText = NULL;
}

void Chart::initChartColors(const uint16_t aDataColor, const uint16_t aAxesColor, const uint16_t aGridColor,
        const uint16_t aLabelColor, const uint16_t aBackgroundColor) {
    mDataColor = aDataColor;
    mAxesColor = aAxesColor;
    mGridColor = aGridColor;
    mLabelColor = aLabelColor;
    mChartBackgroundColor = aBackgroundColor;
}

void Chart::setDataColor(uint16_t aDataColor) {
    mDataColor = aDataColor;
}

/**
 * aPositionX and aPositionY are the 0 coordinates of the grid and part of the axes
 */
uint8_t Chart::initChart(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX, const uint16_t aHeightY,
        const uint8_t aAxesSize, const bool aHasGrid, const uint8_t aGridXResolution, const uint8_t aGridYResolution) {
    mPositionX = aPositionX;
    mPositionY = aPositionY;
    mWidthX = aWidthX;
    mHeightY = aHeightY;
    mAxesSize = aAxesSize;
    if (aHasGrid) {
        mFlags |= CHART_HAS_GRID;
    } else {
        mFlags &= ~CHART_HAS_GRID;
    }
    mGridXSpacing = aGridXResolution;
    mGridYSpacing = aGridYResolution;

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

    if (mGridXSpacing > mWidthX) {
        mGridXSpacing = mWidthX / 2;
        tRetValue = CHART_ERROR_GRID_X_RESOLUTION;
    }
    return tRetValue;

}
/**
 *
 * @param aXLabelStartValue
 * @param aXLabelIncrementValue (real value at grid is depends on scale factor
 * @param aXLabelScaleFactor
 * @param aXMinStringWidth
 */
void Chart::initXLabelInt(const int aXLabelStartValue, const int aXLabelIncrementValue, const uint8_t aXLabelScaleFactor,
        const uint8_t aXMinStringWidth) {
    mXLabelStartValue.IntValue = aXLabelStartValue;
    mXLabelBaseIncrementValue.IntValue = aXLabelIncrementValue;
    mXScaleFactor = aXLabelScaleFactor;
    mXMinStringWidth = aXMinStringWidth;
    mFlags |= CHART_X_LABEL_INT | CHART_X_LABEL_USED;
}

/**
 *
 * @param aGridXSpacing
 * @param aXLabelIncrementValue
 * @param aXMinStringWidth
 */
void Chart::iniXAxisInt(const uint8_t aGridXSpacing, const int aXLabelStartValue, const int aXLabelIncrementValue,
        const uint8_t aXMinStringWidth) {
    mGridXSpacing = aGridXSpacing;
    mXLabelStartValue.IntValue = aXLabelStartValue;
    mXLabelBaseIncrementValue.IntValue = aXLabelIncrementValue;
    mXMinStringWidth = aXMinStringWidth;
    mFlags |= CHART_X_LABEL_INT;
    if (mXMinStringWidth != 0) {
        mFlags |= CHART_X_LABEL_USED;
    }
}

/**
 *
 * @param aXLabelStartValue
 * @param aXLabelIncrementValue
 * @param aXScaleFactor
 * @param aXMinStringWidthIncDecimalPoint
 * @param aXNumVarsAfterDecimal
 */
void Chart::initXLabelFloat(const float aXLabelStartValue, const float aXLabelIncrementValue, const int8_t aXScaleFactor,
        uint8_t aXMinStringWidthIncDecimalPoint, uint8_t aXNumVarsAfterDecimal) {
    mXLabelStartValue.FloatValue = aXLabelStartValue;
    mXLabelBaseIncrementValue.FloatValue = aXLabelIncrementValue;
    mXScaleFactor = aXScaleFactor;
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
 * @param aYFactor factor for input to chart value - e.g. (3.0 / 4096) for adc reading of 4096 for 3 Volt
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
    mYNumVarsAfterDecimal = aYNumVarsAfterDecimal;
    mYMinStringWidth = aYMinStringWidthIncDecimalPoint;
    mYDataFactor = aYFactor;
    mFlags &= ~CHART_Y_LABEL_INT;
    mFlags |= CHART_Y_LABEL_USED;
}

/**
 * Render the chart on the lcd
 */
void Chart::drawAxesAndGrid(void) {
    drawAxes(false);
    drawGrid();
}

void Chart::drawGrid(void) {
    if (!(mFlags & CHART_HAS_GRID)) {
        return;
    }
    uint16_t tOffset;
// draw vertical lines
    for (tOffset = mGridXSpacing; tOffset <= mWidthX; tOffset += mGridXSpacing) {
        DisplayForChart.drawLineRel(mPositionX + tOffset, mPositionY - (mHeightY - 1), 0, mHeightY - 1, mGridColor);
    }
// draw horizontal lines
    for (tOffset = mGridYSpacing; tOffset <= mHeightY; tOffset += mGridYSpacing) {
        DisplayForChart.drawLineRel(mPositionX + 1, mPositionY - tOffset, mWidthX - 1, 0, mGridColor);
    }

}

/**
 * render axes
 * renders indicators if labels but no grid are specified
 * render labels only if at least one increment value != 0
 * @retval -11 in no space for X labels or - 12 if no space for Y labels on lcd
 */
void Chart::drawAxes(bool aClearLabelsBefore) {
    drawXAxis(aClearLabelsBefore);
    drawYAxis(aClearLabelsBefore);
}

/**
 * draw x title
 */
void Chart::drawXAxisTitle(void) const {
    if (mXTitleText != NULL) {
        /**
         * draw axis title
         */
        uint16_t tTextLenPixel = strlen(mXTitleText) * TEXT_SIZE_11_WIDTH;
        DisplayForChart.drawText(mPositionX + mWidthX - tTextLenPixel - 1, mPositionY - TEXT_SIZE_11_DECEND, mXTitleText,
        TEXT_SIZE_11, mLabelColor, mChartBackgroundColor);
    }
}

int Chart::adjustIntWithXScaleFactor(int aValue) {
    return adjustIntWithScaleFactor(aValue, mXScaleFactor);
}

float Chart::adjustFloatWithXScaleFactor(float aValue) {
    return adjustFloatWithScaleFactor(aValue, mXScaleFactor);
}

/**
 * draw x line with indicators and labels
 */
void Chart::drawXAxis(bool aClearLabelsBefore) {

    char tLabelStringBuffer[32];

// draw X line
    DisplayForChart.fillRectRel(mPositionX - (mAxesSize - 1), mPositionY, mWidthX + (mAxesSize - 1), mAxesSize, mAxesColor);
    if (mFlags & CHART_X_LABEL_USED) {
        if (!(mFlags & CHART_HAS_GRID)) {
            /*
             * draw indicators with the same size the axis has
             */
            for (uint16_t tGridOffset = 0; tGridOffset <= mWidthX; tGridOffset += mGridXSpacing) {
                DisplayForChart.fillRectRel(mPositionX + tGridOffset, mPositionY + mAxesSize, 1, mAxesSize, mGridColor);
            }
        }

        /*
         * draw labels (numbers)
         */
        uint16_t tNumberYTop = mPositionY + 2 * mAxesSize;
#if !defined(ARDUINO)
        assertParamMessage((tNumberYTop <= (DisplayForChart.getDisplayHeight() - TEXT_SIZE_11_DECEND)), tNumberYTop,
                "no space for x labels");
#endif

        // first offset is negative
        int16_t tOffset = 1 - ((TEXT_SIZE_11_WIDTH * mXMinStringWidth) / 2);
        if (aClearLabelsBefore) {
            // clear label space before
            DisplayForChart.fillRect(mPositionX + tOffset, tNumberYTop, mPositionX + mWidthX, tNumberYTop + TEXT_SIZE_11_HEIGHT,
                    mChartBackgroundColor);
        }

        // initialize both variables to avoid compiler warnings
        int tValue = mXLabelStartValue.IntValue;
        int tIncrementValue = adjustIntWithXScaleFactor(mXLabelBaseIncrementValue.IntValue);
        float tValueFloat = mXLabelStartValue.FloatValue;
        float tIncrementValueFloat = adjustFloatWithXScaleFactor(mXLabelBaseIncrementValue.FloatValue);

        /*
         * draw loop for labels
         */
        do {
            if (mFlags & CHART_X_LABEL_INT) {
                snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%d", tValue);
                tValue += tIncrementValue;
            } else {
#if defined(AVR)
                dtostrf(tValueFloat, mXMinStringWidth, mXNumVarsAfterDecimal, tLabelStringBuffer);

#else
                snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%*.*f", mXMinStringWidth, mXNumVarsAfterDecimal,
                        tValueFloat);
#endif
                tValueFloat += tIncrementValueFloat;
            }
            DisplayForChart.drawText(mPositionX + tOffset, tNumberYTop + TEXT_SIZE_11_ASCEND, tLabelStringBuffer,
            TEXT_SIZE_11, mLabelColor, mChartBackgroundColor);
            tOffset += mGridXSpacing;
        } while (tOffset <= (int16_t) mWidthX);
    }
}

/**
 * Set x label start to index.th value - start not with first but with startIndex label
 * redraw Axis
 */
void Chart::setXLabelIntStartValueByIndex(const int aNewXStartIndex, const bool doDraw) {
    mXLabelStartValue.IntValue = mXLabelBaseIncrementValue.IntValue * aNewXStartIndex;
    if (doDraw) {
        drawXAxis(true);
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
        mXLabelStartValue.IntValue += mXLabelBaseIncrementValue.IntValue;
        if (mXLabelStartValue.IntValue > aMaxValue) {
            mXLabelStartValue.IntValue = aMaxValue;
            tRetval = false;
        }
    } else {
        mXLabelStartValue.IntValue -= mXLabelBaseIncrementValue.IntValue;
        if (mXLabelStartValue.IntValue < aMinValue) {
            mXLabelStartValue.IntValue = aMinValue;
            tRetval = false;
        }
    }
    drawXAxis(true);
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
    drawXAxis(true);
    return mXLabelStartValue.FloatValue;
}

/**
 * draw y title
 */
void Chart::drawYAxisTitle(const int aYOffset) const {
    if (mYTitleText != NULL) {
        /**
         * draw axis title - use data color
         */
        DisplayForChart.drawText(mPositionX + mAxesSize + 1, mPositionY - mHeightY + aYOffset + TEXT_SIZE_11_ASCEND, mYTitleText,
        TEXT_SIZE_11, mDataColor, mChartBackgroundColor);
    }
}

/**
 * draw y line with indicators and labels
 */
void Chart::drawYAxis(const bool aClearLabelsBefore) {

    char tLabelStringBuffer[32];

    DisplayForChart.fillRectRel(mPositionX - (mAxesSize - 1), mPositionY - (mHeightY - 1), mAxesSize, (mHeightY - 1), mAxesColor);

    if (mFlags & CHART_Y_LABEL_USED) {
        uint16_t tOffset;
        if (!(mFlags & CHART_HAS_GRID)) {
            /*
             * draw indicators with the same size the axis has
             */
            for (tOffset = 0; tOffset <= mHeightY; tOffset += mGridYSpacing) {
                DisplayForChart.fillRectRel(mPositionX - (2 * mAxesSize) + 1, mPositionY - tOffset, mAxesSize, 1, mGridColor);
            }
        }

        /*
         * draw labels (numbers)
         */
        int16_t tNumberXLeft = mPositionX - 2 * mAxesSize - 1 - (mYMinStringWidth * TEXT_SIZE_11_WIDTH);
#if !defined(ARDUINO)
        assertParamMessage((tNumberXLeft >= 0), tNumberXLeft, "no space for y labels");
#endif

        // first offset is half of character height
        tOffset = TEXT_SIZE_11_HEIGHT / 2;
        if (aClearLabelsBefore) {
            // clear label space before
            DisplayForChart.fillRect(tNumberXLeft, mPositionY - mHeightY + 1, mPositionX - mAxesSize,
                    mPositionY - tOffset + TEXT_SIZE_11_HEIGHT + 1, mChartBackgroundColor);
        }

        // convert to string
        // initialize both variables to avoid compiler warnings
        long tValue = mYLabelStartValue.IntValue;
        float tValueFloat = mYLabelStartValue.FloatValue;
        /*
         * draw loop
         */
        do {
            if (mFlags & CHART_Y_LABEL_INT) {
                snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%ld", tValue);
                tValue += mYLabelIncrementValue.IntValue;
            } else {
#if defined(AVR)
                dtostrf(tValueFloat, mXMinStringWidth, mXNumVarsAfterDecimal, tLabelStringBuffer);

#else
                snprintf(tLabelStringBuffer, sizeof tLabelStringBuffer, "%*.*f", mYMinStringWidth, mYNumVarsAfterDecimal,
                        tValueFloat);
#endif
                tValueFloat += mYLabelIncrementValue.FloatValue;
            }
            DisplayForChart.drawText(tNumberXLeft, mPositionY - tOffset + TEXT_SIZE_11_ASCEND, tLabelStringBuffer,
            TEXT_SIZE_11, mLabelColor, mChartBackgroundColor);
            tOffset += mGridYSpacing;
        } while (tOffset <= (mHeightY + TEXT_SIZE_11_HEIGHT / 2));
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
    drawYAxis(true);
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
    drawYAxis(false);
    return mYLabelStartValue.FloatValue;
}

/**
 * Clears chart area and redraws axes lines
 */
void Chart::clear(void) {
    DisplayForChart.fillRectRel(mPositionX + 1, mPositionY - (mHeightY - 1), mWidthX, mHeightY - 1, mChartBackgroundColor);
    // draw X line
    DisplayForChart.fillRectRel(mPositionX - (mAxesSize - 1), mPositionY, mWidthX + (mAxesSize - 1), mAxesSize, mAxesColor);
    //draw y line
    DisplayForChart.fillRectRel(mPositionX - (mAxesSize - 1), mPositionY - (mHeightY - 1), mAxesSize, mHeightY - 1, mAxesColor);
}

/**
 * Draws a chart  - Factor for float to chart value (mYFactor) is used to compute display values
 * @param aDataPointer pointer to raw data array
 * @param aDataEndPointer pointer to first element after data
 * @param aMode CHART_MODE_PIXEL, CHART_MODE_LINE or CHART_MODE_AREA
 * @return false if clipping occurs
 */
bool Chart::drawChartDataFloat(const float *aDataPointer, const float *aDataEndPointer, const uint8_t aMode) {

    bool tRetValue = true;
    float tInputValue;

// used only in line mode
    int tLastValue = 0;

    // Factor for Float -> Display value
    float tYDisplayFactor = 1;
    float tYOffset = 0;

    if (mFlags & CHART_Y_LABEL_INT) {
        // mGridYSpacing / mYLabelIncrementValue.IntValue is factor float -> pixel e.g. 40 pixel for 200 value
        tYDisplayFactor = (mYDataFactor * mGridYSpacing) / mYLabelIncrementValue.IntValue;
        tYOffset = mYLabelStartValue.IntValue / mYDataFactor;
    } else {
        tYDisplayFactor = (mYDataFactor * mGridYSpacing) / mYLabelIncrementValue.FloatValue;
        tYOffset = mYLabelStartValue.FloatValue / mYDataFactor;
    }

    uint16_t tXpos = mPositionX;
    bool tFirstValue = true;

    int tXScaleCounter = mXScaleFactor;
    if (mXScaleFactor < -1) {
        tXScaleCounter = -mXScaleFactor;
    }

    for (int i = mWidthX; i > 0; i--) {
        /*
         *  get data and perform X scaling
         */
        if (mXScaleFactor == 0) {
            tInputValue = *aDataPointer++;
        } else if (mXScaleFactor == -1) {
            // compress by factor 1.5 - every second value is the average of the next two values
            tInputValue = *aDataPointer++;
            tXScaleCounter--; // starts with 1
            if (tXScaleCounter < 0) {
                // get average of actual and next value
                tInputValue += *aDataPointer++;
                tInputValue /= 2;
                tXScaleCounter = 1;
            }
        } else if (mXScaleFactor <= -1) {
            // compress - get average of multiple values
            tInputValue = 0;
            for (int j = 0; j < tXScaleCounter; ++j) {
                tInputValue += *aDataPointer++;
            }
            tInputValue /= tXScaleCounter;
        } else if (mXScaleFactor == 1) {
            // expand by factor 1.5 - every second value will be shown 2 times
            tInputValue = *aDataPointer++;
            tXScaleCounter--; // starts with 1
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
                tXScaleCounter = mXScaleFactor;
            }
        }
        // check for data pointer still in data buffer area
        if (aDataPointer >= aDataEndPointer) {
            break;
        }

        uint16_t tDisplayValue = tYDisplayFactor * (tInputValue - tYOffset);
        // clip to bottom line
        if (tYOffset > tInputValue) {
            tDisplayValue = 0;
            tRetValue = false;
        }
        // clip to top value
        if (tDisplayValue > mHeightY - 1) {
            tDisplayValue = mHeightY - 1;
            tRetValue = false;
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
    return tRetValue;
}

/**
 * Draws a chart  - Factor for uint16_t values to chart value (mYFactor) is used to compute display values
 * @param aDataPointer pointer to input data array
 * @param aDataEndPointer pointer to first element after data
 * @param aMode CHART_MODE_PIXEL, CHART_MODE_LINE or CHART_MODE_AREA
 * @return false if clipping occurs
 */
bool Chart::drawChartData(const int16_t *aDataPointer, const uint16_t aDataLength, const uint8_t aMode) {
    return drawChartData(aDataPointer, aDataPointer + aDataLength, aMode);
}

bool Chart::drawChartData(const int16_t *aDataPointer, const int16_t *aDataEndPointer, const uint8_t aMode) {

    bool tRetValue = true;
    int tDisplayValue;

// used only in line mode
    int tLastValue = 0;

    // Factor for Input -> Display value
    float tYDisplayFactor = 1;
    int tYOffset = 0;

    if (mFlags & CHART_Y_LABEL_INT) {
        // mGridYSpacing / mYLabelIncrementValue.IntValue is factor input -> pixel e.g. 40 pixel for 200 value
        tYDisplayFactor = (mYDataFactor * mGridYSpacing) / mYLabelIncrementValue.IntValue;
        tYOffset = mYLabelStartValue.IntValue / mYDataFactor;
    } else {
        // Label is float
        tYDisplayFactor = (mYDataFactor * mGridYSpacing) / mYLabelIncrementValue.FloatValue;
        tYOffset = mYLabelStartValue.FloatValue / mYDataFactor;
    }

    uint16_t tXpos = mPositionX;
    bool tFirstValue = true;

    int tXScaleCounter = mXScaleFactor;
    if (mXScaleFactor < -1) {
        tXScaleCounter = -mXScaleFactor;
    }

    for (int i = mWidthX; i > 0; i--) {
        /*
         *  get data and perform X scaling
         */
        if (mXScaleFactor == 0) {
            tDisplayValue = *aDataPointer++;
        } else if (mXScaleFactor == -1) {
            // compress by factor 1.5 - every second value is the average of the next two values
            tDisplayValue = *aDataPointer++;
            tXScaleCounter--; // starts with 1
            if (tXScaleCounter < 0) {
                // get average of actual and next value
                tDisplayValue += *aDataPointer++;
                tDisplayValue /= 2;
                tXScaleCounter = 1;
            }
        } else if (mXScaleFactor <= -1) {
            // compress - get average of multiple values
            tDisplayValue = 0;
            for (int j = 0; j < tXScaleCounter; ++j) {
                tDisplayValue += *aDataPointer++;
            }
            tDisplayValue /= tXScaleCounter;
        } else if (mXScaleFactor == 1) {
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
                tXScaleCounter = mXScaleFactor;
            }
        }
        // check for data pointer still in data buffer
        if (aDataPointer >= aDataEndPointer) {
            break;
        }

        tDisplayValue = tYDisplayFactor * (tDisplayValue - tYOffset);
        // clip to bottom line
        if (tDisplayValue < 0) {
            tDisplayValue = 0;
            tRetValue = false;
        }
        // clip to top value
        if (tDisplayValue > (int) mHeightY - 1) {
            tDisplayValue = mHeightY - 1;
            tRetValue = false;
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
    return tRetValue;
}

/**
 * Draws a chart of values of the uint8_t data array pointed to by aDataPointer
 * @param aDataPointer
 * @param aDataLength
 * @param aMode
 * @return false if clipping occurs
 */
bool Chart::drawChartDataDirect(const uint8_t *aDataPointer, const uint16_t aDataLength, const uint8_t aMode) {

    bool tRetValue = true;
    uint8_t tValue;
    uint16_t tDataLength = aDataLength;
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
            DisplayForChart.drawLineFastOneX(tXpos, mPositionY - tLastValue, mPositionY - tValue, mDataColor);
//          drawLine(tXpos, mPositionY - tLastValue, tXpos + 1, mPositionY - tValue,
//                  aDataColor);
            tXpos++;
            tLastValue = tValue;
        } else if (aMode == CHART_MODE_AREA) {
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

void Chart::setXGridSpacing(uint8_t aXGridSpacing) {
    mGridXSpacing = aXGridSpacing;
}

void Chart::setYGridSpacing(uint8_t aYGridSpacing) {
    mGridYSpacing = aYGridSpacing;
}

uint8_t Chart::getXGridSpacing(void) const {
    return mGridXSpacing;
}

uint8_t Chart::getYGridSpacing(void) const {
    return mGridYSpacing;
}

void Chart::setXScaleFactor(int aXScaleFactor, const bool doDraw) {
    mXScaleFactor = aXScaleFactor;
    if (doDraw) {
        drawXAxis(true);
    }
}

int Chart::getXScaleFactor(void) const {
    return mXScaleFactor;
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
    return ((mYLabelStartValue.FloatValue + mYLabelIncrementValue.FloatValue * (mHeightY / mGridYSpacing)) / mYDataFactor);
}

void Chart::setXLabelBaseIncrementValue(int xLabelBaseIncrementValue) {
    mXLabelBaseIncrementValue.IntValue = xLabelBaseIncrementValue;
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

int_float_union Chart::getXLabelStartValue(void) const {
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

void Chart::setXTitleText(const char *aTitleText) {
    mXTitleText = aTitleText;
}

void Chart::setYTitleText(const char *aTitleText) {
    mYTitleText = aTitleText;
}

/**
 * aScaleFactor > 1 : expansion by factor aScaleFactor
 * aScaleFactor == 1 : expansion by 1.5
 * aScaleFactor == 0 : identity
 * aScaleFactor == -1 : compression by 1.5
 * aScaleFactor < -1 : compression by factor -aScaleFactor
 * multiplies value with factor if aScaleFactor is < 0 (compression) or divide if aScaleFactor is > 0 (expansion)
 */
int adjustIntWithScaleFactor(int aValue, int aScaleFactor) {
    if (aScaleFactor == 0) {
        return aValue;
    }
    int tRetValue = aValue;
    if (aScaleFactor > 1) {
        tRetValue = aValue / aScaleFactor;
    } else if (aScaleFactor == 1) {
        // value * 2/3
        tRetValue = (aValue * 2) / 3;
    } else if (aScaleFactor == -1) {
        // value * 3/2
        tRetValue = (aValue * 3) / 2;
    } else if (aScaleFactor < -1) {
        tRetValue = aValue * -aScaleFactor;
    }
    return tRetValue;
}

void getScaleFactorAsString(char *tStringBuffer, int aScaleFactor) {
    if (aScaleFactor >= 0) {
        *tStringBuffer++ = '*';
        aScaleFactor = -aScaleFactor; // for adjustIntWithScaleFactor() down below scaleFactor must be negative
    } else {
        *tStringBuffer++ = '\xF7'; // division
    }
    if (aScaleFactor == -1) {
        *tStringBuffer++ = '1';
        *tStringBuffer++ = '.';
        *tStringBuffer++ = '5';
        *tStringBuffer++ = '\0';
    } else {
        snprintf(tStringBuffer, 5, "%-3d", adjustIntWithScaleFactor(1, aScaleFactor));
    }

}

/**
 * multiplies value with aScaleFactor if aScaleFactor is < -1 or divide if aScaleFactor is > 1
 */
float adjustFloatWithScaleFactor(float aValue, int aScaleFactor) {

    if (aScaleFactor == 0) {
        return aValue;
    }
    float tRetValue = aValue;
    if (aScaleFactor > 1) {
        tRetValue = aValue / aScaleFactor;
    } else if (aScaleFactor == 1) {
        // value * 2/3
        tRetValue = aValue * 0.666666666;
    } else if (aScaleFactor == -1) {
        // value * 1.5
        tRetValue = aValue * 1.5;
    } else if (aScaleFactor < -1) {
        tRetValue = aValue * -aScaleFactor;
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
        DisplayForChart.drawText(0, 0, "malloc failed", TEXT_SIZE_11, COLOR16_BLACK, COLOR16_RED);
#endif
        return;
    }

    /*
     * 1. Chart: 120 8-bit values, pixel with grid, no labels, height = 90, axes size = 2, no grid
     */
    ChartExample.disableXLabel();
    ChartExample.disableYLabel();
    ChartExample.initChartColors(COLOR16_RED, COLOR16_RED, CHART_DEFAULT_GRID_COLOR, COLOR16_RED, COLOR16_WHITE);
    ChartExample.initChart(5, DISPLAY_HEIGHT - 20, CHART_1_LENGTH, 90, 2, CHART_DISPLAY_GRID, 20, 20);
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
     * 2. Chart: 140 16-bit values, with grid, with (negative) integer labels
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

    ChartExample.initXLabelInt(0, 12, 1, 2);
    ChartExample.initYLabelInt(-20, 20, 20 / 15, 3);
    ChartExample.initChart(170, DISPLAY_HEIGHT - 20, CHART_2_LENGTH, 88, 2, CHART_DISPLAY_GRID, 30, 15);
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

    ChartExample.initXLabelFloat(0, 0.5, 1, 3, 1);
    ChartExample.initYLabelFloat(0, 0.3, 1.3 / 60, 3, 1); // display 1.3 for raw value of 60
    ChartExample.initChart(30, 100, CHART_3_LENGTH, 90, 2, CHART_DISPLAY_NO_GRID, 30, 16);
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
#endif // _CHART_HPP
