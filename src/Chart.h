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
#define CHART_ERROR_POS_X            -1
#define CHART_ERROR_POS_Y            -2
#define CHART_ERROR_WIDTH            -4
#define CHART_ERROR_HEIGHT           -8
#define CHART_ERROR_AXES_SIZE       -16
#define CHART_ERROR_GRID_X_SPACING  -32

// Masks for mFlags
#define CHART_HAS_GRID      0x01 // no marks for label are rendered
#define CHART_X_LABEL_USED  0x02
#define CHART_X_LABEL_INT   0x04 // else label is float
#define CHART_Y_LABEL_USED  0x08
#define CHART_Y_LABEL_INT   0x10 // else label is float
typedef union {
    int IntValue;
    float FloatValue;
} int_float_union;

#if !defined(__time_t_defined) // avoid conflict with newlib or other posix libc
typedef unsigned long time_t;
#endif

struct drawXAxisTimeDateSettingsStruct {
    int (*firstTokenFunction)(time_t t);
    int (*secondTokenFunction)(time_t t);
    char TokenSeparatorChar;
    int (*intermediateTokenFunction)(time_t t);
    uint8_t maximumCharactersOfLabel; // Used for clearing label space before
};

typedef union {
    int IntValue;
    long LongValue;
    float FloatValue;
} int_long_float_union;

typedef union {
    long LongValue;
    float FloatValue;
} long_float_union;
/*
 * sizeof(Chart) is 62 bytes
 */
class Chart {
public:
    Chart();
    uint8_t initChart(const uint16_t aPositionX, const uint16_t aPositionY, const uint16_t aWidthX, const uint16_t aHeightY,
            const uint8_t aAxesSize, const uint8_t aLabelTextSize, const bool aHasGrid, const uint16_t aGridOrLabelXPixelSpacing,
            const uint16_t aGridOrLabelYPixelSpacing);

    void initChartColors(const color16_t aDataColor, const color16_t aAxesColor, const color16_t aGridColor,
            const color16_t aLabelColor, const color16_t aBackgroundColor);
    void setDataColor(color16_t aDataColor);
    void setBackgroundColor(color16_t aBackgroundColor);
    void setLabelColor(color16_t aLabelColor);

    void clear(void);

    void setWidthX(const uint16_t widthX);
    void setHeightY(const uint16_t heightY);
    void setPositionX(const uint16_t positionX);
    void setPositionY(const uint16_t positionY);
    void setGridPixelSpacing(uint8_t aXGridSpacing, uint8_t aYGridSpacing);

    uint16_t getWidthX(void) const;
    uint16_t getHeightY(void) const;
    uint16_t getPositionX(void) const;
    uint16_t getPositionY(void) const;

    void drawAxesAndLabels();
    void drawAxesAndGrid(void);
    bool drawChartDataDirect(const uint8_t *aDataPointer, const uint16_t aLengthOfValidData, const uint8_t aMode);
    void drawChartDataWithYOffset(uint8_t *aDataPointer, const uint16_t aLengthOfValidData, const uint8_t aMode); // 8 Bit (compressed) data with factor and offset
    void drawChartData(int16_t *aDataPointer, const uint16_t aLengthOfValidData, const uint8_t aMode);       // 16 bit data
    void drawChartDataFloat(float *aDataPointer, const uint16_t aLengthOfValidData, const uint8_t aMode);
    void drawGrid(void);

    /*
     * X Axis
     */
    void drawXAxisAndLabels();
    void drawXAxisAndDateLabels(time_t aStartTimestamp, drawXAxisTimeDateSettingsStruct *aDrawXAxisTimeDateSettings);

    void setXLabelDistance(uint8_t aXLabelDistance);
    void setGridXPixelSpacing(uint8_t aXGridSpacing);
    uint8_t getGridXPixelSpacing(void) const;

    /*
     * X Label
     */
    // Init
    void initXLabelInteger(const int aXLabelStartValue, const long aXLabelIncrementValue, const uint8_t aXLabelScaleFactor,
            const uint8_t aXMinStringWidth);
    void initXLabelFloat(const float aXLabelStartValue, const float aXLabelIncrementValue, const uint8_t aXLabelScaleFactor,
            uint8_t aXMinStringWidthIncDecimalPoint, uint8_t aXNumVarsAfterDecimal);
    void disableXLabel(void);

    // Start value
    void setXLabelStartValue(int xLabelStartValue);
    void setXLabelStartValueFloat(float xLabelStartValueFloat);
    void setXLabelIntStartValueByIndex(const int aNewXStartIndex, const bool doRedrawXAxis); // does not use IntegerScaleFactor
    int_long_float_union getXLabelStartValue(void) const;
    bool stepXLabelStartValueInt(const bool aDoIncrement, const int aMinValue, const int aMaxValue); // does not use IntegerScaleFactor
    float stepXLabelStartValueFloat(const bool aDoIncrement); // does not use IntegerScaleFactor

    // Increment value
    void setXLabelBaseIncrementValue(long xLabelIncrementValue);
    void setXLabelBaseIncrementValueFloat(float xLabelIncrementValueFloat);

    // access factors
    void setXLabelScaleFactor(int aXLabelScaleFactor);
    int getXLabelScaleFactor(void) const;
    void setXDataScaleFactor(int8_t aIntegerScaleFactor);
    int8_t getXDataScaleFactor(void) const;
    void setXLabelAndXDataScaleFactor(int aXFactor);

    long reduceLongWithIntegerScaleFactor(long aValue);
    float reduceFloatWithIntegerScaleFactor(float Value);
    int enlargeLongWithIntegerScaleFactor(int Value);
    float enlargeFloatWithIntegerScaleFactor(float Value);
    static long reduceLongWithIntegerScaleFactor(long aValue, int aScaleFactor);
    static void getIntegerScaleFactorAsString(char *tStringBuffer, int aScaleFactor);
    static float reduceFloatWithIntegerScaleFactor(float aValue, int aScaleFactor);
    int8_t computeXFactor(uint16_t aDataLength);
    void computeAndSetXLabelAndXDataScaleFactor(uint16_t aDataLength);

    // X Title
    void setXTitleText(const char *aLabelText);
    void drawXAxisTitle() const;
    void setTitleTextSize(const uint8_t aTitleTextSize);

    /*
     *  Y Axis
     */
    void drawYAxisAndLabels();

    void setGridYPixelSpacing(uint8_t aYGridSpacing);
    uint8_t getGridYPixelSpacing(void) const;
    void setXLabelAndGridOffset(float aXLabelAndGridOffset);
    void setXAxisTimeDateSettings(drawXAxisTimeDateSettingsStruct *aDrawXAxisTimeDateSettingsStructToFill,
            int (*aFirstTokenFunction)(time_t t), int (*aSecondTokenFunction)(time_t t), char aTokenSeparatorChar,
            int (*aIntermediateTokenFunction)(time_t t), uint8_t aMaximumCharactersOfLabel);

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
    void setYTitleText(const char *aLabelText);
    void drawYAxisTitle(const int aYOffset) const;

    // layout all values are in pixels
    uint16_t mPositionX;    // Position in display coordinates of x - origin is on x axis
    uint16_t mPositionY;    // Position in display coordinates of y - origin is on y axis
    uint16_t mWidthX;       // length of x axes in pixels
    uint16_t mHeightY;      // height of y axes in pixel
    uint8_t mAxesSize;      // thickness of x and y axes - origin is on innermost line of axes
    uint8_t mLabelTextSize;
    uint8_t mTitleTextSize;
    uint8_t mFlags;

    // Colors
    color16_t mDataColor;
    color16_t mAxesColor;
    color16_t mGridColor;
    color16_t mLabelColor;
    color16_t mBackgroundColor;

    /*
     *  X axis
     */
    int_long_float_union mXLabelStartValue;

    /*
     * If mXLabelAndGridStartValueOffset == mXLabelBaseIncrementValue then grid starts with 2. value
     * If label distance is 1, then label also starts with 2. value
     * Pixel offset is (mXLabelAndGridStartValueOffset / mXLabelBaseIncrementValue) * mGridXPixelSpacing
     * for X ScaleFactor != identity, pixel offset is (mXLabelAndGridStartValueOffset / adjusted(mXLabelBaseIncrementValue)) * mGridXPixelSpacing
     * because increment has changed by mXLabelScaleFactor
     * Offset positive -> grid is shifted left
     * If offset is negative, the starting intermediate labels are not rendered!
     */
    float mXLabelAndGridStartValueOffset; // The value in the same units as mXLabelBaseIncrementValue. If > 0, the label and grid will be shifted left

    /*
     * Value difference between 2 grid labels - the effective IncrementValue is mXLabelScaleFactor * mXLabelBaseIncrementValue
     * This behavior is different to the Y axis.
     * Here we have only discrete (integer) scale factors to easily support local displays.
     */
    long_float_union mXLabelBaseIncrementValue; // The base increment value for one grid
    uint8_t mGridXPixelSpacing; // Difference in pixel between 2 X grid lines

    /*
     * Scale factor is CHART_WIDTH / lengthOfDataToShow if this is > 1
     */
#define CHART_X_AXIS_SCALE_FACTOR_1                 0 // identity is code with 0
#define CHART_X_AXIS_SCALE_FACTOR_EXPANSION_1_5     1 // expansion by 1.5
#define CHART_X_AXIS_SCALE_FACTOR_EXPANSION_2       2 // expansion by factor 2
#define CHART_X_AXIS_SCALE_FACTOR_EXPANSION_3       3 // expansion by factor 3
#define CHART_X_AXIS_SCALE_FACTOR_EXPANSION_4       4 // expansion by factor 4
#define CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_1_5  -1 // compression by 1.5
#define CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_2    -2 // compression by factor 2
#define CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_3    -3 // compression by factor 3
#define CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_4    -4 // compression by factor 4
    /**
     * Factor > 1 : expansion by factor Factor. E.g. one value is rendered twice, label increment value is halve
     * Factor == 1 : expansion by 1.5
     * Factor == 0 : identity
     * Factor == -1 : compression by 1.5
     * Factor < -1 : compression by factor Factor E.g. two values are rendered as one, label increment value is doubled
     */
    // Normally these factors are equal.
    int8_t mXDataScaleFactor; // Factor for X Data expansion(>0) or compression(<0). 2->display 1 value 2 times -2->display average of 2 values etc.
    int8_t mXLabelScaleFactor; // Factor for X scale expansion(>0) or compression(<0). 2-> use half the increment per label -2-> use double of increment per label

    // label formatting
    uint8_t mXNumVarsAfterDecimal;
    uint8_t mXMinStringWidth;
    uint8_t mXLabelDistance; // 1 label at every grid position, 2 label at every 2. grid position etc.

    const char *mXTitleText; // No title text if NULL

    /*
     * Y axis
     */
    int_float_union mYLabelStartValue;
    int_float_union mYLabelStartOffset;
    int_float_union mYLabelIncrementValue; // Value difference between 2 grid labels - serves as Y scale factor
    float mYDataFactor; // Factor for input (raw (int16_t) or float) to chart (not display!!!) value - e.g. (3.0 / 4096) for adc reading of 4096 for 3 (Volt) or 0.2 for 1000 display at 5000 input value
    uint8_t mGridYPixelSpacing; // difference in pixel between 2 Y grid lines

    // label formatting
    uint8_t mYNumVarsAfterDecimal;
    uint8_t mYMinStringWidth;

    const char *mYTitleText; // No title text if NULL

    uint8_t checkParameterValues();

};
void showChartDemo(void);

#endif // _CHART_H
