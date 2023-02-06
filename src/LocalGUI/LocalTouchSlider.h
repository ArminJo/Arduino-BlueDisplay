/**
 *
 * LocalTouchSlider.h
 *
 * Size of one slider is 46 bytes on Arduino
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
 *
 */

#ifndef _LOCAL_TOUCH_SLIDER_H
#define _LOCAL_TOUCH_SLIDER_H

#if !defined(DISABLE_REMOTE_DISPLAY)
class BDSlider;

#  if defined(AVR)
typedef uint8_t BDSliderHandle_t;
#  else
typedef uint16_t BDSliderHandle_t;
#  endif
#endif

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Slider
 * @{
 */
/**
 * @name SliderOptions
 * Options for LocalTouchSlider::initSlider() - they can be or-ed together
 * @{
 */

// A copy from BDSlider.h for documentation
//static const int FLAG_SLIDER_VERTICAL = 0x00;
//static const int FLAG_SLIDER_VERTICAL_SHOW_NOTHING = 0x00;
//static const int FLAG_SLIDER_SHOW_BORDER = 0x01;
//static const int FLAG_SLIDER_SHOW_VALUE = 0x02;         // If set, ASCII value is printed along with change of bar value
//static const int FLAG_SLIDER_IS_HORIZONTAL = 0x04;
//static const int FLAG_SLIDER_IS_INVERSE = 0x08;         // is equivalent to negative slider length at init
//static const int FLAG_SLIDER_VALUE_BY_CALLBACK = 0x10;  // If set, bar (+ ASCII) value will be set by callback handler, not by touch
//static const int FLAG_SLIDER_IS_ONLY_OUTPUT = 0x20;     // is equivalent to slider aOnChangeHandler NULL at init
//#define LOCAL_SLIDER_FLAG_USE_BDSLIDER_FOR_CALLBACK 0x80 // Use pointer to index in slider list instead of pointer to this in callback
/** @} */

#define SLIDER_DEFAULT_VALUE_COLOR          COLOR16_BLUE
#define SLIDER_DEFAULT_CAPTION_VALUE_BACK_COLOR  COLOR16_NO_BACKGROUND
#define SLIDER_DEFAULT_BAR_WIDTH            8
#define SLIDER_MAX_BAR_WIDTH                40 /** global max value of size parameter */
#define SLIDER_DEFAULT_TOUCH_BORDER         4 /** extension of touch region in pixel */
#define SLIDER_DEFAULT_SHOW_CAPTION         true
#define SLIDER_DEFAULT_SHOW_VALUE           true
#define SLIDER_DEFAULT_MAX_VALUE            160
#define SLIDER_DEFAULT_THRESHOLD_VALUE      100

#define SLIDER_MAX_DISPLAY_VALUE            LOCAL_DISPLAY_WIDTH //! maximum value which can be displayed
/**
 * @name SliderErrorCodes
 * @{
 */
#define SLIDER_ERROR     -1
/** @} */

// Typedef in order to save program space on AVR, but allow bigger values on other platforms
#ifdef AVR
typedef uint8_t uintForPgmSpaceSaving;
typedef uint8_t uintForRamSpaceSaving;
#else
typedef unsigned int uintForPgmSpaceSaving;
typedef uint16_t uintForRamSpaceSaving;
#endif

class LocalTouchSlider {
public:

    LocalTouchSlider();

#if !defined(ARDUINO)
    ~LocalTouchSlider();
#endif
    void init(uint16_t aPositionX, uint16_t aPositionY, uint8_t aBarWidth, uint16_t aBarLength, uint16_t aThresholdValue,
            int16_t aInitalValue, uint16_t aSliderColor, uint16_t aBarColor, uint8_t aFlags,
            void (*aOnChangeHandler)(LocalTouchSlider*, uint16_t));
    void deinit(); // Dummy to be more compatible with BDButton
    void activate();
    void deactivate();

    void initSliderColors(uint16_t aSliderColor, uint16_t aBarColor, uint16_t aBarThresholdColor, uint16_t aBarBackgroundColor,
            uint16_t aCaptionColor, uint16_t aValueColor, uint16_t aValueCaptionBackgroundColor);

#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
    static LocalTouchSlider* getLocalSliderFromBDSliderHandle(BDSliderHandle_t aSliderHandleToSearchFor);
    static void createAllLocalSlidersAtRemote();
#endif

    // Defaults
    static void setDefaults(uintForPgmSpaceSaving aDefaultTouchBorder, uint16_t aDefaultSliderColor, uint16_t aDefaultBarColor,
            uint16_t aDefaultBarThresholdColor, uint16_t aDefaultBarBackgroundColor, uint16_t aDefaultCaptionColor,
            uint16_t aDefaultValueColor, uint16_t aDefaultValueCaptionBackgroundColor);
    static void setDefaultSliderColor(uint16_t aDefaultSliderColor);
    static void setDefaultBarColor(uint16_t aDefaultBarColor);
    void setDefaultBarThresholdColor(color16_t aDefaultBarThresholdColor);

    /*
     * Basic touch functions
     */
    bool isTouched(uint16_t aTouchPositionX, uint16_t aTouchPositionY);
    void performTouchAction(uint16_t aTouchPositionX, uint16_t aTouchPositionY);

    /*
     * Functions using the list of all sliders
     */
    static void activateAll();
    static void deactivateAll();

    static LocalTouchSlider* find(unsigned int aTouchPositionX, unsigned int aTouchPositionY);
    static LocalTouchSlider* findAndAction(unsigned int aTouchPositionX, unsigned int aTouchPositionY);
    static bool checkAllSliders(unsigned int aTouchPositionX, unsigned int aTouchPositionY);

    // Position
    void setPosition(int16_t aPositionX, int16_t aPositionY);
    uint16_t getPositionXRight() const;
    uint16_t getPositionYBottom() const;

    // Draw
    void drawSlider();
    void drawBorder();
    void drawBar(); // Used internally

    // Color
    void setSliderColor(uint16_t sliderColor);
    void setBarColor(uint16_t aBarColor);
    void setBarThresholdColor(uint16_t aBarThresholdColor);
    void setBarBackgroundColor(uint16_t aBarBackgroundColor);

    void setValueColor(uint16_t aValueColor);
    void setValueStringColors(uint16_t aValueStringColor, uint16_t aValueStringCaptionBackgroundColor);
    void setCaptionColors(uint16_t aCaptionColor, uint16_t aValueCaptionBackgroundColor);
    void setValueAndCaptionBackgroundColor(uint16_t aValueCaptionBackgroundColor); // in setCaptionProperties()

    uint16_t getBarColor() const;

    // Caption
    void setCaption(const char *aCaption);
    // Dummy stub to compatible with BDSliders
    void setCaptionProperties(uint8_t aCaptionSize, uint8_t aCaptionPositionFlags, uint8_t aCaptionMargin, color16_t aCaptionColor,
            color16_t aValueCaptionBackgroundColor); // Uses setCaptionColors() and sets only aCaptionColor and aValueCaptionBackgroundColor
    void printCaption(); // Used internally

    // Value
    void setValue(int16_t aValue);
    void setValueAndDrawBar(int16_t aValue);

    int printValue(const char *aValueString);
    void setXOffsetValue(int16_t aXOffsetValue); // Set offset for printValue
    // Dummy stub to compatible with BDSliders
    void setPrintValueProperties(uint8_t aPrintValueTextSize, uint8_t aPrintValuePositionFlags, uint8_t aPrintValueMargin,
            color16_t aPrintValueColor, color16_t aPrintValueBackgroundColor); // Uses setValueStringColors() and sets only aPrintValueColor and aPrintValueBackgroundColor
    int16_t getValue() const;
    int printValue(); // Used internally

    // Deprecated
    void setBarThresholdDefaultColor(color16_t aBarThresholdDefaultColor)
            __attribute__ ((deprecated ("Renamed to setDefaultBarThresholdColor")));
    void setValueAndDraw(int16_t aValue) __attribute__ ((deprecated ("Renamed to setValueAndDrawBar")));
    static void activateAllSliders() __attribute__ ((deprecated ("Renamed to activateAll")));
    static void deactivateAllSliders() __attribute__ ((deprecated ("Renamed to deactivateAll")));

#if !defined(DISABLE_REMOTE_DISPLAY)
    LocalTouchSlider(BDSlider *aBDSliderPtr); // Required for creating a local slider for an existing aBDSlider at aBDSlider::init
    BDSlider *mBDSliderPtr;
#endif

    /*
     * Slider list, required for the *AllSliders functions
     */
    static LocalTouchSlider *sSliderListStart; // Start of list of touch sliders, required for the *AllSliders functions
    LocalTouchSlider *mNextObject;

    /*
     * Defaults
     */
    static uint16_t sDefaultSliderColor;
    static uint16_t sDefaultBarColor;
    static uint16_t sDefaultBarThresholdColor;
    static uint16_t sDefaultBarBackgroundColor;
    static uint16_t sDefaultCaptionColor;
    static uint16_t sDefaultValueColor;
    static uint16_t sDefaultValueCaptionBackgroundColor;
    static uint8_t sDefaultTouchBorder;

    /*
     * The Slider
     */
    uint16_t mPositionX; // X left
    uint16_t mPositionXRight;
    uint16_t mXOffsetValue;
    uint16_t mPositionY; // Y top
    uint16_t mPositionYBottom;
    uint16_t mBarLength; //aMaxValue serves also as height
    uint16_t mThresholdValue; // Value for color change
    uintForRamSpaceSaving mBarWidth; // Size of border and bar
    const char *mCaption; // No caption if NULL
    uint8_t mTouchBorder; // extension of touch region
    uint8_t mFlags;

    uint16_t mActualTouchValue; // The value as provided by touch
    uint16_t mValue; // This value can be different from mActualTouchValue and is provided by callback handler

    // Colors
    uint16_t mSliderColor;
    uint16_t mBarColor;
    uint16_t mBarThresholdColor;
    uint16_t mBarBackgroundColor;
    uint16_t mCaptionColor;
    uint16_t mValueColor;
    uint16_t mValueCaptionBackgroundColor;

    bool mIsActive;

    // misc
    void (*mOnChangeHandler)(LocalTouchSlider*, uint16_t);

    int8_t checkParameterValues();

};
/** @} */
/** @} */

#endif /* _LOCAL_TOUCH_SLIDER_H_ */
