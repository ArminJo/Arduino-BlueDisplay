/*
 * BDButton.hpp
 *
 * Implementation of the Button client stub for the Android BlueDisplay app.
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

#ifndef _BDBUTTON_HPP
#define _BDBUTTON_HPP

#include "BDButton.h"
#include "BlueDisplay.h" // for BUTTONS_SET_BEEP_TONE

#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
#include "TouchButtonAutorepeat.h"
#endif

#include <string.h>  // for strlen

/*
 * Can be interpreted as pointer to button stack.
 */
BDButtonHandle_t sLocalButtonIndex = 0;

BDButton::BDButton(void) { // @suppress("Class members should be properly initialized")
}

BDButton::BDButton(BDButtonHandle_t aButtonHandle) { // @suppress("Class members should be properly initialized")
    mButtonHandle = aButtonHandle;
}

#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
BDButton::BDButton(BDButtonHandle_t aButtonHandle, TouchButton *aLocalButtonPtr) {
    mButtonHandle = aButtonHandle;
    mLocalButtonPtr = aLocalButtonPtr;
}
#endif

BDButton::BDButton(BDButton const &aButton) {
    mButtonHandle = aButton.mButtonHandle;
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalButtonPtr = aButton.mLocalButtonPtr;
#endif
}

bool BDButton::operator==(const BDButton &aButton) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    return (mButtonHandle == aButton.mButtonHandle && mLocalButtonPtr == aButton.mLocalButtonPtr);
#else
    return (mButtonHandle == aButton.mButtonHandle);
#endif
}

bool BDButton::operator!=(const BDButton &aButton) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    return (mButtonHandle != aButton.mButtonHandle || mLocalButtonPtr != aButton.mLocalButtonPtr);
#else
    return (mButtonHandle != aButton.mButtonHandle);
#endif
}

/*
 * initialize a button stub
 * If local display is attached, allocate a button from the local pool, so do not forget to call deinit()
 * Caption is value for false (0) if FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN is set
 * Multi-line caption has \n as line separator.
 */
void BDButton::init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor,
        const char *aCaption, uint16_t aCaptionSize, uint8_t aFlags, int16_t aValue, void (*aOnTouchHandler)(BDButton*, int16_t)) {

    BDButtonHandle_t tButtonNumber = sLocalButtonIndex++;
    if (USART_isBluetoothPaired()) {
#if __SIZEOF_POINTER__ == 4
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 11, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
                aButtonColor, aCaptionSize, aFlags, aValue, aOnTouchHandler, (reinterpret_cast<uint32_t>(aOnTouchHandler) >> 16),
                strlen(aCaption), aCaption);
#else
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 10, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
                aButtonColor, aCaptionSize, aFlags, aValue, aOnTouchHandler, strlen(aCaption), aCaption);
#endif
    }
    mButtonHandle = tButtonNumber;
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    if (aFlags & FLAG_BUTTON_TYPE_AUTOREPEAT) {
        mLocalButtonPtr = new TouchButtonAutorepeat();
    } else {
        mLocalButtonPtr = new TouchButton();
    }
    // Cast required here. At runtime the right pointer is returned because of FLAG_USE_BDBUTTON_FOR_CALLBACK
    mLocalButtonPtr->initButton(aPositionX, aPositionY, aWidthX, aHeightY, aButtonColor, aCaption, aCaptionSize,
            aFlags | FLAG_USE_BDBUTTON_FOR_CALLBACK, aValue, reinterpret_cast<void (*)(TouchButton*, int16_t)> (aOnTouchHandler));

    mLocalButtonPtr->mBDButtonPtr = this;
#endif
}

#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
/*
 * Assume a button stack, e.g. only local buttons are deinitialized which were initialized last.
 * localButtonIndex is used as stack pointer.
 */
void BDButton::deinit(void) {
    sLocalButtonIndex--;
    delete mLocalButtonPtr;
}
#endif

void BDButton::drawButton(void) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    if (mLocalButtonPtr != NULL) {
        mLocalButtonPtr->drawButton();
    }
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_DRAW, 1, mButtonHandle);
    }
}

void BDButton::removeButton(color16_t aBackgroundColor) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalButtonPtr->removeButton(aBackgroundColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_REMOVE, 2, mButtonHandle, aBackgroundColor);
    }
}

void BDButton::drawCaption(void) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalButtonPtr->drawCaption();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_DRAW_CAPTION, 1, mButtonHandle);
    }
}
//
//void BDButton::setCaption(const char *aCaption) {
//#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
//    mLocalButtonPtr->setCaption(aCaption);
//#endif
//    if (USART_isBluetoothPaired()) {
//        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_SET_CAPTION, 1, mButtonHandle, strlen(aCaption), aCaption);
//    }
//}

/*
 * Sets caption for value true (green button) if different from default false (red button) caption
 */
void BDButton::setCaptionForValueTrue(const char *aCaption) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    // not supported
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_SET_CAPTION_FOR_VALUE_TRUE, 1, mButtonHandle, strlen(aCaption), aCaption);
    }
}

void BDButton::setCaption(const char *aCaption, bool doDrawButton) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalButtonPtr->setCaption(aCaption);
    if (doDrawButton) {
        mLocalButtonPtr->drawButton();
    }
#endif
    if (USART_isBluetoothPaired()) {
        uint8_t tFunctionCode = FUNCTION_BUTTON_SET_CAPTION;
        if (doDrawButton) {
            tFunctionCode = FUNCTION_BUTTON_SET_CAPTION_AND_DRAW_BUTTON;
        }
        sendUSARTArgsAndByteBuffer(tFunctionCode, 1, mButtonHandle, strlen(aCaption), aCaption);
    }
}

void BDButton::setCaptionFromStringArray(const char *const aCaptionStringArrayPtr[], uint8_t aStringIndex, bool doDrawButton) {
    setCaption(aCaptionStringArrayPtr[aStringIndex], doDrawButton);
}

void BDButton::setValue(int16_t aValue, bool doDrawButton) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalButtonPtr->setValue(aValue);
    if (doDrawButton) {
        mLocalButtonPtr->drawButton();
    }
#endif
    uint8_t tSubFunctionCode = SUBFUNCTION_BUTTON_SET_VALUE;
    if (doDrawButton) {
        tSubFunctionCode = SUBFUNCTION_BUTTON_SET_VALUE_AND_DRAW;
    }
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonHandle, tSubFunctionCode, aValue);
    }
}

void BDButton::setValueAndDraw(int16_t aValue) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalButtonPtr->setValue(aValue);
    mLocalButtonPtr->drawButton();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonHandle, SUBFUNCTION_BUTTON_SET_VALUE_AND_DRAW, aValue);
    }
}

void BDButton::setButtonColor(color16_t aButtonColor) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalButtonPtr->setButtonColor(aButtonColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonHandle, SUBFUNCTION_BUTTON_SET_BUTTON_COLOR, aButtonColor);
    }
}

void BDButton::setButtonColorAndDraw(color16_t aButtonColor) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalButtonPtr->setButtonColor(aButtonColor);
    mLocalButtonPtr->drawButton();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonHandle, SUBFUNCTION_BUTTON_SET_BUTTON_COLOR_AND_DRAW, aButtonColor);
    }
}

void BDButton::setPosition(int16_t aPositionX, int16_t aPositionY) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalButtonPtr->setPosition(aPositionX, aPositionY);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 4, mButtonHandle, SUBFUNCTION_BUTTON_SET_POSITION, aPositionX, aPositionY);
    }
}

/**
 * after aMillisFirstDelay milliseconds a callback is done every aMillisFirstRate milliseconds for aFirstCount times
 * after this a callback is done every aMillisSecondRate milliseconds
 */
void BDButton::setButtonAutorepeatTiming(uint16_t aMillisFirstDelay, uint16_t aMillisFirstRate, uint16_t aFirstCount,
        uint16_t aMillisSecondRate) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
//    ((TouchButtonAutorepeat*) mLocalButtonPtr)->setButtonAutorepeatTiming(aMillisFirstDelay, aMillisFirstRate, aFirstCount,
//            aMillisSecondRate);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 7, mButtonHandle, SUBFUNCTION_BUTTON_SET_AUTOREPEAT_TIMING, aMillisFirstDelay,
                aMillisFirstRate, aFirstCount, aMillisSecondRate);
    }
}

void BDButton::activate(void) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalButtonPtr->activate();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 2, mButtonHandle, SUBFUNCTION_BUTTON_SET_ACTIVE);
    }
}

void BDButton::deactivate(void) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    mLocalButtonPtr->deactivate();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 2, mButtonHandle, SUBFUNCTION_BUTTON_RESET_ACTIVE);
    }
}

/*
 * Static functions
 */
void BDButton::resetAllButtons(void) {
    sLocalButtonIndex = 0;
}

void BDButton::setGlobalFlags(uint16_t aFlags) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_GLOBAL_SETTINGS, 1, aFlags);
    }
}

/*
 * aToneVolume: value in percent
 */
void BDButton::setButtonsTouchTone(uint8_t aToneIndex, uint16_t aToneDuration) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_GLOBAL_SETTINGS, 3, FLAG_BUTTON_GLOBAL_SET_BEEP_TONE, aToneIndex, aToneDuration);
    }
}

void BDButton::setButtonsTouchTone(uint8_t aToneIndex, uint16_t aToneDuration, uint8_t aToneVolume) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_GLOBAL_SETTINGS, 4, FLAG_BUTTON_GLOBAL_SET_BEEP_TONE, aToneIndex, aToneDuration, aToneVolume);
    }
}

void BDButton::activateAllButtons(void) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_ACTIVATE_ALL, 0);
    }
}

void BDButton::deactivateAllButtons(void) {
#if defined(BD_DRAW_TO_LOCAL_DISPLAY_TOO)
    TouchButton::deactivateAllButtons();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_DEACTIVATE_ALL, 0);
    }
}

#if defined(ARDUINO)
// this uses around 160 to 200 bytes initially and saves around 20 bytes per call.
//uint16_t sLastWidthX;
//uint16_t sLastHeightY;
//color16_t sLastButtonColor;
//uint8_t sLastCaptionSize;
//uint8_t sLastFlags;
//void BDButton::init(uint16_t aPositionX, uint16_t aPositionY, const __FlashStringHelper *aPGMCaption, int16_t aValue,
//        void (*aOnTouchHandler)(BDButton*, int16_t)) {
//
//    BDButtonHandle_t tButtonNumber = sLocalButtonIndex++;
//    PGM_P tPGMCaption = reinterpret_cast<PGM_P>(aPGMCaption);
//
//    if (USART_isBluetoothPaired()) {
//        uint8_t tCaptionLength = strlen_P(tPGMCaption);
//        if (tCaptionLength > STRING_BUFFER_STACK_SIZE) {
//            tCaptionLength = STRING_BUFFER_STACK_SIZE;
//        }
//        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
//        strncpy_P(tStringBuffer, tPGMCaption, tCaptionLength);
//
//#if __SIZEOF_POINTER__ == 4
//        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 11, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
//                aButtonColor, aCaptionSize, aFlags, aValue, aOnTouchHandler, (reinterpret_cast<uint32_t>(aOnTouchHandler) >> 16),
//                tCaptionLength, tStringBuffer);
//#else
//        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 10, tButtonNumber, aPositionX, aPositionY, sLastWidthX, sLastHeightY,
//                sLastButtonColor, sLastCaptionSize, sLastFlags, aValue, aOnTouchHandler, tCaptionLength, tStringBuffer);
//#endif
//    }
//    mButtonHandle = tButtonNumber;
//}

uint8_t StringClipAndCopy(char *aStringBuffer, const __FlashStringHelper *aPGMCaption) {
    PGM_P tPGMCaption = reinterpret_cast<PGM_P>(aPGMCaption);
    /*
     * compute string length
     */
    uint8_t tLength = strlen_P(tPGMCaption);
    if (tLength > STRING_BUFFER_STACK_SIZE) {
        tLength = STRING_BUFFER_STACK_SIZE;
    }
    /*
     * copy string up to length
     */
    strncpy_P(aStringBuffer, tPGMCaption, tLength);
    return tLength;
}

// This uses around 200 bytes and saves 8 to 24 bytes per button
//void BDButton::init(const struct ButtonInit *aButtonInfo, const __FlashStringHelper *aPGMCaption ) {
//    init(aButtonInfo,aPGMCaption,pgm_read_word(aButtonInfo->Value));
//}
//
//void BDButton::init(const struct ButtonInit *aButtonInfo, const __FlashStringHelper *aPGMCaption, int16_t aValue ) {
//
//    BDButtonHandle_t tButtonNumber = sLocalButtonIndex++;
//    if (USART_isBluetoothPaired()) {
//
//        uint16_t tParamBuffer[MAX_NUMBER_OF_ARGS_FOR_BD_FUNCTIONS + 4];
//        uint16_t *tBufferPointer = &tParamBuffer[0];
//        *tBufferPointer++ = FUNCTION_BUTTON_CREATE << 8 | SYNC_TOKEN; // add sync token
//        *tBufferPointer++ = 10 * 2;
//        *tBufferPointer++ = tButtonNumber;
//        memcpy_P(tBufferPointer, aButtonInfo, sizeof(ButtonInit));
//        tParamBuffer[10] = aValue;
//
//        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
//        uint8_t tLength = StringClipAndCopy(tStringBuffer, aPGMCaption);
//
//        // add data field header
//        tBufferPointer += sizeof(ButtonInit) / sizeof(uint16_t);
//        *tBufferPointer++ = DATAFIELD_TAG_BYTE << 8 | SYNC_TOKEN; // start new transmission block
//        *tBufferPointer = tLength;
//
//        sendUSARTBufferNoSizeCheck(reinterpret_cast<uint8_t*>(&tParamBuffer[0]), 10 * 2 + 8,
//                reinterpret_cast<uint8_t*>(&tStringBuffer[0]), tLength);
//    }
//    mButtonHandle = tButtonNumber;
//}

void BDButton::init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor,
        const __FlashStringHelper *aPGMCaption, uint8_t aCaptionSize, uint8_t aFlags, int16_t aValue,
        void (*aOnTouchHandler)(BDButton*, int16_t)) {

    BDButtonHandle_t tButtonNumber = sLocalButtonIndex++;

    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        uint8_t tCaptionLength = StringClipAndCopy(tStringBuffer, aPGMCaption);

#if __SIZEOF_POINTER__ == 4
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 11, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
                aButtonColor, aCaptionSize, aFlags, aValue, aOnTouchHandler, (reinterpret_cast<uint32_t>(aOnTouchHandler) >> 16),
                tCaptionLength, tStringBuffer);
#else
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 10, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
                aButtonColor, aCaptionSize, aFlags, aValue, aOnTouchHandler, tCaptionLength, tStringBuffer);
#endif
    }
    mButtonHandle = tButtonNumber;
}

/*
 * Sets caption for value true (green button) if different from false (red button) caption
 */

void BDButton::setCaptionForValueTrue(const __FlashStringHelper *aPGMCaption) {

    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        uint8_t tCaptionLength = StringClipAndCopy(tStringBuffer, aPGMCaption);
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_SET_CAPTION_FOR_VALUE_TRUE, 1, mButtonHandle, tCaptionLength, tStringBuffer);
    }
}

void BDButton::setCaptionPGMForValueTrue(const char *aPGMCaption) {
    setCaptionForValueTrue((const __FlashStringHelper*) aPGMCaption);

}

void BDButton::setCaptionFromStringArrayPGM(const char *const aPGMCaptionStringArrayPtr[], uint8_t aStringIndex,
        bool doDrawButton) {
#if defined(AVR)
    __FlashStringHelper *tPGMCaption = (__FlashStringHelper*) pgm_read_word(&aPGMCaptionStringArrayPtr[aStringIndex]);
#else
    const char *tPGMCaption = aPGMCaptionStringArrayPtr[aStringIndex];
#endif
    setCaption(tPGMCaption, doDrawButton);
}

void BDButton::setCaptionPGM(const char *aPGMCaption, bool doDrawButton) {
    setCaption(reinterpret_cast<const __FlashStringHelper*>(aPGMCaption), doDrawButton);

}

/*
 * sets only caption
 */
void BDButton::setCaption(const __FlashStringHelper *aPGMCaption, bool doDrawButton) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        uint8_t tCaptionLength = StringClipAndCopy(tStringBuffer, aPGMCaption);
        uint8_t tFunctionCode = FUNCTION_BUTTON_SET_CAPTION;
        if (doDrawButton) {
            tFunctionCode = FUNCTION_BUTTON_SET_CAPTION_AND_DRAW_BUTTON;
        }
        sendUSARTArgsAndByteBuffer(tFunctionCode, 1, mButtonHandle, tCaptionLength, tStringBuffer);
    }
}

#endif // defined(ARDUINO)

#endif //_BDBUTTON_H
