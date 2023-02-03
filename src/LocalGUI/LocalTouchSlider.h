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
class BDSlider;
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
// return values for checkAllSliders()
#ifndef NOT_TOUCHED
#define NOT_TOUCHED false
#endif
#define SLIDER_TOUCHED 4
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

    /*
     * Static functions
     */
#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
    static LocalTouchSlider* getLocalSliderFromBDSliderHandle(BDSliderHandle_t aSliderHandleToSearchFor);
    static void createAllLocalSlidersAtRemote();
#endif

    static void setDefaults(uintForPgmSpaceSaving aDefaultTouchBorder, uint16_t aDefaultSliderColor, uint16_t aDefaultBarColor,
            uint16_t aDefaultBarThresholdColor, uint16_t aDefaultBarBackgroundColor, uint16_t aDefaultCaptionColor,
            uint16_t aDefaultValueColor, uint16_t aDefaultValueCaptionBackgroundColor);
    static void setDefaultSliderColor(uint16_t aDefaultSliderColor);
    static void setDefaultBarColor(uint16_t aDefaultBarColor);
    static bool checkAllSliders(unsigned int aTouchPositionX, unsigned int aTouchPositionY);
    static void deactivateAllSliders();
    static void activateAllSliders();

    /*
     * Member functions
     */
    void init(uint16_t aPositionX, uint16_t aPositionY, uint8_t aBarWidth, uint16_t aBarLength, uint16_t aThresholdValue,
            int16_t aInitalValue, uint16_t aSliderColor, uint16_t aBarColor, uint8_t aFlags,
            void (*aOnChangeHandler)(LocalTouchSlider*, uint16_t));
    void deinit(); // Dummy to be more compatible with BDButton

    void initSliderColors(uint16_t aSliderColor, uint16_t aBarColor, uint16_t aBarThresholdColor, uint16_t aBarBackgroundColor,
            uint16_t aCaptionColor, uint16_t aValueColor, uint16_t aValueCaptionBackgroundColor);

    void drawSlider();
    bool checkSlider(uint16_t aPositionX, uint16_t aPositionY);
    void drawBar();
    void drawBorder();

    void activate();
    void deactivate();

    void setPosition(int16_t aPositionX, int16_t aPositionY);
    uint16_t getPositionXRight() const;
    uint16_t getPositionYBottom() const;

    void setBarBackgroundColor(uint16_t aBarBackgroundColor);
    void setValueAndCaptionBackgroundColor(uint16_t aValueCaptionBackgroundColor);
    void setValueColor(uint16_t aValueColor);
    void setCaptionColors(uint16_t aCaptionColor, uint16_t aValueCaptionBackgroundColor);
    void setValueStringColors(uint16_t aValueStringColor, uint16_t aValueStringCaptionBackgroundColor);
    void setSliderColor(uint16_t sliderColor);
    void setBarColor(uint16_t barColor);
    void setBarThresholdColor(uint16_t barThresholdColor);
    uint16_t getBarColor() const;

    void setValue(int16_t aCurrentValue);
    void setValueAndDraw(int16_t aCurrentValue);
    void setValueAndDrawBar(int16_t aCurrentValue);
    int16_t getCurrentValue() const;

    void setCaption(const char *aCaption);
    void printCaption();
    int printValue();
    int printValue(const char *aValueString);
    void setXOffsetValue(int16_t aXOffsetValue);

    // Dummy stubs to compatible with BDSliders
    void setCaptionProperties(uint8_t aCaptionSize, uint8_t aCaptionPosition, uint8_t aCaptionMargin, color16_t aCaptionColor,
            color16_t aCaptionBackgroundColor);
    void setPrintValueProperties(uint8_t aPrintValueTextSize, uint8_t aPrintValuePosition, uint8_t aPrintValueMargin,
            color16_t aPrintValueColor, color16_t aPrintValueBackgroundColor);

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

    // the value as provided by touch
    uint16_t mActualTouchValue;
    // This value can be different from mActualTouchValue and is provided by callback handler
    uint16_t mActualValue;

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
