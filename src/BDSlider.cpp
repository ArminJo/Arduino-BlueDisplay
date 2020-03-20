/*
 * BDSlider.cpp
 *
 * Implementation of the Slider client stub for the Android BlueDisplay app.
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

#include "BDSlider.h"
#include "BlueDisplayProtocol.h"
#include "BlueSerial.h"

#include <string.h>  // for strlen

BDSliderHandle_t sLocalSliderIndex = 0;

BDSlider::BDSlider(void) { // @suppress("Class members should be properly initialized")
}

#ifdef LOCAL_DISPLAY_EXISTS
BDSlider::BDSlider(BDSliderHandle_t aSliderHandle, TouchSlider * aLocalSliderPointer) {
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
 * @param aBarLength - Size of slider bar in pixel = maximum slider value - no scaling!
 * @param aThresholdValue - Scaling! If selected or sent value is bigger, then color of bar changes from BarColor to BarBackgroundColor
 * @param aInitalValue - Scaling!
 * @param aSliderColor - Color of slider border. If no border specified, then bar background color.
 * @param aBarColor
 * @param aOptions - See #FLAG_SLIDER_SHOW_BORDER etc.
 * @param aOnChangeHandler - If NULL no update of bar is done on touch - equivalent to FLAG_SLIDER_IS_ONLY_OUTPUT
 */
void BDSlider::init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aBarWidth, int16_t aBarLength, int16_t aThresholdValue,
        int16_t aInitalValue, color16_t aSliderColor, color16_t aBarColor, uint8_t aFlags,
        void (*aOnChangeHandler)(BDSlider *, uint16_t)) {
    BDSliderHandle_t tSliderNumber = sLocalSliderIndex++;

    if (USART_isBluetoothPaired()) {
#ifndef AVR
        sendUSARTArgs(FUNCTION_SLIDER_CREATE, 12, tSliderNumber, aPositionX, aPositionY, aBarWidth, aBarLength,
                aThresholdValue, aInitalValue, aSliderColor, aBarColor, aFlags, aOnChangeHandler,
                (reinterpret_cast<uint32_t>(aOnChangeHandler) >> 16));
#else
        sendUSARTArgs(FUNCTION_SLIDER_CREATE, 11, tSliderNumber, aPositionX, aPositionY, aBarWidth, aBarLength, aThresholdValue,
                aInitalValue, aSliderColor, aBarColor, aFlags, aOnChangeHandler);
#endif
    }
    mSliderHandle = tSliderNumber;

#ifdef LOCAL_DISPLAY_EXISTS
    mLocalSliderPointer = new TouchSlider();
    // Cast required here. At runtime the right pointer is returned because of FLAG_USE_INDEX_FOR_CALLBACK
    mLocalSliderPointer->initSlider(aPositionX, aPositionY, aBarWidth, aBarLength, aThresholdValue, aInitalValue,
            aSliderColor, aBarColor, aFlags | FLAG_USE_BDSLIDER_FOR_CALLBACK,
            reinterpret_cast<void (*)(TouchSlider *, uint16_t)>( aOnChangeHandler));
    // keep the formatting
    mLocalSliderPointer ->mBDSliderPtr = this;
#endif
}

#ifdef LOCAL_DISPLAY_EXISTS
void BDSlider::deinit(void) {
    sLocalSliderIndex--;
    delete mLocalSliderPointer;
}
#endif

void BDSlider::drawSlider(void) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalSliderPointer->drawSlider();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_DRAW, 1, mSliderHandle);
    }
}

void BDSlider::drawBorder(void) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalSliderPointer->drawBorder();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_DRAW_BORDER, 1, mSliderHandle);
    }
}

void BDSlider::setValue(int16_t aCurrentValue) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalSliderPointer->setCurrentValueAndDrawBar(aCurrentValue);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, SUBFUNCTION_SLIDER_SET_VALUE, aCurrentValue);
    }
}

/*
 * Deprecated
 */
void BDSlider::setActualValue(int16_t aCurrentValue) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalSliderPointer->setCurrentValueAndDrawBar(aCurrentValue);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, SUBFUNCTION_SLIDER_SET_VALUE, aCurrentValue);
    }
}

void BDSlider::setValueAndDrawBar(int16_t aCurrentValue) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalSliderPointer->setCurrentValueAndDrawBar(aCurrentValue);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, SUBFUNCTION_SLIDER_SET_VALUE_AND_DRAW_BAR, aCurrentValue);
    }
}

/*
 * Deprecated
 */
void BDSlider::setActualValueAndDrawBar(int16_t aCurrentValue) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalSliderPointer->setCurrentValueAndDrawBar(aCurrentValue);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, SUBFUNCTION_SLIDER_SET_VALUE_AND_DRAW_BAR, aCurrentValue);
    }
}

void BDSlider::setBarThresholdColor(color16_t aBarThresholdColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalSliderPointer->setBarThresholdColor(aBarThresholdColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, SUBFUNCTION_SLIDER_SET_COLOR_THRESHOLD, aBarThresholdColor);
    }
}

void BDSlider::setBarBackgroundColor(color16_t aBarBackgroundColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalSliderPointer->setBarBackgroundColor(aBarBackgroundColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, mSliderHandle, SUBFUNCTION_SLIDER_SET_COLOR_BAR_BACKGROUND, aBarBackgroundColor);
    }
}

/*
 * Default values are ((BlueDisplay1.mReferenceDisplaySize.YHeight / 12), (FLAG_SLIDER_CAPTION_ALIGN_MIDDLE | FLAG_SLIDER_CAPTION_ABOVE),
 *                      (BlueDisplay1.mReferenceDisplaySize.YHeight / 40), COLOR_BLACK, COLOR_WHITE);
 */
void BDSlider::setCaptionProperties(uint8_t aCaptionSize, uint8_t aCaptionPosition, uint8_t aCaptionMargin, color16_t aCaptionColor,
        color16_t aCaptionBackgroundColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalSliderPointer->setCaptionColors(aCaptionColor, aCaptionBackgroundColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 7, mSliderHandle, SUBFUNCTION_SLIDER_SET_CAPTION_PROPERTIES, aCaptionSize,
                aCaptionPosition, aCaptionMargin, aCaptionColor, aCaptionBackgroundColor);
    }
}

void BDSlider::setCaption(const char * aCaption) {
#ifdef LOCAL_DISPLAY_EXISTS
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
void BDSlider::setValueUnitString(const char * aValueUnitString) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_SLIDER_SET_VALUE_UNIT_STRING, 1, mSliderHandle, strlen(aValueUnitString),
                aValueUnitString);
    }
}

/*
 * This format string is used in (Java) String.format(mValueFormatString, mCurrentValue) which uses the printf specs.
 * Default is "%2d" for a slider with virtual slider length from 10 to 99 and "%3d" for length 100 to 999.
 */
void BDSlider::setValueFormatString(const char * aValueFormatString) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_SLIDER_SET_VALUE_FORMAT_STRING, 1, mSliderHandle, strlen(aValueFormatString),
                aValueFormatString);
    }
}

/*
 * Default values are ((BlueDisplay1.mReferenceDisplaySize.YHeight / 20), (FLAG_SLIDER_CAPTION_ALIGN_MIDDLE | FLAG_SLIDER_CAPTION_BELOW),
 *                      (BlueDisplay1.mReferenceDisplaySize.YHeight / 40), COLOR_BLACK, COLOR_WHITE);
 */
void BDSlider::setPrintValueProperties(uint8_t aPrintValueTextSize, uint8_t aPrintValuePosition, uint8_t aPrintValueMargin,
        color16_t aPrintValueColor, color16_t aPrintValueBackgroundColor) {
#ifdef LOCAL_DISPLAY_EXISTS
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

void BDSlider::printValue(const char * aValueString) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalSliderPointer->printValue(aValueString);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_SLIDER_PRINT_VALUE, 1, mSliderHandle, strlen(aValueString), aValueString);
    }
}

void BDSlider::activate(void) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalSliderPointer->activate();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 2, mSliderHandle, SUBFUNCTION_SLIDER_SET_ACTIVE);
    }
}

void BDSlider::deactivate(void) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalSliderPointer->deactivate();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 2, mSliderHandle, SUBFUNCTION_SLIDER_RESET_ACTIVE);
    }
}

#ifdef LOCAL_DISPLAY_EXISTS

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
#ifdef LOCAL_DISPLAY_EXISTS
    TouchSlider::activateAllSliders();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_ACTIVATE_ALL, 0);
    }
}

void BDSlider::deactivateAllSliders(void) {
#ifdef LOCAL_DISPLAY_EXISTS
    TouchSlider::deactivateAllSliders();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_DEACTIVATE_ALL, 0);
    }
}

