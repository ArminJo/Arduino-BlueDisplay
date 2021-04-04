/*
 * BDSlider.h
 *
 *  SUMMARY
 *  Blue Display is an Open Source Android remote Display for Arduino etc.
 *  It receives basic draw requests from Arduino etc. over Bluetooth and renders it.
 *  It also implements basic GUI elements as buttons and sliders.
 *  GUI callback, touch and sensor events are sent back to Arduino.
 *
 *  Copyright (C) 2015  Armin Joachimsmeyer
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#ifndef BLUEDISPLAY_INCLUDE_BDSLIDER_H_
#define BLUEDISPLAY_INCLUDE_BDSLIDER_H_

#include <stdint.h>
/*
 * For more slider constants see BlueDisplay.h
 */
// Default values
#define SLIDER_DEFAULT_BORDER_COLOR     COLOR16_BLUE
#define SLIDER_DEFAULT_BAR_COLOR        COLOR16_GREEN
#define SLIDER_DEFAULT_BACKGROUND_COLOR COLOR16_WHITE
#define SLIDER_DEFAULT_THRESHOLD_COLOR  COLOR16_RED

#define SLIDER_DEFAULT_CAPTION_COLOR    COLOR16_BLACK
#define SLIDER_DEFAULT_CAPTION_BACKGROUND_COLOR    COLOR16_WHITE

// Flags for slider options
static const int FLAG_SLIDER_VERTICAL = 0x00;
static const int FLAG_SLIDER_VERTICAL_SHOW_NOTHING = 0x00;
static const int FLAG_SLIDER_SHOW_BORDER = 0x01;
static const int FLAG_SLIDER_SHOW_VALUE = 0x02;         // If set, ASCII value is printed along with change of bar value
static const int FLAG_SLIDER_IS_HORIZONTAL = 0x04;
static const int FLAG_SLIDER_IS_INVERSE = 0x08;         // is equivalent to negative slider length at init
static const int FLAG_SLIDER_VALUE_BY_CALLBACK = 0x10; // If set,  bar (+ ASCII) value will be set by callback handler, not by touch
static const int FLAG_SLIDER_IS_ONLY_OUTPUT = 0x20;     // is equivalent to slider aOnChangeHandler NULL at init

// Flags for slider caption position
static const int FLAG_SLIDER_CAPTION_ALIGN_LEFT_BELOW = 0x00;
static const int FLAG_SLIDER_CAPTION_ALIGN_LEFT = 0x00;
static const int FLAG_SLIDER_CAPTION_ALIGN_RIGHT = 0x01;
static const int FLAG_SLIDER_CAPTION_ALIGN_MIDDLE = 0x02;
static const int FLAG_SLIDER_CAPTION_BELOW = 0x00;
static const int FLAG_SLIDER_CAPTION_ABOVE = 0x04;

#ifdef LOCAL_DISPLAY_EXISTS
#include "TouchSlider.h"
// assume we have only a restricted amount of local sliders
typedef uint8_t BDSliderHandle_t;
#else
#ifdef AVR
typedef uint8_t BDSliderHandle_t;
#else
typedef uint16_t BDSliderHandle_t;
#endif
#endif

extern BDSliderHandle_t sLocalSliderIndex;

#include "Colors.h" // for color16_t

#ifdef __cplusplus
class BDSlider {
public:

    static void resetAllSliders(void);
    static void activateAllSliders(void);
    static void deactivateAllSliders(void);

    // Constructors
    BDSlider();
#ifdef LOCAL_DISPLAY_EXISTS
    BDSlider(BDSliderHandle_t aSliderHandle, TouchSlider *aLocalSliderPointer);
#endif

    /**
     * @brief initialization with all parameters except BarBackgroundColor
     * @param aPositionX determines upper left corner
     * @param aPositionY determines upper left corner
     * @param aBarWidth width of bar (and border) in pixel
     * @param aBarLength size of slider bar in pixel = maximum slider value
     * @param aThresholdValue value - if bigger, then color of bar changes from BarColor to BarBackgroundColor
     * @param aInitalValue
     * @param aSliderColor color of slider frame
     * @param aBarColor
     * @param aOptions see #FLAG_SLIDER_SHOW_BORDER etc.
     * @param aOnChangeHandler - if NULL no update of bar is done on touch
     */
    void init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aBarWidth, int16_t aBarLength, int16_t aThresholdValue,
            int16_t aInitalValue, color16_t aSliderColor, color16_t aBarColor, uint8_t aFlags,
            void (*aOnChangeHandler)(BDSlider *, uint16_t));

    void drawSlider(void);
    void drawBorder(void);
    void setValue(int16_t aCurrentValue);
    void setValueAndDrawBar(int16_t aCurrentValue);
    void setActualValue(int16_t aCurrentValue) __attribute__ ((deprecated ("Renamed to setValue()"))); // deprecated
    void setActualValueAndDrawBar(int16_t aCurrentValue) __attribute__ ((deprecated ("Renamed to setValueAndDrawBar()"))); // deprecated
    void setBarColor(color16_t aBarColor);
    void setBarThresholdColor(color16_t aBarThresholdColor);
    void setBarThresholdDefaultColor(color16_t aBarThresholdDefaultColor);
    void setBarBackgroundColor(color16_t aBarBackgroundColor);

    void setCaptionProperties(uint8_t aCaptionSize, uint8_t aCaptionPosition, uint8_t aCaptionMargin, color16_t aCaptionColor,
            color16_t aCaptionBackgroundColor);
    void setCaption(const char *aCaption);
    void setValueUnitString(const char *aValueUnitString);
    void setValueFormatString(const char *aValueFormatString);
    void setPrintValueProperties(uint8_t aPrintValueSize, uint8_t aPrintValuePosition, uint8_t aPrintValueMargin,
            color16_t aPrintValueColor, color16_t aPrintValueBackgroundColor);
    void printValue(const char *aValueString);
    /*
     * Scale factor of 2 means, that the slider is virtually 2 times larger than displayed
     */
    void setScaleFactor(float aScaleFactor);
    void setValueScaleFactor(float aScaleFactorValue); // calls setScaleFactor( 1/aScaleFactorValue);

    void activate(void);
    void deactivate(void);

    BDSliderHandle_t mSliderHandle;

#ifdef LOCAL_DISPLAY_EXISTS
    int printValue();
    void setXOffsetValue(int16_t aXOffsetValue);

    int16_t getCurrentValue(void) const;
    uint16_t getPositionXRight(void) const;
    uint16_t getPositionYBottom(void) const;
    void deinit(void);
    TouchSlider * mLocalSliderPointer;
#endif

private:
};
#endif // __cplusplus

/*
 * To show a signed value on two sliders positioned back to back (one of it is inverse or has a negative length value)
 */
struct positiveNegativeSlider {
    BDSlider *positiveSliderPtr;
    BDSlider *negativeSliderPtr;
    int lastSliderValue;      // positive value with sensor dead band applied
    BDSlider *lastZeroSlider; // to decide if we draw new zero slider
};
void initPositiveNegativeSliders(struct positiveNegativeSlider *aSliderStructPtr, BDSlider *aPositiveSliderPtr,
        BDSlider *aNegativeSliderPtr);
int setPositiveNegativeSliders(struct positiveNegativeSlider *aSliderStructPtr, int aValue, uint8_t aSliderDeadBand = 0);

#endif /* BLUEDISPLAY_INCLUDE_BDSLIDER_H_ */

#pragma once
