/*
 * BDButton.hpp
 *
 * Implementation of the button client stub for the Android BlueDisplay app.
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

#ifndef _BDBUTTON_HPP
#define _BDBUTTON_HPP

#include "BDButton.h"
#include "BlueDisplay.h" // for BUTTONS_SET_BEEP_TONE

#if defined(SUPPORT_LOCAL_DISPLAY)
#include "LocalGUI/LocalTouchButton.hpp"
#include "LocalGUI/LocalTouchButtonAutorepeat.hpp"
#endif

#include <string.h>  // for strlen

/*
 * The number of the button used as identifier for the remote button.
 * Can be interpreted as index into button stack.
 */
BDButtonHandle_t sLocalButtonIndex = 0;

BDButton::BDButton() { // @suppress("Class members should be properly initialized")
}

BDButton::BDButton(BDButtonHandle_t aButtonHandle) { // @suppress("Class members should be properly initialized")
    mButtonHandle = aButtonHandle;
}

BDButton::BDButton(BDButton const &aButton) { // @suppress("Class members should be properly initialized")
    mButtonHandle = aButton.mButtonHandle;
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr = aButton.mLocalButtonPtr;
#endif
}

bool BDButton::operator==(const BDButton &aButton) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    return (mButtonHandle == aButton.mButtonHandle && mLocalButtonPtr == aButton.mLocalButtonPtr);
#else
    return (mButtonHandle == aButton.mButtonHandle);
#endif
}

bool BDButton::operator!=(const BDButton &aButton) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    return (mButtonHandle != aButton.mButtonHandle || mLocalButtonPtr != aButton.mLocalButtonPtr);
#else
    return (mButtonHandle != aButton.mButtonHandle);
#endif
}

#if defined(SUPPORT_LOCAL_DISPLAY)
bool BDButton::operator==(const LocalTouchButton &aButton) {
    return (mLocalButtonPtr == &aButton);
}

bool BDButton::operator!=(const LocalTouchButton &aButton) {
    return (mLocalButtonPtr != &aButton);
}
#endif

/*
 * initialize a button stub
 * If local display is attached, allocate a button (if the local pool is used, do not forget to call deinit()).
 * Multi-line caption requires \n as line separator.
 * @param aCaption  value for false (0) if FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN is set
 *
 * Assume, that remote display is attached if called here! Use TouchButton::initButton() for only local display.
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

#if defined(SUPPORT_LOCAL_DISPLAY)
    /*
     * Allocate a local button here to be displayed locally
     */
    if (aFlags & FLAG_BUTTON_TYPE_AUTOREPEAT) {
#  if defined(DISABLE_REMOTE_DISPLAY)
        mLocalButtonPtr = new LocalTouchButtonAutorepeat();
#  else
        mLocalButtonPtr = new LocalTouchButtonAutorepeat(this);
#  endif
    } else {
#  if defined(DISABLE_REMOTE_DISPLAY)
        mLocalButtonPtr = new LocalTouchButton();
#  else
        mLocalButtonPtr = new LocalTouchButton(this);
#  endif
    }
    // Cast required here. At runtime the right pointer is returned because of FLAG_USE_BDBUTTON_FOR_CALLBACK
    mLocalButtonPtr->init(aPositionX, aPositionY, aWidthX, aHeightY, aButtonColor, aCaption, aCaptionSize,
            aFlags | LOCAL_BUTTON_FLAG_USE_BDBUTTON_FOR_CALLBACK, aValue, reinterpret_cast<void (*)(LocalTouchButton*, int16_t)> (aOnTouchHandler));

#endif
}

void BDButton::init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor,
        const __FlashStringHelper *aPGMCaption, uint16_t aCaptionSize, uint8_t aFlags, int16_t aValue,
        void (*aOnTouchHandler)(BDButton*, int16_t)) {
#if !defined (AVR)
    init(aPositionX, aPositionY, aWidthX, aHeightY, aButtonColor, reinterpret_cast<const char*>(aPGMCaption), aCaptionSize, aFlags, aValue,
            aOnTouchHandler);
#else

    BDButtonHandle_t tButtonNumber = sLocalButtonIndex++;

    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        uint8_t tCaptionLength = _clipAndCopyPGMString(tStringBuffer, aPGMCaption);

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
#if defined(SUPPORT_LOCAL_DISPLAY)
    /*
     * Allocate a local button here to be displayed locally
     */
    if (aFlags & FLAG_BUTTON_TYPE_AUTOREPEAT) {
#  if defined(DISABLE_REMOTE_DISPLAY)
        mLocalButtonPtr = new LocalTouchButtonAutorepeat();
#  else
        mLocalButtonPtr = new LocalTouchButtonAutorepeat(this);
#  endif
    } else {
#  if defined(DISABLE_REMOTE_DISPLAY)
        mLocalButtonPtr = new LocalTouchButton();
#  else
        mLocalButtonPtr = new LocalTouchButton(this);
#  endif
    }
    // Cast required here. At runtime the right pointer is returned because of FLAG_USE_BDBUTTON_FOR_CALLBACK
    mLocalButtonPtr->init(aPositionX, aPositionY, aWidthX, aHeightY, aButtonColor, aPGMCaption, aCaptionSize,
            aFlags | LOCAL_BUTTON_FLAG_USE_BDBUTTON_FOR_CALLBACK, aValue, reinterpret_cast<void (*)(LocalTouchButton*, int16_t)> (aOnTouchHandler));
#endif // defined(SUPPORT_LOCAL_DISPLAY)
#endif // !defined (AVR)
}

/*
 * This function deletes the last BDButton initialized by BDButton::init() simply by decreasing sLocalButtonIndex by one.
 * So next BDButton::init() uses the same button on the remote side again.
 * The local button is deleted regular.
 * This assumes a button stack. with localButtonIndex as stack pointer.
 * The local button is deleted regular.
 *
 * This helps to reuse the memory of unused local buttons if a display page was left,
 * but requires to do the init() of each button at page entering/start time and the deinit() at leaving.
 * This cannot be done with ~BDButton, because we can only delete the last BDButton from stack, and not any arbitrary one.
 */
void BDButton::deinit() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    sLocalButtonIndex--;
    delete mLocalButtonPtr; // free memory
#endif
}

void BDButton::drawButton() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    if (mLocalButtonPtr != NULL) {
        mLocalButtonPtr->drawButton();
    }
#endif
    sendUSARTArgs(FUNCTION_BUTTON_DRAW, 1, mButtonHandle);
}

void BDButton::removeButton(color16_t aBackgroundColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->removeButton(aBackgroundColor);
#endif
    sendUSARTArgs(FUNCTION_BUTTON_REMOVE, 2, mButtonHandle, aBackgroundColor);
}

/*
 * Sets caption for value true (green button) if different from default false (red button) caption
 */
void BDButton::setCaptionForValueTrue(const char *aCaption) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->setCaptionForValueTrue(aCaption);
#endif
    sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_SET_CAPTION_FOR_VALUE_TRUE, 1, mButtonHandle, strlen(aCaption), aCaption);
}

void BDButton::setCaptionForValueTrue(const __FlashStringHelper *aPGMCaption) {
#if defined (AVR)
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        uint8_t tCaptionLength = _clipAndCopyPGMString(tStringBuffer, aPGMCaption);
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_SET_CAPTION_FOR_VALUE_TRUE, 1, mButtonHandle, tCaptionLength, tStringBuffer);
    }
#else
    setCaptionForValueTrue(reinterpret_cast<const char*>(aPGMCaption));
#endif
}

void BDButton::setCaption(const char *aCaption, bool doDrawButton) {
#if defined(SUPPORT_LOCAL_DISPLAY)
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
void BDButton::setCaption(const __FlashStringHelper *aPGMCaption, bool doDrawButton) {
#if defined (AVR)
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        uint8_t tCaptionLength = _clipAndCopyPGMString(tStringBuffer, aPGMCaption);
        uint8_t tFunctionCode = FUNCTION_BUTTON_SET_CAPTION;
        if (doDrawButton) {
            tFunctionCode = FUNCTION_BUTTON_SET_CAPTION_AND_DRAW_BUTTON;
        }
        sendUSARTArgsAndByteBuffer(tFunctionCode, 1, mButtonHandle, tCaptionLength, tStringBuffer);
    }
#else
    setCaption(reinterpret_cast<const char*>(aPGMCaption), doDrawButton);
#endif
}

void BDButton::setCaptionFromStringArray(const char *const *aCaptionStringArrayPtr, uint8_t aStringIndex, bool doDrawButton) {
    setCaption(aCaptionStringArrayPtr[aStringIndex], doDrawButton);
}

void BDButton::setCaptionFromStringArray(const __FlashStringHelper *const *aPGMCaptionStringArrayPtr, uint8_t aStringIndex,
        bool doDrawButton) {
#if defined(AVR)
        __FlashStringHelper *tPGMCaption = (__FlashStringHelper*) pgm_read_word(&aPGMCaptionStringArrayPtr[aStringIndex]);
        setCaption(tPGMCaption, doDrawButton);
    #else
        setCaptionFromStringArray((const char *const *)aPGMCaptionStringArrayPtr,aStringIndex,doDrawButton);
#endif
}

void BDButton::setValue(int16_t aValue, bool doDrawButton) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->setValue(aValue);
    if (doDrawButton) {
        mLocalButtonPtr->drawButton();
    }
#endif
    if (USART_isBluetoothPaired()) {
        uint8_t tSubFunctionCode = SUBFUNCTION_BUTTON_SET_VALUE;
        if (doDrawButton) {
            tSubFunctionCode = SUBFUNCTION_BUTTON_SET_VALUE_AND_DRAW;
        }
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonHandle, tSubFunctionCode, aValue);
    }
}

void BDButton::setValueAndDraw(int16_t aValue) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->setValue(aValue);
    mLocalButtonPtr->drawButton();
#endif
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonHandle, SUBFUNCTION_BUTTON_SET_VALUE_AND_DRAW, aValue);
}

void BDButton::setButtonColor(color16_t aButtonColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->setButtonColor(aButtonColor);
#endif
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonHandle, SUBFUNCTION_BUTTON_SET_BUTTON_COLOR, aButtonColor);
}

void BDButton::setButtonColorAndDraw(color16_t aButtonColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->setButtonColor(aButtonColor);
    mLocalButtonPtr->drawButton();
#endif
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonHandle, SUBFUNCTION_BUTTON_SET_BUTTON_COLOR_AND_DRAW, aButtonColor);
}

void BDButton::setPosition(int16_t aPositionX, int16_t aPositionY) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->setPosition(aPositionX, aPositionY);
#endif
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 4, mButtonHandle, SUBFUNCTION_BUTTON_SET_POSITION, aPositionX, aPositionY);
}

/**
 * after aMillisFirstDelay milliseconds a callback is done every aMillisFirstRate milliseconds for aFirstCount times
 * after this a callback is done every aMillisSecondRate milliseconds
 */
void BDButton::setButtonAutorepeatTiming(uint16_t aMillisFirstDelay, uint16_t aMillisFirstRate, uint16_t aFirstCount,
        uint16_t aMillisSecondRate) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    // In this case, timing must also be generated locally
    ((LocalTouchButtonAutorepeat*) mLocalButtonPtr)->setButtonAutorepeatTiming(aMillisFirstDelay, aMillisFirstRate, aFirstCount,
            aMillisSecondRate);
#endif
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 7, mButtonHandle, SUBFUNCTION_BUTTON_SET_AUTOREPEAT_TIMING, aMillisFirstDelay,
            aMillisFirstRate, aFirstCount, aMillisSecondRate);
}

void BDButton::disableAutorepeatUntilEndOfTouch() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalTouchButtonAutorepeat::disableAutorepeatUntilEndOfTouch();
#endif
    sendUSARTArgs(FUNCTION_BUTTON_DISABLE_AUTOREPEAT_UNTIL_END_OF_TOUCH, 0); // 2/2023 not yet implemented
}

void BDButton::activate() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->activate();
#endif
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 2, mButtonHandle, SUBFUNCTION_BUTTON_SET_ACTIVE);
}

void BDButton::deactivate() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->deactivate();
#endif
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 2, mButtonHandle, SUBFUNCTION_BUTTON_RESET_ACTIVE);
}

/*
 * Static functions
 */
void BDButton::resetAll() {
    sLocalButtonIndex = 0;
}

/*
 * @param aFlags FLAG_BUTTON_GLOBAL_USE_DOWN_EVENTS_FOR_BUTTONS, FLAG_BUTTON_GLOBAL_USE_UP_EVENTS_FOR_BUTTONS
 */
void BDButton::setGlobalFlags(uint16_t aFlags) {
    sendUSARTArgs(FUNCTION_BUTTON_GLOBAL_SETTINGS, 1, aFlags);
}

/*
 * aToneVolume: value in percent
 */
void BDButton::setButtonsTouchTone(uint8_t aToneIndex, uint16_t aToneDuration) {
    sendUSARTArgs(FUNCTION_BUTTON_GLOBAL_SETTINGS, 3, FLAG_BUTTON_GLOBAL_SET_BEEP_TONE, aToneIndex, aToneDuration);
}

void BDButton::setButtonsTouchTone(uint8_t aToneIndex, uint16_t aToneDuration, uint8_t aToneVolume) {
    sendUSARTArgs(FUNCTION_BUTTON_GLOBAL_SETTINGS, 4, FLAG_BUTTON_GLOBAL_SET_BEEP_TONE, aToneIndex, aToneDuration, aToneVolume);
}

void BDButton::playFeedbackTone() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalTouchButton::playFeedbackTone();
#endif
    sendUSARTArgs(FUNCTION_PLAY_TONE, 1, FEEDBACK_TONE_OK);
}

void BDButton::playFeedbackTone(bool aPlayErrorTone) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalTouchButton::playFeedbackTone(aPlayErrorTone);
#endif
    uint8_t tAndroidToneIndex = TONE_PROP_BEEP_OK;
    if (aPlayErrorTone) {
        tAndroidToneIndex = TONE_PROP_BEEP_ERROR;
    }
    sendUSARTArgs(FUNCTION_PLAY_TONE, 1, tAndroidToneIndex);
}

void BDButton::activateAll() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalTouchButton::activateAll();
#endif
    sendUSARTArgs(FUNCTION_BUTTON_ACTIVATE_ALL, 0);
}

void BDButton::deactivateAll() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalTouchButton::deactivateAll();
#endif
    sendUSARTArgs(FUNCTION_BUTTON_DEACTIVATE_ALL, 0);
}

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
//        uint8_t tLength = _clipAndCopyPGMString(tStringBuffer, aPGMCaption);
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

#if defined(AVR)
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

#endif // defined(AVR)

#endif //_BDBUTTON_H
