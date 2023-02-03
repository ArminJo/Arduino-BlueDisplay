/*
 * BDSlider.hpp
 *
 * Implementation of the slider client stub for the Android BlueDisplay app.
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

#ifndef _BDSLIDER_HPP
#define _BDSLIDER_HPP

#include "BDSlider.h"
#include "BlueDisplayProtocol.h"
#include "BlueSerial.h"

#if defined(SUPPORT_LOCAL_DISPLAY)
#include "LocalGUI/LocalTouchSlider.hpp"
#endif

#include <string.h>  // for strlen

BDSliderHandle_t sLocalSliderIndex = 0;

BDSlider::BDSlider() { // @suppress("Class members should be properly initialized")
}

#if defined(SUPPORT_LOCAL_DISPLAY)
BDSlider::BDSlider(BDSliderHandle_t aSliderHandle, LocalTouchSlider *aLocalSliderPointer) {
    mSliderHandle = aSliderHandle;
    mLocalSliderPointer = aLocalSliderPointer;
}
#endif

/**
 * @brief initialization with all parameters (except BarBackgroundColor)
 * @param aPositionX - Determines upper left corner
 * @param aPositionY - Determines upper left corner
 * Only next 2 values are physical values in pixel
 * @param aBarWidth - Width of bar (and border) in pixel - no scaling!
 * @param aBarLength - Size of slider bar in pixel = maximum slider value if no scaling applied!
 *                     Negative means slider bar is top down an is equivalent to positive with FLAG_SLIDER_IS_INVERSE.
 * @param aThresholdValue - Scaling applied! If selected or sent value is bigger, then color of bar changes from BarColor to BarBackgroundColor
 * @param aInitalValue - Scaling applied!
 * @param aSliderColor - Color of slider border. If no border specified, then bar background color.
 * @param aBarColor
 * @param aFlags - See #FLAG_SLIDER_SHOW_BORDER etc.
 * @param aOnChangeHandler - If NULL no update of bar is done on touch - equivalent to FLAG_SLIDER_IS_ONLY_OUTPUT
 */
void BDSlider::init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aBarWidth, int16_t aBarLength, int16_t aThresholdValue,
        int16_t aInitalValue, color16_t aSliderColor, color16_t aBarColor, uint8_t aFlags,
        void (*aOnChangeHandler)(BDSlider*, uint16_t)) {
    BDSliderHandle_t tSliderNumber = sLocalSliderIndex++;

    if (USART_isBluetoothPaired()) {
#if __SIZEOF_POINTER__ == 4
        sendUSARTArgs(FUNCTION_SLIDER_CREATE, 12, tSliderNumber, aPositionX, aPositionY, aBarWidth, aBarLength, aThresholdValue,
                aInitalValue, aSliderColor, aBarColor, aFlags, aOnChangeHandler,
                (reinterpret_cast<uint32_t>(aOnChangeHandler) >> 16));
#else
        sendUSARTArgs(FUNCTION_SLIDER_CREATE, 11, tSliderNumber, aPositionX, aPositionY, aBarWidth, aBarLength, aThresholdValue,
                aInitalValue, aSliderColor, aBarColor, aFlags, aOnChangeHandler);
#endif
    }
    mSliderHandle = tSliderNumber;

#if defined(SUPPORT_LOCAL_DISPLAY)
#  if defined(DISABLE_REMOTE_DISPLAY)
    mLocalSliderPointer = new LocalTouchSlider();
#  else
    mLocalSliderPointer = new LocalTouchSlider(this);
#  endif
    // Cast required here. At runtime the right pointer is returned because of FLAG_USE_INDEX_FOR_CALLBACK
    mLocalSliderPointer->init(aPositionX, aPositionY, aBarWidth, aBarLength, aThresholdValue, aInitalValue,
            aSliderColor, aBarColor, aFlags | LOCAL_SLIDER_FLAG_USE_BDSLIDER_FOR_CALLBACK,
            reinterpret_cast<void (*)(LocalTouchSlider *, uint16_t)>(aOnChangeHandler));
    // keep the formatting
#endif
}


/*
 * For description see BDButton::deinit()
 */
void BDSlider::deinit() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    sLocalSliderIndex--;
    delete mLocalSliderPointer;
#endif
}

/**
 * @param aPositionX - Determines upper left corner
 * @param aPositionY - Determines upper left corner
 */
void BDSlider::setPosition(int16_t aPositionX, int16_t aPositionY) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalSliderPointer->setPosition(aPositionX, aPositionY);
#endif
    sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 4, mSliderHandle, SUBFUNCTION_SLIDER_SET_POSITION, aPositionX, aPositionY);
}

/*
 * Sets slider to active, draws border, bar, caption and value
 */
void BDSlider::drawSlider() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalSliderPointer->drawSlider();
#endif
    sendUSARTArgs(FUNCTION_SLIDER_DRAW, 1, mSliderHandle);
}

void BDSlider::drawBorder() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalSliderPointer->drawBorder();
#endif
    sendUSARTArgs(FUNCTION_SLIDER_DRAW_BORDER, 1, mSliderHandle);
}

void BDSlider::setValue(int16_t aCurrentValue) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalSliderPointer->setValueAndDrawBar(aCurrentValue);
#endif
    sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, SUBFUNCTION_SLIDER_SET_VALUE, aCurrentValue);
}

/*
 * Draw bar and set and print current value
 */
void BDSlider::setValueAndDrawBar(int16_t aCurrentValue) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalSliderPointer->setValueAndDrawBar(aCurrentValue);
#endif
    sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, SUBFUNCTION_SLIDER_SET_VALUE_AND_DRAW_BAR, aCurrentValue);
}

/*
 * Draw bar and set and print current value
 */
void BDSlider::setValue(int16_t aCurrentValue, bool doDrawBar) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalSliderPointer->setValueAndDrawBar(aCurrentValue);
#endif
    if (USART_isBluetoothPaired()) {
        uint8_t tSubFunctionCode = SUBFUNCTION_SLIDER_SET_VALUE;
        if (doDrawBar) {
            tSubFunctionCode = SUBFUNCTION_SLIDER_SET_VALUE_AND_DRAW_BAR;
        }
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, tSubFunctionCode, aCurrentValue);
    }
}

void BDSlider::setBarThresholdColor(color16_t aBarThresholdColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalSliderPointer->setBarThresholdColor(aBarThresholdColor);
#endif
    sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, SUBFUNCTION_SLIDER_SET_COLOR_THRESHOLD, aBarThresholdColor);
}

/*
 * Default threshold color is COLOR16_RED initially
 */
void BDSlider::setBarThresholdDefaultColor(color16_t aBarThresholdDefaultColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalSliderPointer->setBarThresholdColor(aBarThresholdDefaultColor);
#endif
    sendUSARTArgs(FUNCTION_SLIDER_GLOBAL_SETTINGS, 2, SUBFUNCTION_SLIDER_SET_DEFAULT_COLOR_THRESHOLD, aBarThresholdDefaultColor);
}

void BDSlider::setBarBackgroundColor(color16_t aBarBackgroundColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalSliderPointer->setBarBackgroundColor(aBarBackgroundColor);
#endif
    sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, SUBFUNCTION_SLIDER_SET_COLOR_BAR_BACKGROUND, aBarBackgroundColor);
}

/*
 * Default values are ((BlueDisplay1.mReferenceDisplaySize.YHeight / 12), (FLAG_SLIDER_CAPTION_ALIGN_MIDDLE | FLAG_SLIDER_CAPTION_ABOVE),
 *                      (BlueDisplay1.mReferenceDisplaySize.YHeight / 40), COLOR16_BLACK, COLOR16_WHITE);
 */
void BDSlider::setCaptionProperties(uint8_t aCaptionSize, uint8_t aCaptionPosition, uint8_t aCaptionMargin, color16_t aCaptionColor,
        color16_t aCaptionBackgroundColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalSliderPointer->setCaptionColors(aCaptionColor, aCaptionBackgroundColor);
#endif
    sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 7, mSliderHandle, SUBFUNCTION_SLIDER_SET_CAPTION_PROPERTIES, aCaptionSize,
            aCaptionPosition, aCaptionMargin, aCaptionColor, aCaptionBackgroundColor);
}

void BDSlider::setCaption(const char *aCaption) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalSliderPointer->setCaption(aCaption);
#endif
    sendUSARTArgsAndByteBuffer(FUNCTION_SLIDER_SET_CAPTION, 1, mSliderHandle, strlen(aCaption), aCaption);
}

/*
 * Sets the unit behind the value e.g. cm or mph
 * This unit string is always appended to the value string.
 */
void BDSlider::setValueUnitString(const char *aValueUnitString) {
    sendUSARTArgsAndByteBuffer(FUNCTION_SLIDER_SET_VALUE_UNIT_STRING, 1, mSliderHandle, strlen(aValueUnitString), aValueUnitString);
}

/*
 * This format string is used in (Java) String.format(mValueFormatString, mCurrentValue) which uses the printf specs.
 * Default is "%2d" for a slider with virtual slider length from 10 to 99 and "%3d" for length 100 to 999.
 */
void BDSlider::setValueFormatString(const char *aValueFormatString) {
    sendUSARTArgsAndByteBuffer(FUNCTION_SLIDER_SET_VALUE_FORMAT_STRING, 1, mSliderHandle, strlen(aValueFormatString),
            aValueFormatString);
}

/*
 * Default values are ((BlueDisplay1.mReferenceDisplaySize.YHeight / 20), (FLAG_SLIDER_CAPTION_ALIGN_MIDDLE | FLAG_SLIDER_CAPTION_BELOW),
 *                      (BlueDisplay1.mReferenceDisplaySize.YHeight / 40), COLOR16_BLACK, COLOR16_WHITE);
 */
void BDSlider::setPrintValueProperties(uint8_t aPrintValueTextSize, uint8_t aPrintValuePosition, uint8_t aPrintValueMargin,
        color16_t aPrintValueColor, color16_t aPrintValueBackgroundColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalSliderPointer->setValueStringColors(aPrintValueColor, aPrintValueBackgroundColor);
#endif
    sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 7, mSliderHandle, SUBFUNCTION_SLIDER_SET_VALUE_STRING_PROPERTIES, aPrintValueTextSize,
            aPrintValuePosition, aPrintValueMargin, aPrintValueColor, aPrintValueBackgroundColor);
}

/*
 * Scale factor of 2 means, that the slider is virtually 2 times larger than displayed.
 * => values were divided by scale factor before displayed on real slider.
 * formula aScaleFactor = virtualLength / realLength
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
void BDSlider::setScaleFactor(float aScaleFactor) {
    long tScaleFactor = *reinterpret_cast<uint32_t*>(&aScaleFactor);
    sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 4, mSliderHandle, SUBFUNCTION_SLIDER_SET_SCALE_FACTOR, (uint16_t) tScaleFactor & 0XFFFF,
            (uint16_t) (tScaleFactor >> 16));
}

/*
 * The scale factor for displaying a slider value. 2 means, that values are multiplied by 2 before displayed on slider.
 * Better use the call to setScaleFactor(1/aScaleFactorValue);
 */
void BDSlider::setValueScaleFactor(float aScaleFactorValue) {
    if (USART_isBluetoothPaired()) {
        setScaleFactor(1 / aScaleFactorValue);
    }
}
#pragma GCC diagnostic pop

void BDSlider::printValue(const char *aValueString) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalSliderPointer->printValue(aValueString);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_SLIDER_PRINT_VALUE, 1, mSliderHandle, strlen(aValueString), aValueString);
}

void BDSlider::activate() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalSliderPointer->activate();
#endif
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 2, mSliderHandle, SUBFUNCTION_SLIDER_SET_ACTIVE);
}

void BDSlider::deactivate() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalSliderPointer->deactivate();
#endif
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 2, mSliderHandle, SUBFUNCTION_SLIDER_RESET_ACTIVE);
}

#if defined(SUPPORT_LOCAL_DISPLAY)

int BDSlider::printValue() {
    return mLocalSliderPointer->printValue();
}

void BDSlider::setXOffsetValue(int16_t aXOffsetValue) {
    mLocalSliderPointer->setXOffsetValue(aXOffsetValue);
}

int16_t BDSlider::getCurrentValue() const {
    return mLocalSliderPointer->getCurrentValue();
}

uint16_t BDSlider::getPositionXRight() const {
    return mLocalSliderPointer->getPositionXRight();
}

uint16_t BDSlider::getPositionYBottom() const {
    return mLocalSliderPointer->getPositionYBottom();
}
#endif

/*
 * Static functions
 */
void BDSlider::resetAllSliders() {
    sLocalSliderIndex = 0;
}

void BDSlider::activateAllSliders() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalTouchSlider::activateAllSliders();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_ACTIVATE_ALL, 0);
    }
}

void BDSlider::deactivateAllSliders() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalTouchSlider::deactivateAllSliders();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_DEACTIVATE_ALL, 0);
    }
}

/*
 * Utility functions
 */

void initPositiveNegativeSliders(struct positiveNegativeSlider *aSliderStructPtr, BDSlider *aPositiveSliderPtr,
        BDSlider *aNegativeSliderPtr) {
    aSliderStructPtr->positiveSliderPtr = aPositiveSliderPtr;
    aSliderStructPtr->negativeSliderPtr = aNegativeSliderPtr;
}

/*
 * @param aValue positive for bar in *positiveSliderPtr
 * @return aValue with aSliderDeadBand applied, i.e. subtract dead band from value but clip at zero
 *                              for negative values, add dead band to value and clip at zero
 */
int setPositiveNegativeSliders(struct positiveNegativeSlider *aSliderStructPtr, int aValue, uint8_t aSliderDeadBand) {
    BDSlider *tValueSlider = aSliderStructPtr->positiveSliderPtr;
    BDSlider *tZeroSlider = aSliderStructPtr->negativeSliderPtr;
    bool tSliderValueIsPositive = true;
    if (aValue < 0) {
        aValue = -aValue;
        tValueSlider = tZeroSlider;
        tZeroSlider = aSliderStructPtr->positiveSliderPtr;
        tSliderValueIsPositive = false;
    }

    /*
     * Now we have a positive value for dead band and slider length
     */
    if (aValue > aSliderDeadBand) {
        // dead band subtraction -> resulting values start at 0
        aValue -= aSliderDeadBand;
    } else {
        aValue = 0;
    }

    /*
     * Draw slider value if values changed
     */
    if (aSliderStructPtr->lastSliderValue != (unsigned int) aValue) {
        aSliderStructPtr->lastSliderValue = aValue;
        tValueSlider->setValueAndDrawBar(aValue);

        if (tSliderValueIsPositive != aSliderStructPtr->lastSliderValueWasPositive) {
            aSliderStructPtr->lastSliderValueWasPositive = tSliderValueIsPositive;
            // the sign has changed, clear old value
            tZeroSlider->setValueAndDrawBar(0);
        }
    }

    /*
     * Restore sign for aValue with dead band applied
     */
    if (tZeroSlider == aSliderStructPtr->positiveSliderPtr) {
        aValue = -aValue;
    }
    return aValue;
}

unsigned int setPositiveNegativeSliders(struct positiveNegativeSlider *aSliderStructPtr, unsigned int aValue, bool aPositiveSider,
        uint8_t aSliderDeadBand) {

    /*
     * We have a positive value for dead band and slider length
     */
    if (aValue > aSliderDeadBand) {
        // dead band subtraction -> resulting values start at 0
        aValue -= aSliderDeadBand;
    } else {
        aValue = 0;
    }

    BDSlider *tValueSlider = aSliderStructPtr->positiveSliderPtr; // the slider which shows the value
    BDSlider *tZeroSlider = aSliderStructPtr->negativeSliderPtr;  // The slider which is cleared
    if (!aPositiveSider) {
        tValueSlider = aSliderStructPtr->negativeSliderPtr;
        tZeroSlider = aSliderStructPtr->positiveSliderPtr;
    }

    if (aPositiveSider == aSliderStructPtr->lastSliderValueWasPositive) {
        // No direction change, only value change possible here
        if (aSliderStructPtr->lastSliderValue != aValue) {
            // value changed here
            tValueSlider->setValueAndDrawBar(aValue);
        }
    } else {
        // direction/slider change
        tZeroSlider->setValueAndDrawBar(0);
        tValueSlider->setValueAndDrawBar(aValue);
    }

    aSliderStructPtr->lastSliderValueWasPositive = aPositiveSider;
    aSliderStructPtr->lastSliderValue = aValue;

    return aValue;
}
#endif //_BDSLIDER_HPP
