/*
 * BDSlider.h
 *
 * The constants here must correspond to the values used in the BlueDisplay App
 *
 *  Copyright (C) 2015-2023  Armin Joachimsmeyer
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

#ifndef _BDSLIDER_H
#define _BDSLIDER_H

#include <stdint.h>
/*
 * For more slider constants see BlueDisplay.h
 */
// Default values
#define SLIDER_DEFAULT_BORDER_COLOR         COLOR16_BLUE
#define SLIDER_DEFAULT_BAR_COLOR            COLOR16_GREEN
#define SLIDER_DEFAULT_BAR_THRESHOLD_COLOR  COLOR16_RED
#define SLIDER_DEFAULT_THRESHOLD_COLOR      SLIDER_DEFAULT_BAR_THRESHOLD_COLOR
#define SLIDER_DEFAULT_BAR_BACKGROUND_COLOR COLOR16_WHITE
#define SLIDER_DEFAULT_BACKGROUND_COLOR     SLIDER_DEFAULT_BAR_BACKGROUND_COLOR
#define SLIDER_DEFAULT_CAPTION_COLOR        COLOR16_BLACK
#define SLIDER_DEFAULT_CAPTION_BACKGROUND_COLOR    COLOR16_WHITE

// Flags for slider options
static const int FLAG_SLIDER_VERTICAL = 0x00;
static const int FLAG_SLIDER_VERTICAL_SHOW_NOTHING = 0x00;
static const int FLAG_SLIDER_SHOW_BORDER = 0x01;
static const int FLAG_SLIDER_SHOW_VALUE = 0x02;         // If set, ASCII value is printed along with change of bar value
static const int FLAG_SLIDER_IS_HORIZONTAL = 0x04;
static const int FLAG_SLIDER_IS_INVERSE = 0x08;         // is equivalent to negative slider length at init
static const int FLAG_SLIDER_VALUE_BY_CALLBACK = 0x10;  // If set, bar (+ ASCII) value will be set by callback handler, not by touch
static const int FLAG_SLIDER_IS_ONLY_OUTPUT = 0x20;     // is equivalent to slider aOnChangeHandler NULL at init
// LOCAL_SLIDER_FLAG_USE_BDSLIDER_FOR_CALLBACK is set, when we have a local and a remote slider, i.e. SUPPORT_REMOTE_AND_LOCAL_DISPLAY is defined.
// Then only the remote slider pointer is used as callback parameter to enable easy comparison of this parameter with a fixed slider.
#define LOCAL_SLIDER_FLAG_USE_BDSLIDER_FOR_CALLBACK 0x80

// Flags for slider value and caption position
#define FLAG_SLIDER_VALUE_CAPTION_ALIGN_LEFT_BELOW   0x00
#define FLAG_SLIDER_VALUE_CAPTION_ALIGN_LEFT         0x00
#define FLAG_SLIDER_VALUE_CAPTION_ALIGN_RIGHT        0x01
#define FLAG_SLIDER_VALUE_CAPTION_ALIGN_MIDDLE       0x02
#define FLAG_SLIDER_VALUE_CAPTION_BELOW              0x00
#define FLAG_SLIDER_VALUE_CAPTION_ABOVE              0x04
#define FLAG_SLIDER_VALUE_TAKE_DEFAULT_MARGIN        0x08 // supported since BlueDisplay App 4.3.2

static const int FLAG_SLIDER_CAPTION_ALIGN_LEFT_BELOW = 0x00;
static const int FLAG_SLIDER_CAPTION_ALIGN_LEFT = 0x00;
static const int FLAG_SLIDER_CAPTION_ALIGN_RIGHT = 0x01;
static const int FLAG_SLIDER_CAPTION_ALIGN_MIDDLE = 0x02;
static const int FLAG_SLIDER_CAPTION_BELOW = 0x00;
static const int FLAG_SLIDER_CAPTION_ABOVE = 0x04;

#define SLIDER_DEFAULT_VALUE_MARGIN 4 // is defined as mRPCView.mRequestedCanvasHeight / 60 -> for a height of 240 we get 4

#if defined(SUPPORT_LOCAL_DISPLAY)
#include "LocalGUI/LocalTouchSlider.h"
#endif

#if defined(AVR)
typedef uint8_t BDSliderHandle_t;
#else
typedef uint16_t BDSliderHandle_t;
#endif

extern BDSliderHandle_t sLocalSliderIndex;

#include "Colors.h" // for color16_t

#ifdef __cplusplus
class BDSlider {
public:

    // Constructors
    BDSlider();

    void init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aBarWidth, int16_t aBarLength, int16_t aThresholdValue,
            int16_t aInitalValue, color16_t aSliderColor, color16_t aBarColor, uint8_t aFlags,
            void (*aOnChangeHandler)(BDSlider*, uint16_t));
    void deinit(); // is defined as dummy if SUPPORT_LOCAL_DISPLAY is not active
    void activate();
    void deactivate();

    // Defaults
    void setDefaultBarThresholdColor(color16_t aDefaultBarThresholdColor);

    // TODO !!! Possible memory leak !!!
    static void resetAll();

    /*
     * Functions using the list of all sliders
     */
    static void activateAll();
    static void deactivateAll();

    // Position
    void setPosition(int16_t aPositionX, int16_t aPositionY);

    // Draw
    void drawSlider();
    void drawBorder();

    // Color
    void setBarColor(color16_t aBarColor);
    void setBarThresholdColor(color16_t aBarThresholdColor);
    void setBarBackgroundColor(color16_t aBarBackgroundColor);

    // Caption
    void setCaption(const char *aCaption);
    void setCaptionProperties(uint8_t aCaptionSize, uint8_t aCaptionPositionFlags, uint8_t aCaptionMargin, color16_t aCaptionColor,
            color16_t aCaptionBackgroundColor);

    // Value
    void setValue(int16_t aCurrentValue);
    void setValueAndDrawBar(int16_t aCurrentValue);
    void setValue(int16_t aCurrentValue, bool doDrawBar);

    void printValue(const char *aValueString);
    void setValueUnitString(const char *aValueUnitString);
    void setValueFormatString(const char *aValueFormatString);
    void setPrintValueProperties(uint8_t aPrintValueSize, uint8_t aPrintValuePositionFlags, uint8_t aPrintValueMargin,
            color16_t aPrintValueColor, color16_t aPrintValueBackgroundColor);

    /*
     * Scale factor of 2 means, that the slider is virtually 2 times larger than displayed
     */
    void setScaleFactor(float aScaleFactor);
    void setValueScaleFactor(float aScaleFactorValue); // calls setScaleFactor( 1/aScaleFactorValue);

    BDSliderHandle_t mSliderHandle;

#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalTouchSlider * mLocalSliderPointer;
#endif

    void setBarThresholdDefaultColor(color16_t aBarThresholdDefaultColor)  __attribute__ ((deprecated ("Renamed to setDefaultBarThresholdColor")));
    static void resetAllSliders() __attribute__ ((deprecated ("Renamed to resetAll")));
    static void activateAllSliders() __attribute__ ((deprecated ("Renamed to activateAll")));
    static void deactivateAllSliders() __attribute__ ((deprecated ("Renamed to deactivateAll")));

private:
};
#endif // __cplusplus

/*
 * To show a signed value on two sliders positioned back to back (one of it is inverse or has a negative length value)
 */
struct positiveNegativeSlider {
    BDSlider *positiveSliderPtr;
    BDSlider *negativeSliderPtr;
    unsigned int lastSliderValue;       // positive value with sensor dead band applied
    bool lastSliderValueWasPositive;    // true if positive slider had a value and negative was cleared
};
void initPositiveNegativeSliders(struct positiveNegativeSlider *aSliderStructPtr, BDSlider *aPositiveSliderPtr,
        BDSlider *aNegativeSliderPtr);
int setPositiveNegativeSliders(struct positiveNegativeSlider *aSliderStructPtr, int aValue, uint8_t aSliderDeadBand = 0);
unsigned int setPositiveNegativeSliders(struct positiveNegativeSlider *aSliderStructPtr, unsigned int aValue, bool aPositiveSider,
        uint8_t aSliderDeadBand);

#endif //_BDSLIDER_H
