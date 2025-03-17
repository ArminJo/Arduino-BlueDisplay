/*
 * BDButton.hpp
 *
 * Implementation of the button client stub for the Android BlueDisplay app.
 *
 *  Copyright (C) 2015-2025  Armin Joachimsmeyer
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
BDButtonIndex_t sLocalButtonIndex = 0;

BDButton::BDButton() { // @suppress("Class members should be properly initialized")
}

BDButton::BDButton(BDButtonIndex_t aButtonHandle) { // @suppress("Class members should be properly initialized")
    mButtonIndex = aButtonHandle;
}

BDButton::BDButton(BDButton const &aButton) { // @suppress("Class members should be properly initialized")
    mButtonIndex = aButton.mButtonIndex;
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr = aButton.mLocalButtonPtr;
#endif
}

bool BDButton::operator==(const BDButton &aButton) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    return (mButtonIndex == aButton.mButtonIndex && mLocalButtonPtr == aButton.mLocalButtonPtr);
#else
    return (mButtonIndex == aButton.mButtonIndex);
#endif
}

bool BDButton::operator!=(const BDButton &aButton) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    return (mButtonIndex != aButton.mButtonIndex || mLocalButtonPtr != aButton.mLocalButtonPtr);
#else
    return (mButtonIndex != aButton.mButtonIndex);
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

void BDButton::setInitParameters(BDButtonParameterStruct *aBDButtonParameterStructToFill, uint16_t aPositionX, uint16_t aPositionY,
        uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor, const char *aText, uint16_t aTextSize, uint8_t aFlags,
        int16_t aValue, void (*aOnTouchHandler)(BDButton*, int16_t)) {
    aBDButtonParameterStructToFill->aPositionX = aPositionX;
    aBDButtonParameterStructToFill->aPositionY = aPositionY;
    aBDButtonParameterStructToFill->aWidthX = aWidthX;
    aBDButtonParameterStructToFill->aHeightY = aHeightY;
    aBDButtonParameterStructToFill->aButtonColor = aButtonColor;
    aBDButtonParameterStructToFill->aText = aText;
    aBDButtonParameterStructToFill->aTextSize = aTextSize;
    aBDButtonParameterStructToFill->aFlags = aFlags;
    aBDButtonParameterStructToFill->aValue = aValue;
    aBDButtonParameterStructToFill->aOnTouchHandler = aOnTouchHandler;
}

#if defined(__AVR__)
void BDButton::setInitParameters(BDButtonPGMTextParameterStruct *aBDButtonParameterStructToFill, uint16_t aPositionX,
        uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor, const __FlashStringHelper *aPGMText,
        uint16_t aTextSize, uint8_t aFlags, int16_t aValue, void (*aOnTouchHandler)(BDButton*, int16_t)) {
    aBDButtonParameterStructToFill->aPositionX = aPositionX;
    aBDButtonParameterStructToFill->aPositionY = aPositionY;
    aBDButtonParameterStructToFill->aWidthX = aWidthX;
    aBDButtonParameterStructToFill->aHeightY = aHeightY;
    aBDButtonParameterStructToFill->aButtonColor = aButtonColor;
    aBDButtonParameterStructToFill->aPGMText = aPGMText;
    aBDButtonParameterStructToFill->aTextSize = aTextSize;
    aBDButtonParameterStructToFill->aFlags = aFlags;
    aBDButtonParameterStructToFill->aValue = aValue;
    aBDButtonParameterStructToFill->aOnTouchHandler = aOnTouchHandler;
}
#endif

void BDButton::init(BDButtonParameterStruct *aBDButtonParameter) {
    init(aBDButtonParameter->aPositionX, aBDButtonParameter->aPositionY, aBDButtonParameter->aWidthX, aBDButtonParameter->aHeightY,
            aBDButtonParameter->aButtonColor, aBDButtonParameter->aText, aBDButtonParameter->aTextSize, aBDButtonParameter->aFlags,
            aBDButtonParameter->aValue, aBDButtonParameter->aOnTouchHandler);
}
/*
 * Each further call costs 10 to 14 bytes program memory
 */
#if defined(__AVR__)
void BDButton::init(BDButtonPGMTextParameterStruct *aBDButtonParameter) {
    init(aBDButtonParameter->aPositionX, aBDButtonParameter->aPositionY, aBDButtonParameter->aWidthX, aBDButtonParameter->aHeightY,
            aBDButtonParameter->aButtonColor, aBDButtonParameter->aPGMText, aBDButtonParameter->aTextSize,
            aBDButtonParameter->aFlags, aBDButtonParameter->aValue, aBDButtonParameter->aOnTouchHandler);
}
#endif // !defined (AVR)

/*
 * initialize a button stub
 * If local display is attached, allocate a button (if the local pool is used, do not forget to call deinit()).
 * Multi-line text requires \n as line separator.
 * @param aText  value for false (0) if FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN is set
 *
 * Assume, that remote display is attached if called here! Use TouchButton::initButton() for only local display.
 */
void BDButton::init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor,
        const char *aText, uint16_t aTextSize, uint8_t aFlags, int16_t aValue, void (*aOnTouchHandler)(BDButton*, int16_t)) {

    BDButtonIndex_t tButtonNumber = sLocalButtonIndex++;
    if (USART_isBluetoothPaired()) {
#if __SIZEOF_POINTER__ == 4
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 11, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
                aButtonColor, aTextSize, aFlags, aValue, aOnTouchHandler, (reinterpret_cast<uint32_t>(aOnTouchHandler) >> 16),
                strlen(aText), aText);
#else
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 10, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
                aButtonColor, aTextSize, aFlags, aValue, aOnTouchHandler, strlen(aText), aText);
#endif
    }
    mButtonIndex = tButtonNumber;

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
    mLocalButtonPtr->init(aPositionX, aPositionY, aWidthX, aHeightY, aButtonColor, aText, aTextSize,
            aFlags | LOCAL_BUTTON_FLAG_USE_BDBUTTON_FOR_CALLBACK, aValue, reinterpret_cast<void (*)(LocalTouchButton*, int16_t)> (aOnTouchHandler));

#endif
}

// The same but with aPGMText instead of Text
void BDButton::init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor,
        const __FlashStringHelper *aPGMText, uint16_t aTextSize, uint8_t aFlags, int16_t aValue,
        void (*aOnTouchHandler)(BDButton*, int16_t)) {

#if !defined (AVR)
    init(aPositionX, aPositionY, aWidthX, aHeightY, aButtonColor, reinterpret_cast<const char*>(aPGMText), aTextSize, aFlags,
            aValue, aOnTouchHandler);
#else
    BDButtonIndex_t tButtonNumber = sLocalButtonIndex++;

    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        uint8_t tTextLength = _clipAndCopyPGMString(tStringBuffer, aPGMText);

#if __SIZEOF_POINTER__ == 4
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 11, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
                aButtonColor, aTextSize, aFlags, aValue, aOnTouchHandler, (reinterpret_cast<uint32_t>(aOnTouchHandler) >> 16),
                tTextLength, tStringBuffer);
#else
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 10, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
                aButtonColor, aTextSize, aFlags, aValue, aOnTouchHandler, tTextLength, tStringBuffer);
#endif
    }
    mButtonIndex = tButtonNumber;
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
    mLocalButtonPtr->init(aPositionX, aPositionY, aWidthX, aHeightY, aButtonColor, aPGMText, aTextSize,
            aFlags | LOCAL_BUTTON_FLAG_USE_BDBUTTON_FOR_CALLBACK, aValue, reinterpret_cast<void (*)(LocalTouchButton*, int16_t)> (aOnTouchHandler));
#endif // defined(SUPPORT_LOCAL_DISPLAY)
#endif // !defined (AVR)
}

void BDButton::setCallback(void (*aOnTouchHandler)(BDButton*, int16_t)) {
#if __SIZEOF_POINTER__ == 4
    sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 4, mButtonIndex, SUBFUNCTION_SLIDER_SET_CALLBACK, aOnTouchHandler,
                (reinterpret_cast<uint32_t>(aOnTouchHandler) >> 16));
#else
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonIndex, SUBFUNCTION_SLIDER_SET_CALLBACK, aOnTouchHandler);
#endif
}

/*
 * @param aFlags - See #FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN etc.
 */
void BDButton::setFlags(int16_t aFlags) {
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonIndex, SUBFUNCTION_BUTTON_SET_FLAGS, aFlags);
}

/*
 * This function deletes the last BDButton initialized by BDButton::init() simply by decreasing sLocalButtonIndex by one.
 * So next BDButton::init() uses the same button on the remote side again.
 * The memory allocated for this button (one byte on the heap) must be released manually.
 *
 * The local button is deleted regular.
 * This assumes a button stack. with localButtonIndex as stack pointer.
 * The local button is deleted regular.
 *
 * This helps to reuse the memory of unused local buttons if a display page was left,
 * but requires to do the init() of each button at page entering/start time and the deinit() at leaving.
 * This cannot be done with ~BDButton, because we can only delete the last BDButton from stack, and not any arbitrary one.
 */
void BDButton::deinit() {
    sLocalButtonIndex--;
#if defined(SUPPORT_LOCAL_DISPLAY)
    delete mLocalButtonPtr; // free memory
#endif
}

/*
 * Overwrites and deactivate button
 */
void BDButton::removeButton(color16_t aBackgroundColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->removeButton(aBackgroundColor);
#endif
    sendUSARTArgs(FUNCTION_BUTTON_REMOVE, 2, mButtonIndex, aBackgroundColor);
}

void BDButton::drawButton() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    if (mLocalButtonPtr != nullptr) {
        mLocalButtonPtr->drawButton();
    }
#endif
    sendUSARTArgs(FUNCTION_BUTTON_DRAW, 1, mButtonIndex);
}

#if !defined(OMIT_BD_DEPRECATED_FUNCTIONS)
void BDButton::setCaptionForValueTrue(const char *aText) {
    setTextForValueTrue(aText);
}

void BDButton::setCaptionForValueTrue(const __FlashStringHelper *aPGMText) {
    setTextForValueTrue(aPGMText);
}

void BDButton::setCaption(const char *aText, bool doDrawButton) {
    setText(aText, doDrawButton);
}

void BDButton::setCaption(const __FlashStringHelper *aPGMText, bool doDrawButton) {
    setText(aPGMText, doDrawButton);
}

void BDButton::setCaptionFromStringArray(const char *const*aTextStringArrayPtr, uint8_t aStringIndex, bool doDrawButton) {
    setText(aTextStringArrayPtr[aStringIndex], doDrawButton);
}

void BDButton::setCaptionFromStringArray(const __FlashStringHelper* const *aPGMTextStringArrayPtr, uint8_t aStringIndex,
        bool doDrawButton) {
    setTextFromStringArray(aPGMTextStringArrayPtr, aStringIndex, doDrawButton);
}
#endif

/*
 * Sets text for value true (green button) if different from default false (red button) text
 * And implicitly converts button to a green button
 */
void BDButton::setTextForValueTrue(const char *aText) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->setTextForValueTrue(aText);
#endif
    sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_SET_TEXT_FOR_VALUE_TRUE, 1, mButtonIndex, strlen(aText), aText);
}
void BDButton::setTextForValueTrue(const __FlashStringHelper *aPGMText) {
#if defined (AVR)
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        uint8_t tTextLength = _clipAndCopyPGMString(tStringBuffer, aPGMText);
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_SET_TEXT_FOR_VALUE_TRUE, 1, mButtonIndex, tTextLength, tStringBuffer);
    }
#else
    setTextForValueTrue(reinterpret_cast<const char*>(aPGMText));
#endif
}

void BDButton::setText(const char *aText, bool doDrawButton) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->setText(aText);
    if (doDrawButton) {
        mLocalButtonPtr->drawButton();
    }
#endif
    if (USART_isBluetoothPaired()) {
        uint8_t tFunctionCode = FUNCTION_BUTTON_SET_TEXT;
        if (doDrawButton) {
            tFunctionCode = FUNCTION_BUTTON_SET_TEXT_AND_DRAW_BUTTON;
        }
        sendUSARTArgsAndByteBuffer(tFunctionCode, 1, mButtonIndex, strlen(aText), aText);
    }
}

void BDButton::setText(const __FlashStringHelper *aPGMText, bool doDrawButton) {
#if defined (AVR)
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        uint8_t tTextLength = _clipAndCopyPGMString(tStringBuffer, aPGMText);
        uint8_t tFunctionCode = FUNCTION_BUTTON_SET_TEXT;
        if (doDrawButton) {
            tFunctionCode = FUNCTION_BUTTON_SET_TEXT_AND_DRAW_BUTTON;
        }
        sendUSARTArgsAndByteBuffer(tFunctionCode, 1, mButtonIndex, tTextLength, tStringBuffer);
    }
#else
    setText(reinterpret_cast<const char*>(aPGMText), doDrawButton);
#endif
}

void BDButton::setTextFromStringArray(const char *const*aTextStringArrayPtr, uint8_t aStringIndex, bool doDrawButton) {
    setText(aTextStringArrayPtr[aStringIndex], doDrawButton);
}

void BDButton::setTextFromStringArray(const __FlashStringHelper* const *aPGMTextStringArrayPtr, uint8_t aStringIndex,
        bool doDrawButton) {
#if defined(__AVR__)
    __FlashStringHelper *tPGMText = (__FlashStringHelper*) pgm_read_word(&aPGMTextStringArrayPtr[aStringIndex]);
    setText(tPGMText, doDrawButton);
#else
    setTextFromStringArray((const char* const*) aPGMTextStringArrayPtr, aStringIndex, doDrawButton);
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
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonIndex, tSubFunctionCode, aValue);
    }
}

void BDButton::setValueAndDraw(int16_t aValue) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->setValue(aValue);
    mLocalButtonPtr->drawButton();
#endif
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonIndex, SUBFUNCTION_BUTTON_SET_VALUE_AND_DRAW, aValue);
}

void BDButton::setButtonColor(color16_t aButtonColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->setButtonColor(aButtonColor);
#endif
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonIndex, SUBFUNCTION_BUTTON_SET_BUTTON_COLOR, aButtonColor);
}

void BDButton::setButtonColorAndDraw(color16_t aButtonColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->setButtonColor(aButtonColor);
    mLocalButtonPtr->drawButton();
#endif
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonIndex, SUBFUNCTION_BUTTON_SET_BUTTON_COLOR_AND_DRAW, aButtonColor);
}

void BDButton::setButtonTextColor(color16_t aButtonTextColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->setTextColor(aButtonTextColor);
#endif
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonIndex, SUBFUNCTION_BUTTON_SET_TEXT_COLOR, aButtonTextColor);
}

void BDButton::setButtonTextColorAndDraw(color16_t aButtonTextColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->setButtonColor(aButtonTextColor);
    mLocalButtonPtr->drawButton();
#endif
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, mButtonIndex, SUBFUNCTION_BUTTON_SET_TEXT_COLOR_AND_DRAW, aButtonTextColor);
}

void BDButton::setPosition(int16_t aPositionX, int16_t aPositionY) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->setPosition(aPositionX, aPositionY);
#endif
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 4, mButtonIndex, SUBFUNCTION_BUTTON_SET_POSITION, aPositionX, aPositionY);
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
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 7, mButtonIndex, SUBFUNCTION_BUTTON_SET_AUTOREPEAT_TIMING, aMillisFirstDelay,
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
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 2, mButtonIndex, SUBFUNCTION_BUTTON_SET_ACTIVE);
}

void BDButton::deactivate() {
#if defined(SUPPORT_LOCAL_DISPLAY)
    mLocalButtonPtr->deactivate();
#endif
    sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 2, mButtonIndex, SUBFUNCTION_BUTTON_RESET_ACTIVE);
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
//uint8_t sLastTextSize;
//uint8_t sLastFlags;
//void BDButton::init(uint16_t aPositionX, uint16_t aPositionY, const __FlashStringHelper *aPGMText, int16_t aValue,
//        void (*aOnTouchHandler)(BDButton*, int16_t)) {
//
//    BDButtonIndex_t tButtonNumber = sLocalButtonIndex++;
//    PGM_P tPGMText = reinterpret_cast<PGM_P>(aPGMText);
//
//    if (USART_isBluetoothPaired()) {
//        uint8_t tTextLength = strlen_P(tPGMText);
//        if (tTextLength > STRING_BUFFER_STACK_SIZE) {
//            tTextLength = STRING_BUFFER_STACK_SIZE;
//        }
//        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
//        strncpy_P(tStringBuffer, tPGMText, tTextLength);
//
//#if __SIZEOF_POINTER__ == 4
//        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 11, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
//                aButtonColor, aTextSize, aFlags, aValue, aOnTouchHandler, (reinterpret_cast<uint32_t>(aOnTouchHandler) >> 16),
//                tTextLength, tStringBuffer);
//#else
//        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 10, tButtonNumber, aPositionX, aPositionY, sLastWidthX, sLastHeightY,
//                sLastButtonColor, sLastTextSize, sLastFlags, aValue, aOnTouchHandler, tTextLength, tStringBuffer);
//#endif
//    }
//    mButtonIndex = tButtonNumber;
//}

// This uses around 200 bytes and saves 8 to 24 bytes per button
//void BDButton::init(const struct ButtonInit *aButtonInfo, const __FlashStringHelper *aPGMText ) {
//    init(aButtonInfo,aPGMText,pgm_read_word(aButtonInfo->Value));
//}
//
//void BDButton::init(const struct ButtonInit *aButtonInfo, const __FlashStringHelper *aPGMText, int16_t aValue ) {
//
//    BDButtonIndex_t tButtonNumber = sLocalButtonIndex++;
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
//        uint8_t tLength = _clipAndCopyPGMString(tStringBuffer, aPGMText);
//
//        // add data field header
//        tBufferPointer += sizeof(ButtonInit) / sizeof(uint16_t);
//        *tBufferPointer++ = DATAFIELD_TAG_BYTE << 8 | SYNC_TOKEN; // start new transmission block
//        *tBufferPointer = tLength;
//
//        sendUSARTBufferNoSizeCheck(reinterpret_cast<uint8_t*>(&tParamBuffer[0]), 10 * 2 + 8,
//                reinterpret_cast<uint8_t*>(&tStringBuffer[0]), tLength);
//    }
//    mButtonIndex = tButtonNumber;
//}

#if !defined(OMIT_BD_DEPRECATED_FUNCTIONS)
#  if defined(__AVR__)
void BDButton::setCaptionPGMForValueTrue(const char *aPGMText) {
    setTextForValueTrue((const __FlashStringHelper*) aPGMText);
}

/*
 * deprecated: Use setTextFromStringArray((const __FlashStringHelper* const*) aTextStringArrayPtr,...) with this cast
 */
void BDButton::setCaptionFromStringArrayPGM(const char *const aPGMTextStringArrayPtr[], uint8_t aStringIndex, bool doDrawButton) {
#    if defined(__AVR__)
    __FlashStringHelper *tPGMText = (__FlashStringHelper*) pgm_read_word(&aPGMTextStringArrayPtr[aStringIndex]);
#    else
    const char *tPGMText = aPGMTextStringArrayPtr[aStringIndex];
#    endif
    setText(tPGMText, doDrawButton);
}

void BDButton::setCaptionPGM(const char *aPGMText, bool doDrawButton) {
    setText(reinterpret_cast<const __FlashStringHelper*>(aPGMText), doDrawButton);
}

#  endif // defined(__AVR__)
#  endif // !defined(OMIT_BD_DEPRECATED_FUNCTIONS)

#endif //_BDBUTTON_H
