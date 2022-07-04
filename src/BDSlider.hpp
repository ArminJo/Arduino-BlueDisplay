/*
 * BDSlider.hpp
 *
 * Implementation of the Slider client stub for the Android BlueDisplay app.
 *
 *  SUMMARY
 *  Blue Display is an Open Source Android remote Display for Arduino etc.
 *  It receives basic draw requests from Arduino etc. over Bluetooth and renders it.
 *  It also implements basic GUI elements as buttons and sliders.
 *  GUI callback, touch and sensor events are sent back to Arduino.
 *
 *  Copyright (C) 2015-2022  Armin Joachimsmeyer
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
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#ifndef _BDSLIDER_HPP
#define _BDSLIDER_HPP

#include "BDSlider.h"
#include "BlueDisplayProtocol.h"
#include "BlueSerial.h"

#include <string.h>  // for strlen

BDSliderHandle_t sLocalSliderIndex = 0;

BDSlider::BDSlider(void) { // @suppress("Class members should be properly initialized")
}

#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
BDSlider::BDSlider(BDSliderHandle_t aSliderHandle, TouchSlider *aLocalSliderPointer) {
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
 * @param aOptions - See #FLAG_SLIDER_SHOW_BORDER etc.
 * @param aOnChangeHandler - If NULL no update of bar is done on touch - equivalent to FLAG_SLIDER_IS_ONLY_OUTPUT
 */
void BDSlider::init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aBarWidth, int16_t aBarLength, int16_t aThresholdValue,
        int16_t aInitalValue, color16_t aSliderColor, color16_t aBarColor, uint8_t aFlags,
        void (*aOnChangeHandler)(BDSlider*, uint16_t)) {
    BDSliderHandle_t tSliderNumber = sLocalSliderIndex++;

    if (USART_isBluetoothPaired()) {
#if __SIZEOF_POINTER__ == 4
        sendUSARTArgs(FUNCTION_SLIDER_CREATE, 12, tSliderNumber, aPositionX, aPositionY, aBarWidth, aBarLength,
                aThresholdValue, aInitalValue, aSliderColor, aBarColor, aFlags, aOnChangeHandler,
                (reinterpret_cast<uint32_t>(aOnChangeHandler) >> 16));
#else
        sendUSARTArgs(FUNCTION_SLIDER_CREATE, 11, tSliderNumber, aPositionX, aPositionY, aBarWidth, aBarLength, aThresholdValue,
                aInitalValue, aSliderColor, aBarColor, aFlags, aOnChangeHandler);
#endif
    }
    mSliderHandle = tSliderNumber;

#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer = new TouchSlider();
    // Cast required here. At runtime the right pointer is returned because of FLAG_USE_INDEX_FOR_CALLBACK
    mLocalSliderPointer->initSlider(aPositionX, aPositionY, aBarWidth, aBarLength, aThresholdValue, aInitalValue,
            aSliderColor, aBarColor, aFlags | FLAG_USE_BDSLIDER_FOR_CALLBACK,
            reinterpret_cast<void (*)(TouchSlider *, uint16_t)>( aOnChangeHandler));
    // keep the formatting
    mLocalSliderPointer ->mBDSliderPtr = this;
#endif
}

#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
void BDSlider::deinit(void) {
    sLocalSliderIndex--;
    delete mLocalSliderPointer;
}
#endif

/**
 * @param aPositionX - Determines upper left corner
 * @param aPositionY - Determines upper left corner
 */
void BDSlider::setPosition(int16_t aPositionX, int16_t aPositionY) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->setPosition(aPositionX, aPositionY);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 4, mSliderHandle, SUBFUNCTION_SLIDER_SET_POSITION, aPositionX, aPositionY);
    }
}

/*
 * Sets slider to active, draws border, bar, caption and value
 */
void BDSlider::drawSlider(void) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->drawSlider();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_DRAW, 1, mSliderHandle);
    }
}

void BDSlider::drawBorder(void) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->drawBorder();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_DRAW_BORDER, 1, mSliderHandle);
    }
}

void BDSlider::setValue(int16_t aCurrentValue) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->setValueAndDrawBar(aCurrentValue);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, SUBFUNCTION_SLIDER_SET_VALUE, aCurrentValue);
    }
}

/*
 * Draw bar and set and print current value
 */
void BDSlider::setValueAndDrawBar(int16_t aCurrentValue) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->setValueAndDrawBar(aCurrentValue);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, SUBFUNCTION_SLIDER_SET_VALUE_AND_DRAW_BAR, aCurrentValue);
    }
}

/*
 * Draw bar and set and print current value
 */
void BDSlider::setValue(int16_t aCurrentValue, bool doDrawBar) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->setValueAndDrawBar(aCurrentValue);
#endif
    uint8_t tSubFunctionCode = SUBFUNCTION_SLIDER_SET_VALUE;
    if (doDrawBar) {
        tSubFunctionCode = SUBFUNCTION_SLIDER_SET_VALUE_AND_DRAW_BAR;
    }
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, tSubFunctionCode, aCurrentValue);
    }
}

/*
 * Deprecated
 */
void BDSlider::setActualValue(int16_t aCurrentValue) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->setValueAndDrawBar(aCurrentValue);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, SUBFUNCTION_SLIDER_SET_VALUE, aCurrentValue);
    }
}

/*
 * Deprecated
 */
void BDSlider::setActualValueAndDrawBar(int16_t aCurrentValue) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->setValueAndDrawBar(aCurrentValue);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, SUBFUNCTION_SLIDER_SET_VALUE_AND_DRAW_BAR, aCurrentValue);
    }
}

void BDSlider::setBarThresholdColor(color16_t aBarThresholdColor) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->setBarThresholdColor(aBarThresholdColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, SUBFUNCTION_SLIDER_SET_COLOR_THRESHOLD, aBarThresholdColor);
    }
}

/*
 * Default threshold color is COLOR16_RED initially
 */
void BDSlider::setBarThresholdDefaultColor(color16_t aBarThresholdDefaultColor) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->setBarThresholdColor(aBarThresholdDefaultColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_GLOBAL_SETTINGS, 2, SUBFUNCTION_SLIDER_SET_DEFAULT_COLOR_THRESHOLD,
                aBarThresholdDefaultColor);
    }
}

void BDSlider::setBarBackgroundColor(color16_t aBarBackgroundColor) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->setBarBackgroundColor(aBarBackgroundColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, SUBFUNCTION_SLIDER_SET_COLOR_BAR_BACKGROUND, aBarBackgroundColor);
    }
}

/*
 * Default values are ((BlueDisplay1.mReferenceDisplaySize.YHeight / 12), (FLAG_SLIDER_CAPTION_ALIGN_MIDDLE | FLAG_SLIDER_CAPTION_ABOVE),
 *                      (BlueDisplay1.mReferenceDisplaySize.YHeight / 40), COLOR16_BLACK, COLOR_WHITE);
 */
void BDSlider::setCaptionProperties(uint8_t aCaptionSize, uint8_t aCaptionPosition, uint8_t aCaptionMargin, color16_t aCaptionColor,
        color16_t aCaptionBackgroundColor) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->setCaptionColors(aCaptionColor, aCaptionBackgroundColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 7, mSliderHandle, SUBFUNCTION_SLIDER_SET_CAPTION_PROPERTIES, aCaptionSize,
                aCaptionPosition, aCaptionMargin, aCaptionColor, aCaptionBackgroundColor);
    }
}

void BDSlider::setCaption(const char *aCaption) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->setCaption(aCaption);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_SLIDER_SET_CAPTION, 1, mSliderHandle, strlen(aCaption), aCaption);
    }
}

/*
 * Sets the unit behind the value e.g. cm or mph
 * This unit string is always appended to the value string.
 */
void BDSlider::setValueUnitString(const char *aValueUnitString) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_SLIDER_SET_VALUE_UNIT_STRING, 1, mSliderHandle, strlen(aValueUnitString),
                aValueUnitString);
    }
}

/*
 * This format string is used in (Java) String.format(mValueFormatString, mCurrentValue) which uses the printf specs.
 * Default is "%2d" for a slider with virtual slider length from 10 to 99 and "%3d" for length 100 to 999.
 */
void BDSlider::setValueFormatString(const char *aValueFormatString) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_SLIDER_SET_VALUE_FORMAT_STRING, 1, mSliderHandle, strlen(aValueFormatString),
                aValueFormatString);
    }
}

/*
 * Default values are ((BlueDisplay1.mReferenceDisplaySize.YHeight / 20), (FLAG_SLIDER_CAPTION_ALIGN_MIDDLE | FLAG_SLIDER_CAPTION_BELOW),
 *                      (BlueDisplay1.mReferenceDisplaySize.YHeight / 40), COLOR16_BLACK, COLOR_WHITE);
 */
void BDSlider::setPrintValueProperties(uint8_t aPrintValueTextSize, uint8_t aPrintValuePosition, uint8_t aPrintValueMargin,
        color16_t aPrintValueColor, color16_t aPrintValueBackgroundColor) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->setValueStringColors(aPrintValueColor, aPrintValueBackgroundColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 7, mSliderHandle, SUBFUNCTION_SLIDER_SET_VALUE_STRING_PROPERTIES,
                aPrintValueTextSize, aPrintValuePosition, aPrintValueMargin, aPrintValueColor, aPrintValueBackgroundColor);
    }
}

/*
 * Scale factor of 2 means, that the slider is virtually 2 times larger than displayed.
 * => values were divided by scale factor before displayed on real slider.
 * formula aScaleFactor = virtualLength / realLength
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
void BDSlider::setScaleFactor(float aScaleFactor) {
    if (USART_isBluetoothPaired()) {
        long tScaleFactor = *reinterpret_cast<uint32_t*>(&aScaleFactor);
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 4, mSliderHandle, SUBFUNCTION_SLIDER_SET_SCALE_FACTOR,
                (uint16_t) tScaleFactor & 0XFFFF, (uint16_t) (tScaleFactor >> 16));
    }
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
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->printValue(aValueString);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_SLIDER_PRINT_VALUE, 1, mSliderHandle, strlen(aValueString), aValueString);
    }
}

void BDSlider::activate(void) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->activate();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 2, mSliderHandle, SUBFUNCTION_SLIDER_SET_ACTIVE);
    }
}

void BDSlider::deactivate(void) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalSliderPointer->deactivate();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 2, mSliderHandle, SUBFUNCTION_SLIDER_RESET_ACTIVE);
    }
}

#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)

int BDSlider::printValue(void) {
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
void BDSlider::resetAllSliders(void) {
    sLocalSliderIndex = 0;
}

void BDSlider::activateAllSliders(void) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    TouchSlider::activateAllSliders();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_ACTIVATE_ALL, 0);
    }
}

void BDSlider::deactivateAllSliders(void) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    TouchSlider::deactivateAllSliders();
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
