/**
 * Chart.h
 *
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

#include "BlueDisplay.h"

#ifndef _CHART_H
#define _CHART_H

// for init()
#define CHART_DISPLAY_NO_GRID           false
#define CHART_DISPLAY_GRID              true

#define CHART_DEFAULT_AXES_COLOR        COLOR16_BLACK
#define CHART_DEFAULT_GRID_COLOR        COLOR16( 180, 180, 180)
#define CHART_DEFAULT_BACKGROUND_COLOR  COLOR16_WHITE
#define CHART_DEFAULT_LABEL_COLOR       COLOR16_BLACK
#define CHART_MAX_AXES_SIZE             10

// data drawing modes
#define CHART_MODE_PIXEL                0
#define CHART_MODE_LINE                 1
#define CHART_MODE_AREA                 2

// Error codes
#define CHART_ERROR_POS_X       -1
#define CHART_ERROR_POS_Y       -2
#define CHART_ERROR_WIDTH       -4
#define CHART_ERROR_HEIGHT      -8
#define CHART_ERROR_AXES_SIZE   -16
#define CHART_ERROR_GRID_X_RESOLUTION -32

// Masks for mFlags
#define CHART_HAS_GRID      0x01
#define CHART_X_LABEL_USED  0x02
#define CHART_X_LABEL_INT   0x04 // else label is float
#define CHART_Y_LABEL_USED  0x08
#define CHART_Y_LABEL_INT   0x10 // else label is float
typedef union {
    int IntValue;
    float FloatValue;
} int_float_union;

int adjustIntWithScaleFactor(int aValue, int aScaleFactor);
float adjustFloatWithScaleFactor(float aValue, int aScaleFactor);
void getScaleFactorAsString(char * tStringBuffer, int aScaleFactor);
void showChartDemo(void);

class Chart {
public:
    Chart();
    uint8_t initChart(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX, const uint16_t aHeightY,
            const uint8_t aAxesSize, const bool aHasGrid, const uint8_t aGridXResolution, const uint8_t aGridYResolution);

    void initChartColors(const uint16_t aDataColor, const uint16_t aAxesColor, const uint16_t aGridColor,
            const uint16_t aLabelColor, const uint16_t aBackgroundColor);
    void setDataColor(uint16_t aDataColor);

    void clear(void);

    void setWidthX(const uint16_t widthX);
    void setHeightY(const uint16_t heightY);
    void setPositionX(const uint16_t positionX);
    void setPositionY(const uint16_t positionY);

    uint16_t getWidthX(void) const;
    uint16_t getHeightY(void) const;
    uint16_t getPositionX(void) const;
    uint16_t getPositionY(void) const;

    void drawAxes(const bool aClearLabelsBefore);
    void drawAxesAndGrid(void);
    bool drawChartDataDirect(const uint8_t *aDataPointer, const uint16_t aDataLength, const uint8_t aMode);
    bool drawChartData(const int16_t *aDataPointer, const uint16_t aDataLength, const uint8_t aMode);
    bool drawChartData(const int16_t *aDataPointer, const int16_t * aDataEndPointer, const uint8_t aMode);
    bool drawChartDataFloat(const float * aDataPointer, const float * aDataEndPointer, const uint8_t aMode);
    void drawGrid(void);

    /*
     * X Axis
     */
    void drawXAxis(const bool aClearLabelsBefore);

    void setXGridSpacing(uint8_t aXGridSpacing);
    void iniXAxisInt(const uint8_t aGridXSpacing, const int aXLabelStartValue, const int aXLabelIncrementValue,
            const uint8_t aXMinStringWidth);
    uint8_t getXGridSpacing(void) const;

    /*
     * X Label
     */
    // Init
    void initXLabelInt(const int aXLabelStartValue, const int aXLabelIncrementValue, const uint8_t aXLabelScaleFactor,
            const uint8_t aXMinStringWidth);
    void initXLabelFloat(const float aXLabelStartValue, const float aXLabelIncrementValue, const int8_t aXScaleFactor,
            uint8_t aXMinStringWidthIncDecimalPoint, uint8_t aXNumVarsAfterDecimal);
    void disableXLabel(void);

    // Start value
    void setXLabelStartValue(int xLabelStartValue);
    void setXLabelStartValueFloat(float xLabelStartValueFloat);
    void setXLabelIntStartValueByIndex(const int aNewXStartIndex, const bool doDraw); // does not use XScaleFactor
    int_float_union getXLabelStartValue(void) const;
    bool stepXLabelStartValueInt(const bool aDoIncrement, const int aMinValue, const int aMaxValue); // does not use XScaleFactor
    float stepXLabelStartValueFloat(const bool aDoIncrement); // does not use XScaleFactor

    // Increment value
    void setXLabelBaseIncrementValue(int xLabelIncrementValue);
    void setXLabelBaseIncrementValueFloat(float xLabelIncrementValueFloat);
    // Increment factor
    void setXScaleFactor(int aXScaleFactor, const bool doDraw);
    int getXScaleFactor(void) const;

    int adjustIntWithXScaleFactor(int Value);
    float adjustFloatWithXScaleFactor(float Value);

    // X Title
    void setXTitleText(const char * aLabelText);
    void drawXAxisTitle(void) const;

    /*
     *  Y Axis
     */
    void drawYAxis(const bool aClearLabelsBefore);

    void setYGridSpacing(uint8_t aYGridSpacing);
    uint8_t getYGridSpacing(void) const;

    // Y Label
    void initYLabelInt(const int aYLabelStartValue, const int aYLabelIncrementValue, const float aYFactor,
            const uint8_t aMaxYLabelCharacters);
    void initYLabelFloat(const float aYLabelStartValue, const float aYLabelIncrementValue, const float aYFactor,
            const uint8_t aYMinStringWidthIncDecimalPoint, const uint8_t aYNumVarsAfterDecimal);
    void disableYLabel(void);

    // Start value
    void setYLabelStartValue(int yLabelStartValue);
    void setYLabelStartValueFloat(float yLabelStartValueFloat);
    void setYDataFactor(float aYDataFactor);
    int_float_union getYLabelStartValue(void) const;
    uint16_t getYLabelStartValueRawFromFloat(void);
    uint16_t getYLabelEndValueRawFromFloat(void);
    bool stepYLabelStartValueInt(const bool aDoIncrement, const int aMinValue, const int aMaxValue);
    float stepYLabelStartValueFloat(int aSteps);

    // Increment value
    void setYLabelBaseIncrementValue(int yLabelIncrementValue);
    void setYLabelBaseIncrementValueFloat(float yLabelIncrementValueFloat);
    // Increment factor
    //void setYScaleFactor(int aYScaleFactor, const bool doDraw);

    // Y Title
    void setYTitleText(const char * aLabelText);
    void drawYAxisTitle(const int aYOffset) const;

    // layout
    uint16_t mPositionX;    // Position in display coordinates of x - origin is on x axis
    uint16_t mPositionY;    // Position in display coordinates of y - origin is on y axis
    uint16_t mWidthX;       // length of x axes in pixel
    uint16_t mHeightY;      // height of y axes in pixel
    uint16_t mChartBackgroundColor;
    uint8_t mAxesSize;      //thickness of x and y axes - origin is on innermost line of axes
    uint8_t mFlags;
    // grid
    uint16_t mDataColor;
    uint16_t mAxesColor;
    uint16_t mGridColor;
    uint16_t mLabelColor;

    /*
     *  X axis
     */
    int_float_union mXLabelStartValue;
    // Value difference between 2 grid labels - the effective IncrementValue is mXScaleFactor * mXLabelBaseIncrementValue
    // This behavior is different to the Y axis because here we have only discrete (integer) scale factors.
    int_float_union mXLabelBaseIncrementValue;
    uint8_t mGridXSpacing; // difference in pixel between 2 X grid lines

    /**
     * aScaleFactor > 1 : expansion by factor aScaleFactor
     * aScaleFactor == 1 : expansion by 1.5
     * aScaleFactor == 0 : identity
     * aScaleFactor == -1 : compression by 1.5
     * aScaleFactor < -1 : compression by factor -aScaleFactor
     */
    int8_t mXScaleFactor; // Factor for X Data expansion(>0) or compression(<0). 2->display 1 value 2 times -2->display average of 2 values etc.

    // label formatting
    uint8_t mXNumVarsAfterDecimal;
    uint8_t mXMinStringWidth;

    const char* mXTitleText; // No title text if NULL

    /*
     * Y axis
     */
    int_float_union mYLabelStartValue;
    int_float_union mYLabelIncrementValue; // Value difference between 2 grid labels - serves as Y scale factor
    float mYDataFactor; // Factor for input (raw (int16_t) or float) to chart (not display!!!) value - e.g. (3.0 / 4096) for adc reading of 4096 for 3 Volt or 0.2 for 1000 display at 5000 raw value
    uint8_t mGridYSpacing; // difference in pixel between 2 Y grid lines

    // label formatting
    uint8_t mYNumVarsAfterDecimal;
    uint8_t mYMinStringWidth;

    const char* mYTitleText; // No title text if NULL

    uint8_t checkParameterValues();

};

#endif // _CHART_H
