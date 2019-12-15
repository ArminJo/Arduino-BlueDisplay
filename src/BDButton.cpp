/*
 * BDButton.cpp
 *
 * Implementation of the Button client stub for the Android BlueDisplay app.
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

#include "BDButton.h"
#include "BlueDisplay.h" // for BUTTONS_SET_BEEP_TONE

#ifdef LOCAL_DISPLAY_EXISTS
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

#ifdef LOCAL_DISPLAY_EXISTS
BDButton::BDButton(BDButtonHandle_t aButtonHandle, TouchButton * aLocalButtonPtr) {
    mButtonHandle = aButtonHandle;
    mLocalButtonPtr = aLocalButtonPtr;
}
#endif

BDButton::BDButton(BDButton const &aButton) {
    mButtonHandle = aButton.mButtonHandle;
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalButtonPtr = aButton.mLocalButtonPtr;
#endif
}

bool BDButton::operator==(const BDButton &aButton) {
#ifdef LOCAL_DISPLAY_EXISTS
    return (mButtonHandle == aButton.mButtonHandle && mLocalButtonPtr == aButton.mLocalButtonPtr);
#else
    return (mButtonHandle == aButton.mButtonHandle);
#endif
}

bool BDButton::operator!=(const BDButton &aButton) {
#ifdef LOCAL_DISPLAY_EXISTS
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
        const char * aCaption, uint16_t aCaptionSize, uint8_t aFlags, int16_t aValue, void (*aOnTouchHandler)(BDButton*, int16_t)) {

    BDButtonHandle_t tButtonNumber = sLocalButtonIndex++;
    if (USART_isBluetoothPaired()) {
#ifndef AVR
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 11, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
                aButtonColor, aCaptionSize, aFlags, aValue, aOnTouchHandler, (reinterpret_cast<uint32_t>(aOnTouchHandler) >> 16),
                strlen(aCaption), aCaption);
#else
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 10, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
                aButtonColor, aCaptionSize, aFlags, aValue, aOnTouchHandler, strlen(aCaption), aCaption);
#endif
    }
    mButtonHandle = tButtonNumber;
#ifdef LOCAL_DISPLAY_EXISTS
    if (aFlags & FLAG_BUTTON_TYPE_AUTOREPEAT) {
        mLocalButtonPtr = new TouchButtonAutorepeat();
    } else {
        mLocalButtonPtr = new TouchButton();
    }
    // Cast needed here. At runtime the right pointer is returned because of FLAG_USE_BDBUTTON_FOR_CALLBACK
    mLocalButtonPtr->initButton(aPositionX, aPositionY, aWidthX, aHeightY, aButtonColor, aCaption, aCaptionSize,
            aFlags | FLAG_USE_BDBUTTON_FOR_CALLBACK, aValue, reinterpret_cast<void (*)(TouchButton*, int16_t)> (aOnTouchHandler));

#ifdef REMOTE_DISPLAY_SUPPORTED
    mLocalButtonPtr ->mBDButtonPtr = this;
#endif
#endif
}
#ifdef LOCAL_DISPLAY_EXISTS
/*
 * Assume a button stack, e.g. only local buttons are deinitialize which were initialized last.
 * localButtonIndex is used as stack pointer.
 */
void BDButton::deinit(void) {
    sLocalButtonIndex--;
    delete mLocalButtonPtr;
}
#endif

void BDButton::drawButton(void) {
#ifdef LOCAL_DISPLAY_EXISTS
    if (mLocalButtonPtr != NULL) {
        mLocalButtonPtr->drawButton();
    }
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_DRAW, 1, mButtonHandle);
    }
}

void BDButton::removeButton(color16_t aBackgroundColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalButtonPtr->removeButton(aBackgroundColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_REMOVE, 2, mButtonHandle, aBackgroundColor);
    }
}

void BDButton::drawCaption(void) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalButtonPtr->drawCaption();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_DRAW_CAPTION, 1, mButtonHandle);
    }
}

void BDButton::setCaption(const char * aCaption) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalButtonPtr->setCaption(aCaption);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_SET_CAPTION, 1, mButtonHandle, strlen(aCaption), aCaption);
    }
}

/*
 * Sets caption for value true (green button) if different from false (red button) caption
 */
void BDButton::setCaptionForValueTrue(const char * aCaption) {
#ifdef LOCAL_DISPLAY_EXISTS
    // not supported
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_SET_CAPTION_FOR_VALUE_TRUE, 1, mButtonHandle, strlen(aCaption), aCaption);
    }
}

void BDButton::setCaptionAndDraw(const char * aCaption) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalButtonPtr->setCaption(aCaption);
    mLocalButtonPtr->drawButton();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_SET_CAPTION_AND_DRAW_BUTTON, 1, mButtonHandle, strlen(aCaption), aCaption);
    }
}

void BDButton::setCaption(const char * aCaption, bool doDrawButton) {
#ifdef LOCAL_DISPLAY_EXISTS
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

void BDButton::setValue(int16_t aValue) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalButtonPtr->setValue(aValue);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonHandle, SUBFUNCTION_BUTTON_SET_VALUE, aValue);
    }
}

void BDButton::setValue(int16_t aValue, bool doDrawButton) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalButtonPtr->setValue(aValue);
    if (doDrawButton) {
        mLocalButtonPtr->drawButton();
    }
#endif
    uint8_t tSubFunctionCode = SUBFUNCTION_BUTTON_SET_VALUE;
    if (doDrawButton) {
        tSubFunctionCode = SUBFUNCTION_BUTTON_SET_VALUE_AND_DRAW;
    }
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonHandle, tSubFunctionCode, aValue);
}

void BDButton::setValueAndDraw(int16_t aValue) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalButtonPtr->setValue(aValue);
    mLocalButtonPtr->drawButton();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonHandle, SUBFUNCTION_BUTTON_SET_VALUE_AND_DRAW, aValue);
    }
}

void BDButton::setButtonColor(color16_t aButtonColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalButtonPtr->setButtonColor(aButtonColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonHandle, SUBFUNCTION_BUTTON_SET_BUTTON_COLOR, aButtonColor);
    }
}

void BDButton::setButtonColorAndDraw(color16_t aButtonColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalButtonPtr->setButtonColor(aButtonColor);
    mLocalButtonPtr->drawButton();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonHandle, SUBFUNCTION_BUTTON_SET_BUTTON_COLOR_AND_DRAW, aButtonColor);
    }
}

void BDButton::setPosition(int16_t aPositionX, int16_t aPositionY) {
#ifdef LOCAL_DISPLAY_EXISTS
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
#ifdef LOCAL_DISPLAY_EXISTS
//    ((TouchButtonAutorepeat*) mLocalButtonPtr)->setButtonAutorepeatTiming(aMillisFirstDelay, aMillisFirstRate, aFirstCount,
//            aMillisSecondRate);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 7, mButtonHandle, SUBFUNCTION_BUTTON_SET_AUTOREPEAT_TIMING, aMillisFirstDelay,
                aMillisFirstRate, aFirstCount, aMillisSecondRate);
    }
}

void BDButton::activate(void) {
#ifdef LOCAL_DISPLAY_EXISTS
    mLocalButtonPtr->activate();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 2, mButtonHandle, SUBFUNCTION_BUTTON_SET_ACTIVE);
    }
}

void BDButton::deactivate(void) {
#ifdef LOCAL_DISPLAY_EXISTS
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
#ifdef LOCAL_DISPLAY_EXISTS
    TouchButton::deactivateAllButtons();
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_DEACTIVATE_ALL, 0);
    }
}

#ifdef ARDUINO
// Arduino has mapping defines
void BDButton::init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor,
        const __FlashStringHelper * aPGMCaption, uint8_t aCaptionSize, uint8_t aFlags, int16_t aValue,
        void (*aOnTouchHandler)(BDButton*, int16_t)) {

    BDButtonHandle_t tButtonNumber = sLocalButtonIndex++;
    PGM_P tPGMCaption = reinterpret_cast<PGM_P>(aPGMCaption);

    if (USART_isBluetoothPaired()) {
        uint8_t tCaptionLength = strlen_P(tPGMCaption);
        if (tCaptionLength > STRING_BUFFER_STACK_SIZE) {
            tCaptionLength = STRING_BUFFER_STACK_SIZE;
        }
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        strncpy_P(tStringBuffer, tPGMCaption, tCaptionLength);
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 10, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
                aButtonColor, aCaptionSize, aFlags, aValue, aOnTouchHandler, tCaptionLength, tStringBuffer);
    }
    mButtonHandle = tButtonNumber;
}
/*
 * sets only caption
 */
void BDButton::setCaptionPGM(const char * aPGMCaption) {
    if (USART_isBluetoothPaired()) {
        uint8_t tCaptionLength = strlen_P(aPGMCaption);
        if (tCaptionLength > STRING_BUFFER_STACK_SIZE) {
            tCaptionLength = STRING_BUFFER_STACK_SIZE;
        }
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        strncpy_P(tStringBuffer, aPGMCaption, tCaptionLength);
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_SET_CAPTION, 1, mButtonHandle, tCaptionLength, tStringBuffer);
    }
}

void BDButton::setCaption(const __FlashStringHelper * aPGMCaption) {
    if (USART_isBluetoothPaired()) {
        PGM_P tPGMCaption = reinterpret_cast<PGM_P>(aPGMCaption);

        uint8_t tCaptionLength = strlen_P(tPGMCaption);
        if (tCaptionLength > STRING_BUFFER_STACK_SIZE) {
            tCaptionLength = STRING_BUFFER_STACK_SIZE;
        }
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        strncpy_P(tStringBuffer, tPGMCaption, tCaptionLength);
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_SET_CAPTION, 1, mButtonHandle, tCaptionLength, tStringBuffer);
    }
}
/*
 * Sets caption for value true (green button) if different from false (red button) caption
 */
void BDButton::setCaptionPGMForValueTrue(const char * aPGMCaption) {
    if (USART_isBluetoothPaired()) {
        uint8_t tCaptionLength = strlen_P(aPGMCaption);
        if (tCaptionLength > STRING_BUFFER_STACK_SIZE) {
            tCaptionLength = STRING_BUFFER_STACK_SIZE;
        }
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        strncpy_P(tStringBuffer, aPGMCaption, tCaptionLength);
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_SET_CAPTION_FOR_VALUE_TRUE, 1, mButtonHandle, tCaptionLength, tStringBuffer);
    }
}

void BDButton::setCaptionForValueTrue(const __FlashStringHelper * aPGMCaption) {
    if (USART_isBluetoothPaired()) {
        PGM_P tPGMCaption = reinterpret_cast<PGM_P>(aPGMCaption);

        uint8_t tCaptionLength = strlen_P(tPGMCaption);
        if (tCaptionLength > STRING_BUFFER_STACK_SIZE) {
            tCaptionLength = STRING_BUFFER_STACK_SIZE;
        }
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        strncpy_P(tStringBuffer, tPGMCaption, tCaptionLength);
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_SET_CAPTION_FOR_VALUE_TRUE, 1, mButtonHandle, tCaptionLength, tStringBuffer);
    }
}

void BDButton::setCaptionPGM(const char * aPGMCaption, bool doDrawButton) {
    if (USART_isBluetoothPaired()) {
        uint8_t tCaptionLength = strlen_P(aPGMCaption);
        if (tCaptionLength > STRING_BUFFER_STACK_SIZE) {
            tCaptionLength = STRING_BUFFER_STACK_SIZE;
        }
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        strncpy_P(tStringBuffer, aPGMCaption, tCaptionLength);
        uint8_t tFunctionCode = FUNCTION_BUTTON_SET_CAPTION;
        if (doDrawButton) {
            tFunctionCode = FUNCTION_BUTTON_SET_CAPTION_AND_DRAW_BUTTON;
        }
        sendUSARTArgsAndByteBuffer(tFunctionCode, 1, mButtonHandle, tCaptionLength, tStringBuffer);
    }
}

void BDButton::setCaption(const __FlashStringHelper * aPGMCaption, bool doDrawButton) {
    if (USART_isBluetoothPaired()) {
        PGM_P tPGMCaption = reinterpret_cast<PGM_P>(aPGMCaption);

        uint8_t tCaptionLength = strlen_P(tPGMCaption);
        if (tCaptionLength > STRING_BUFFER_STACK_SIZE) {
            tCaptionLength = STRING_BUFFER_STACK_SIZE;
        }
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        strncpy_P(tStringBuffer, tPGMCaption, tCaptionLength);
        uint8_t tFunctionCode = FUNCTION_BUTTON_SET_CAPTION;
        if (doDrawButton) {
            tFunctionCode = FUNCTION_BUTTON_SET_CAPTION_AND_DRAW_BUTTON;
        }
        sendUSARTArgsAndByteBuffer(tFunctionCode, 1, mButtonHandle, tCaptionLength, tStringBuffer);
    }
}
#endif
