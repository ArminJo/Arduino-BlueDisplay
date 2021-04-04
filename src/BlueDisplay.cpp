/*
 * BlueDisplay.cpp
 *
 * C stub for Android BlueDisplay app (and the local MI0283QT2 Display from Watterott).
 * It implements a few display test functions.
 *
 *  SUMMARY
 *  Blue Display is an Open Source Android remote Display for Arduino etc.
 *  It receives basic draw requests from Arduino etc. over Bluetooth and renders it.
 *  It also implements basic GUI elements as buttons and sliders.
 *  GUI callback, touch and sensor events are sent back to Arduino.
 *
 *  Copyright (C) 2014-2020  Armin Joachimsmeyer
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

#include "BlueDisplay.h"

#ifdef LOCAL_DISPLAY_EXISTS
#include "thickLine.h"
#include "tinyPrint.h"
#endif

#include <string.h>  // for strlen
#include <stdio.h> /* for sprintf */
#include <math.h> // for PI
#include <stdlib.h> // for dtostrf()

//-------------------- Constructor --------------------

BlueDisplay::BlueDisplay(void) { // @suppress("Class members should be properly initialized")
    mRequestedDisplaySize.XWidth = DISPLAY_DEFAULT_WIDTH;
    mRequestedDisplaySize.YHeight = DISPLAY_DEFAULT_HEIGHT;
    mConnectionEstablished = false;
}

// One instance of BlueDisplay called BlueDisplay1
BlueDisplay BlueDisplay1;

bool isLocalDisplayAvailable = false;

void BlueDisplay::resetLocal(void) {
    // reset local buttons to be synchronized
    BDButton::resetAllButtons();
    BDSlider::resetAllSliders();
}

/**
 * Sets callback handler and calls host for requestMaxCanvasSize().
 * This results in a EVENT_REQUESTED_DATA_CANVAS_SIZE callback event, which sends display size and local timestamp.
 * This event calls the ConnectCallback as well as the RedrawCallback.
 *
 * Waits for 300 ms for connection to be established -> bool BlueDisplay1.mConnectionEstablished
 *
 * Reorientation callback function is only required if we have a responsive layout,
 * since connect and reorientation event also calls the redraw callback.
 */
void BlueDisplay::initCommunication(void (*aConnectCallback)(void), void (*aRedrawCallback)(void),
        void (*aReorientationCallback)(void)) {
    registerConnectCallback(aConnectCallback);
    registerReorientationCallback(aReorientationCallback);
    registerRedrawCallback(aRedrawCallback);

    mConnectionEstablished = false;
    // consume up old received data
    checkAndHandleEvents();

// This results in a data event, which sends size and timestamp
    requestMaxCanvasSize();

    for (uint_fast8_t i = 0; i < 30; ++i) {
        /*
         * Wait 300 ms for size to be sent back by a canvas size event.
         * Time measured is between 50 and 150 ms (or 80 and 120) for Bluetooth.
         */
        delayMillisWithCheckAndHandleEvents(10);
        if (mConnectionEstablished) { // is set by delayMillisWithCheckAndHandleEvents()
#if defined(TRACE) && defined(USE_SERIAL1)
            Serial.println("Connection established");
#endif
            // Handler are called initially by the received canvas size event
            break;
        }
    }
}

bool BlueDisplay::isConnectionEstablished() {
    return mConnectionEstablished;
}
// sends 4 byte function and 24 byte data message
void BlueDisplay::sendSync(void) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        memset(tStringBuffer, 0, STRING_BUFFER_STACK_SIZE);
        sendUSARTArgsAndByteBuffer(FUNCTION_NOP, 0, STRING_BUFFER_STACK_SIZE, tStringBuffer);
    }
}

void BlueDisplay::setFlagsAndSize(uint16_t aFlags, uint16_t aWidth, uint16_t aHeight) {
    mRequestedDisplaySize.XWidth = aWidth;
    mRequestedDisplaySize.YHeight = aHeight;
    if (USART_isBluetoothPaired()) {
        if (aFlags & BD_FLAG_FIRST_RESET_ALL) {
#if defined(TRACE) && defined(USE_SERIAL1)
            Serial.println("Send reset all");
#endif
            // reset local buttons to be synchronized
            BDButton::resetAllButtons();
            BDSlider::resetAllSliders();
        }
        sendUSARTArgs(FUNCTION_GLOBAL_SETTINGS, 4, SUBFUNCTION_GLOBAL_SET_FLAGS_AND_SIZE, aFlags, aWidth, aHeight);
    }
}

/**
 *
 * @param aCodePage Number for ISO_8859_<Number>
 */
void BlueDisplay::setCodePage(uint16_t aCodePageNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_GLOBAL_SETTINGS, 2, SUBFUNCTION_GLOBAL_SET_CODEPAGE, aCodePageNumber);
    }
}

/*
 * aChar must be bigger than 0x80!
 */
void BlueDisplay::setCharacterMapping(uint8_t aChar, uint16_t aUnicodeChar) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_GLOBAL_SETTINGS, 3, SUBFUNCTION_GLOBAL_SET_CHARACTER_CODE_MAPPING, aChar, aUnicodeChar);
    }
}

void BlueDisplay::setLongTouchDownTimeout(uint16_t aLongTouchDownTimeoutMillis) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_GLOBAL_SETTINGS, 2, SUBFUNCTION_GLOBAL_SET_LONG_TOUCH_DOWN_TIMEOUT, aLongTouchDownTimeoutMillis);
    }
}

/**
 * @param aLockMode one of FLAG_SCREEN_ORIENTATION_LOCK_LANDSCAPE, FLAG_SCREEN_ORIENTATION_LOCK_PORTRAIT,
 *         FLAG_SCREEN_ORIENTATION_LOCK_CURRENT or FLAG_SCREEN_ORIENTATION_LOCK_UNLOCK
 */
void BlueDisplay::setScreenOrientationLock(uint8_t aLockMode) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_GLOBAL_SETTINGS, 2, SUBFUNCTION_GLOBAL_SET_SCREEN_ORIENTATION_LOCKATION_LOCK, aLockMode);
    }
}

void BlueDisplay::playTone(void) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_PLAY_TONE, 1, TONE_DEFAULT);
    }
}

/*
 * index is from android.media.ToneGenerator see also
 * http://developer.android.com/reference/android/media/ToneGenerator.html
 */
void BlueDisplay::playTone(uint8_t aToneIndex) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_PLAY_TONE, 1, aToneIndex);
    }
}

/*
 * aToneDuration -1 means forever
 * but except the value -1 aToneDuration is taken as unsigned so -2 will give 65534 micros
 */
void BlueDisplay::playTone(uint8_t aToneIndex, int16_t aToneDuration) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_PLAY_TONE, 2, aToneIndex, aToneDuration);
    }
}

/*
 * aToneDuration -1 means forever
 * but except the value -1 aToneDuration is taken as unsigned so -2 will give 65534 micros
 */
void BlueDisplay::playTone(uint8_t aToneIndex, int16_t aToneDuration, uint8_t aToneVolume) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_PLAY_TONE, 3, aToneIndex, aToneDuration, aToneVolume);
    }
}

void BlueDisplay::playFeedbackTone(uint8_t aToneType) {
    if (aToneType == FEEDBACK_TONE_OK) {
        BlueDisplay1.playTone(TONE_PROP_BEEP_OK);
    } else if (aToneType == FEEDBACK_TONE_ERROR) {
        BlueDisplay1.playTone(TONE_PROP_BEEP_ERROR);
    } else if (aToneType == FEEDBACK_TONE_NO_TONE) {
        return;
    } else {
        BlueDisplay1.playTone(aToneType);
    }
}

void BlueDisplay::clearDisplay(color16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.clearDisplay(aColor);
#endif
    if (USART_isBluetoothPaired()) {
        // saves 8 bytes for DSO, requires 8 bytes for RobotCar
//        sendUSART(SYNC_TOKEN);
//        sendUSART(FUNCTION_CLEAR_DISPLAY);
//        sendUSART('\0');
//        sendUSART('\0');
//        sendUSART(aColor);
//        sendUSART(aColor >> 8);
        // requires 16 bytes for DSO, saves 26 bytes for RobotCar
//        uint16_t tParamBuffer[3];
//        tParamBuffer[0] = FUNCTION_CLEAR_DISPLAY << 8 | SYNC_TOKEN;
//        tParamBuffer[1] = 1;
//        tParamBuffer[2] = aColor;
//        sendUSARTBufferNoSizeCheck((uint8_t*) &tParamBuffer[0], 1 * 2 + 4, NULL, 0);
        sendUSARTArgs(FUNCTION_CLEAR_DISPLAY, 1, aColor);
    }
}

/*
 * If the buffer of the display device is full, commands up to this command may be skipped and display cleared.
 * Useful if we send commands faster than the display may able to handle, to avoid increasing delay between sending and rendering.
 */
void BlueDisplay::clearDisplayOptional(color16_t aColor) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_CLEAR_DISPLAY_OPTIONAL, 1, aColor);
    }
}

// forces an rendering of the drawn bitmap
void BlueDisplay::drawDisplayDirect(void) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_DRAW_DISPLAY, 0);
    }
}

void BlueDisplay::drawPixel(uint16_t aXPos, uint16_t aYPos, color16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.drawPixel(aXPos, aYPos, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_DRAW_PIXEL, 3, aXPos, aYPos, aColor);
    }
}

void BlueDisplay::drawLine(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, color16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.drawLine(aXStart, aYStart, aXEnd, aYEnd, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSART5Args(FUNCTION_DRAW_LINE, aXStart, aYStart, aXEnd, aYEnd, aColor);
    }
}

void BlueDisplay::drawLineRel(uint16_t aXStart, uint16_t aYStart, uint16_t aXDelta, uint16_t aYDelta, color16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.drawLine(aXStart, aYStart, aXStart + aXDelta, aYStart + aYDelta, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSART5Args(FUNCTION_DRAW_LINE_REL, aXStart, aYStart, aXDelta, aYDelta, aColor);
    }
}

/**
 * Fast routine for drawing data charts
 * draws a line only from x to x+1
 * first pixel is omitted because it is drawn by preceding line
 * uses setArea instead if drawPixel to speed up drawing
 */
void BlueDisplay::drawLineFastOneX(uint16_t aXStart, uint16_t aYStart, uint16_t aYEnd, color16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.drawLineFastOneX(aXStart, aYStart, aYEnd, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        // Just draw plain line, no need to speed up
        sendUSART5Args(FUNCTION_DRAW_LINE, aXStart, aYStart, aXStart + 1, aYEnd, aColor);
    }
}

/*
 * aDegree in degree, not radian
 */
void BlueDisplay::drawVectorDegrees(uint16_t aXStart, uint16_t aYStart, uint16_t aLength, int aDegrees, color16_t aColor,
        int16_t aThickness) {

    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_DRAW_VECTOR_DEGREE, 6, aXStart, aYStart, aLength, aDegrees, aColor, aThickness);
    }
}

/*
 * aRadian in radian, not degree
 */
void BlueDisplay::drawVectorRadian(uint16_t aXStart, uint16_t aYStart, uint16_t aLength, float aRadian, color16_t aColor,
        int16_t aThickness) {

    if (USART_isBluetoothPaired()) {
        union {
            float floatValue;
            uint16_t shortArray[2];
        } floatToShortArray;
        floatToShortArray.floatValue = aRadian;
        sendUSARTArgs(FUNCTION_DRAW_VECTOR_DEGREE, 7, aXStart, aYStart, aLength, floatToShortArray.shortArray[0],
                floatToShortArray.shortArray[1], aColor, aThickness);
    }
}

void BlueDisplay::drawLineWithThickness(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, int16_t aThickness,
        color16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    drawThickLine(aXStart, aYStart, aXEnd, aYEnd, aThickness, LINE_THICKNESS_MIDDLE, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_DRAW_LINE, 6, aXStart, aYStart, aXEnd, aYEnd, aColor, aThickness);
    }
}

void BlueDisplay::drawLineRelWithThickness(uint16_t aXStart, uint16_t aYStart, uint16_t aXDelta, uint16_t aYDelta,
        int16_t aThickness, color16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    drawThickLine(aXStart, aYStart, aXDelta, aYDelta, aThickness, LINE_THICKNESS_MIDDLE, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_DRAW_LINE_REL, 6, aXStart, aYStart, aXDelta, aYDelta, aColor, aThickness);
    }
}

void BlueDisplay::drawRect(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, color16_t aColor,
        uint16_t aStrokeWidth) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.drawRect(aXStart, aYStart, aXEnd - 1, aYEnd - 1, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_DRAW_RECT, 6, aXStart, aYStart, aXEnd, aYEnd, aColor, aStrokeWidth);
    }
}

void BlueDisplay::drawRectRel(uint16_t aXStart, uint16_t aYStart, uint16_t aWidth, uint16_t aHeight, color16_t aColor,
        uint16_t aStrokeWidth) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.drawRect(aXStart, aYStart, aXStart + aWidth - 1, aYStart + aHeight - 1, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_DRAW_RECT_REL, 6, aXStart, aYStart, aWidth, aHeight, aColor, aStrokeWidth);
    }
}

void BlueDisplay::fillRect(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, color16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.fillRect(aXStart, aYStart, aXEnd, aYEnd, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSART5Args(FUNCTION_FILL_RECT, aXStart, aYStart, aXEnd, aYEnd, aColor);
    }
}

void BlueDisplay::fillRectRel(uint16_t aXStart, uint16_t aYStart, uint16_t aWidth, uint16_t aHeight, color16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.fillRect(aXStart, aYStart, aXStart + aWidth - 1, aYStart + aHeight - 1, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSART5Args(FUNCTION_FILL_RECT_REL, aXStart, aYStart, aWidth, aHeight, aColor);
    }
}

void BlueDisplay::drawCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, color16_t aColor, uint16_t aStrokeWidth) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.drawCircle(aXCenter, aYCenter, aRadius, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSART5Args(FUNCTION_DRAW_CIRCLE, aXCenter, aYCenter, aRadius, aColor, aStrokeWidth);
    }
}

void BlueDisplay::fillCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, color16_t aColor) {
#ifdef LOCAL_DISPLAY_EXISTS
    LocalDisplay.fillCircle(aXCenter, aYCenter, aRadius, aColor);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_FILL_CIRCLE, 4, aXCenter, aYCenter, aRadius, aColor);
    }
}

/**
 * @return start x for next character / x + (TEXT_SIZE_11_WIDTH * size)
 */
uint16_t BlueDisplay::drawChar(uint16_t aPosX, uint16_t aPosY, char aChar, uint16_t aCharSize, color16_t aFGColor,
        color16_t aBGColor) {
    uint16_t tRetValue = 0;
#ifdef LOCAL_DISPLAY_EXISTS
    tRetValue = LocalDisplay.drawChar(aPosX, aPosY - getTextAscend(aCharSize), aChar, getLocalTextSize(aCharSize), aFGColor,
            aBGColor);
#endif
    if (USART_isBluetoothPaired()) {
        tRetValue = aPosX + getTextWidth(aCharSize);
        sendUSARTArgs(FUNCTION_DRAW_CHAR, 6, aPosX, aPosY, aCharSize, aFGColor, aBGColor, aChar);
    }
    return tRetValue;
}

/**
 * @param aPosX left position
 * @param aPosY baseline position - use (upper_position + getTextAscend(<aTextSize>))
 * @param aStringPtr  If /r is used as newline character, rest of line will be cleared, if /n is used, rest of line will not be cleared.
 * @param aTextSize FontSize of text
 * @param aFGColor Foreground/text color
 * @param aBGColor if COLOR16_NO_BACKGROUND, then the background will not filled
 * @return uint16_t start x for next character - next x Parameter
 */
uint16_t BlueDisplay::drawText(uint16_t aPosX, uint16_t aPosY, const char *aStringPtr, uint16_t aTextSize, color16_t aFGColor,
        color16_t aBGColor) {
    uint16_t tRetValue = 0;
#ifdef LOCAL_DISPLAY_EXISTS
    tRetValue = LocalDisplay.drawText(aPosX, aPosY - getTextAscend(aTextSize), (char *) aStringPtr, getLocalTextSize(aTextSize),
            aFGColor, aBGColor);
#endif
    if (USART_isBluetoothPaired()) {
        tRetValue = aPosX + strlen(aStringPtr) * getTextWidth(aTextSize);
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPosX, aPosY, aTextSize, aFGColor, aBGColor, strlen(aStringPtr),
                (uint8_t*) aStringPtr);
    }
    return tRetValue;
}

/*
 * Take size and colors from preceding drawText command
 */
void BlueDisplay::drawText(uint16_t aPosX, uint16_t aPosY, const char *aStringPtr) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 2, aPosX, aPosY, strlen(aStringPtr), (uint8_t*) aStringPtr);
    }
}

uint16_t BlueDisplay::drawByte(uint16_t aPosX, uint16_t aPosY, int8_t aByte, uint16_t aTextSize, color16_t aFGColor,
        color16_t aBGColor) {
    uint16_t tRetValue = 0;
    char tStringBuffer[5];
#ifdef AVR
    sprintf_P(tStringBuffer, PSTR("%4hhd"), aByte);
#else
    sprintf(tStringBuffer, "%4hhd", aByte);
#endif
#ifdef LOCAL_DISPLAY_EXISTS
    tRetValue = LocalDisplay.drawText(aPosX, aPosY - getTextAscend(aTextSize), tStringBuffer, getLocalTextSize(aTextSize), aFGColor,
            aBGColor);
#endif
    if (USART_isBluetoothPaired()) {
        tRetValue = aPosX + 4 * getTextWidth(aTextSize);
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPosX, aPosY, aTextSize, aFGColor, aBGColor, 4,
                (uint8_t*) tStringBuffer);
    }
    return tRetValue;
}

uint16_t BlueDisplay::drawUnsignedByte(uint16_t aPosX, uint16_t aPosY, uint8_t aUnsignedByte, uint16_t aTextSize,
        color16_t aFGColor, color16_t aBGColor) {
    uint16_t tRetValue = 0;
    char tStringBuffer[4];
#ifdef AVR
    sprintf_P(tStringBuffer, PSTR("%3u"), aUnsignedByte);
#else
    sprintf(tStringBuffer, "%3u", aUnsignedByte);
#endif
#ifdef LOCAL_DISPLAY_EXISTS
    tRetValue = LocalDisplay.drawText(aPosX, aPosY - getTextAscend(aTextSize), tStringBuffer, getLocalTextSize(aTextSize), aFGColor,
            aBGColor);
#endif
    if (USART_isBluetoothPaired()) {
        tRetValue = aPosX + 3 * getTextWidth(aTextSize);
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPosX, aPosY, aTextSize, aFGColor, aBGColor, 3,
                (uint8_t*) tStringBuffer);
    }
    return tRetValue;
}

uint16_t BlueDisplay::drawShort(uint16_t aPosX, uint16_t aPosY, int16_t aShort, uint16_t aTextSize, color16_t aFGColor,
        color16_t aBGColor) {
    uint16_t tRetValue = 0;
    char tStringBuffer[7];
#ifdef AVR
    sprintf_P(tStringBuffer, PSTR("%6hd"), aShort);
#else
    sprintf(tStringBuffer, "%6hd", aShort);
#endif
#ifdef LOCAL_DISPLAY_EXISTS
    tRetValue = LocalDisplay.drawText(aPosX, aPosY - getTextAscend(aTextSize), tStringBuffer, getLocalTextSize(aTextSize), aFGColor,
            aBGColor);
#endif
    if (USART_isBluetoothPaired()) {
        tRetValue = aPosX + 6 * getTextWidth(aTextSize);
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPosX, aPosY, aTextSize, aFGColor, aBGColor, 6,
                (uint8_t*) tStringBuffer);
    }
    return tRetValue;
}

uint16_t BlueDisplay::drawLong(uint16_t aPosX, uint16_t aPosY, int32_t aLong, uint16_t aTextSize, color16_t aFGColor,
        color16_t aBGColor) {
    uint16_t tRetValue = 0;
    char tStringBuffer[12];
#ifdef AVR
    sprintf_P(tStringBuffer, PSTR("%11ld"), aLong);
#elif defined(__XTENSA__)
    sprintf(tStringBuffer, "%11ld", (long)aLong);
#else
    sprintf(tStringBuffer, "%11ld", aLong);
#endif
#ifdef LOCAL_DISPLAY_EXISTS
    tRetValue = LocalDisplay.drawText(aPosX, aPosY - getTextAscend(aTextSize), tStringBuffer, getLocalTextSize(aTextSize), aFGColor,
            aBGColor);
#endif
    if (USART_isBluetoothPaired()) {
        tRetValue = aPosX + 11 * getTextWidth(aTextSize);
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPosX, aPosY, aTextSize, aFGColor, aBGColor, 11,
                (uint8_t*) tStringBuffer);
    }
    return tRetValue;
}

/*
 * for writeString implementation
 */
void BlueDisplay::setWriteStringSizeAndColorAndFlag(uint16_t aPrintSize, color16_t aPrintColor, color16_t aPrintBackgroundColor,
bool aClearOnNewScreen) {
#ifdef LOCAL_DISPLAY_EXISTS
    printSetOptions(getLocalTextSize(aPrintSize), aPrintColor, aPrintBackgroundColor, aClearOnNewScreen);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSART5Args(FUNCTION_WRITE_SETTINGS, FLAG_WRITE_SETTINGS_SET_SIZE_AND_COLORS_AND_FLAGS, aPrintSize, aPrintColor,
                aPrintBackgroundColor, aClearOnNewScreen);
    }
}

void BlueDisplay::setWriteStringPosition(uint16_t aPosX, uint16_t aPosY) {
#ifdef LOCAL_DISPLAY_EXISTS
    printSetPosition(aPosX, aPosY);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_WRITE_SETTINGS, 3, FLAG_WRITE_SETTINGS_SET_POSITION, aPosX, aPosY);
    }
}

void BlueDisplay::setWriteStringPositionColumnLine(uint16_t aColumnNumber, uint16_t aLineNumber) {
#ifdef LOCAL_DISPLAY_EXISTS
    printSetPositionColumnLine(aColumnNumber, aLineNumber);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_WRITE_SETTINGS, 3, FLAG_WRITE_SETTINGS_SET_LINE_COLUMN, aColumnNumber, aLineNumber);
    }
}

void BlueDisplay::writeString(const char *aStringPtr, uint8_t aStringLength) {
#ifdef LOCAL_DISPLAY_EXISTS
    myPrint(aStringPtr, aStringLength);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_WRITE_STRING, 0, aStringLength, (uint8_t*) aStringPtr);
    }
}

// for use in syscalls.c
extern "C" void writeStringC(const char *aStringPtr, uint8_t aStringLength) {
#ifdef LOCAL_DISPLAY_EXISTS
    myPrint(aStringPtr, aStringLength);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_WRITE_STRING, 0, aStringLength, (uint8_t*) aStringPtr);
    }
}

/**
 * Output String as warning to log and present as toast every 500 ms
 */
void BlueDisplay::debugMessage(const char *aStringPtr) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(aStringPtr), (uint8_t*) aStringPtr);
    }
}

void BlueDisplay::debug(const char *aStringPtr) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(aStringPtr), (uint8_t*) aStringPtr);
    }
}

/**
 * Output as warning to log and present as toast every 500 ms
 */
void BlueDisplay::debug(uint8_t aByte) {
    char tStringBuffer[9];
// hhu -> unsigned char instead of unsigned int with u
#ifdef AVR
    sprintf_P(tStringBuffer, PSTR("%3hhu 0x%2.2hhX"), aByte, aByte);
#else
    sprintf(tStringBuffer, "%3hhu 0x%2.2hhX", aByte, aByte);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessage string is 25 character.
 */
void BlueDisplay::debug(const char *aMessage, uint8_t aByte) {
    char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE];
// hhu -> unsigned char instead of unsigned int with u
#ifdef AVR
    snprintf_P(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, PSTR("%s%3hhu 0x%2.2hhX"), aMessage, aByte, aByte);
#else
    snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%3hhu 0x%2.2hhX", aMessage, aByte, aByte);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessage string is 24 character.
 */
void BlueDisplay::debug(const char *aMessage, int8_t aByte) {
    char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE];
// hhd -> signed char instead of signed int with d
#ifdef AVR
    snprintf_P(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, PSTR("%s%4hhd 0x%2.2hhX"), aMessage, aByte, aByte);
#else
    snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%4hhd 0x%2.2hhX", aMessage, aByte, aByte);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

void BlueDisplay::debug(int8_t aByte) {
    char tStringBuffer[10];
// hhd -> signed char instead of int with d
#ifdef AVR
    sprintf_P(tStringBuffer, PSTR("%4hhd 0x%2.2hhX"), aByte, aByte);
#else
    sprintf(tStringBuffer, "%4hhd 0x%2.2hhX", aByte, aByte);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

void BlueDisplay::debug(uint16_t aShort) {
    char tStringBuffer[13]; //5 decimal + 3 " 0x" + 4 hex +1
// hu -> unsigned short int instead of unsigned int with u
#ifdef AVR
    sprintf_P(tStringBuffer, PSTR("%5u 0x%4.4X"), aShort, aShort);
#else
    sprintf(tStringBuffer, "%5hu 0x%4.4X", aShort, aShort);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

void BlueDisplay::debug(int16_t aShort) {
    char tStringBuffer[14]; //6 decimal + 3 " 0x" + 4 hex +1
// hd -> short int instead of int with d
#ifdef AVR
    sprintf_P(tStringBuffer, PSTR("%6d 0x%4.4X"), aShort, aShort);
#else
    sprintf(tStringBuffer, "%6hd 0x%4.4X", aShort, aShort);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessage string is 21 character.
 */
void BlueDisplay::debug(const char *aMessage, uint16_t aShort) {
    char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE];
// hd -> short int instead of int with d
#ifdef AVR
    snprintf_P(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, PSTR("%s%5u 0x%4.4X"), aMessage, aShort, aShort);
#else
    snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%5hu 0x%4.4X", aMessage, aShort, aShort);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessage string is 20 character.
 */
void BlueDisplay::debug(const char *aMessage, int16_t aShort) {
    char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE];
// hd -> short int instead of int with d
#ifdef AVR
    snprintf_P(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, PSTR("%s%6d 0x%4.4X"), aMessage, aShort, aShort);
#else
    snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%6hd 0x%4.4X", aMessage, aShort, aShort);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

void BlueDisplay::debug(uint32_t aLong) {
    char tStringBuffer[22]; //10 decimal + 3 " 0x" + 8 hex +1
#ifdef AVR
    sprintf_P(tStringBuffer, PSTR("%10lu 0x%lX"), aLong, aLong);
#elif defined(__XTENSA__)
    sprintf(tStringBuffer, "%10lu 0x%lX", (long)aLong, (long)aLong);
#else
    sprintf(tStringBuffer, "%10lu 0x%lX", aLong, aLong);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

void BlueDisplay::debug(int32_t aLong) {
    char tStringBuffer[23]; //11 decimal + 3 " 0x" + 8 hex +1
#ifdef AVR
    sprintf_P(tStringBuffer, PSTR("%11ld 0x%lX"), aLong, aLong);
#elif defined(__XTENSA__)
    sprintf(tStringBuffer, "%11ld 0x%lX", (long)aLong, (long)aLong);
#else
    sprintf(tStringBuffer, "%11ld 0x%lX", aLong, aLong);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessage string is 13 to 20 character depending on content of aLong.
 */
void BlueDisplay::debug(const char *aMessage, uint32_t aLong) {
    char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE];
#ifdef AVR
    snprintf_P(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, PSTR("%s%10lu 0x%lX"), aMessage, aLong, aLong);
#elif defined(__XTENSA__)
    snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%10lu 0x%lX", aMessage, (long)aLong, (long)aLong);
#else
    snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%10lu 0x%lX", aMessage, aLong, aLong);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessage string is 12 to 19 character depending on content of aLong.
 */
void BlueDisplay::debug(const char *aMessage, int32_t aLong) {
    char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE];
#ifdef AVR
    snprintf_P(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, PSTR("%s%11ld 0x%lX"), aMessage, aLong, aLong);
#elif defined(__XTENSA__)
    snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%11ld 0x%lX", aMessage, (long)aLong, (long)aLong);
#else
    snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%11ld 0x%lX", aMessage, aLong, aLong);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

void BlueDisplay::debug(float aFloat) {
    char tStringBuffer[22];
#ifdef AVR
    dtostrf(aFloat, 16, 7, tStringBuffer);
#else
    sprintf(tStringBuffer, "%f", aFloat);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessage string is up to 30 character depending on content of aFloat.
 */
void BlueDisplay::debug(const char *aMessage, float aFloat) {
    char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE];
#ifdef AVR
    strncpy(tStringBuffer, aMessage, (STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE - 22));
    dtostrf(aFloat, 16, 7, &tStringBuffer[strlen(tStringBuffer)]);
//    snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%f", aMessage, (double)aFloat); // requires ca. 800 Bytes more
#else
    snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%f", aMessage, aFloat);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

void BlueDisplay::debug(double aDouble) {
    char tStringBuffer[22];
#ifdef AVR
    dtostrf(aDouble, 16, 7, tStringBuffer);
#else
    sprintf(tStringBuffer, "%f", aDouble);
#endif
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/**
 * if aClearBeforeColor != 0 then previous line is cleared before
 */
void BlueDisplay::drawChartByteBuffer(uint16_t aXOffset, uint16_t aYOffset, color16_t aColor, color16_t aClearBeforeColor,
        uint8_t *aByteBuffer, size_t aByteBufferLength) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_CHART, 4, aXOffset, aYOffset, aColor, aClearBeforeColor, aByteBufferLength,
                aByteBuffer);
    }
}

/**
 * if aClearBeforeColor != 0 then previous line is cleared before
 * chart index is coded in the upper 4 bits of aYOffset
 */
void BlueDisplay::drawChartByteBuffer(uint16_t aXOffset, uint16_t aYOffset, color16_t aColor, color16_t aClearBeforeColor,
        uint8_t aChartIndex, bool aDoDrawDirect, uint8_t *aByteBuffer, size_t aByteBufferLength) {
    if (USART_isBluetoothPaired()) {
        aYOffset = aYOffset | ((aChartIndex & 0x0F) << 12);
        uint8_t tFunctionTag = FUNCTION_DRAW_CHART_WITHOUT_DIRECT_RENDERING;
        if (aDoDrawDirect) {
            tFunctionTag = FUNCTION_DRAW_CHART;
        }
        sendUSARTArgsAndByteBuffer(tFunctionTag, 4, aXOffset, aYOffset, aColor, aClearBeforeColor, aByteBufferLength, aByteBuffer);
    }
}

struct XYSize* BlueDisplay::getMaxDisplaySize(void) {
    return &mMaxDisplaySize;
}

uint16_t BlueDisplay::getMaxDisplayWidth(void) {
    return mMaxDisplaySize.XWidth;
}

uint16_t BlueDisplay::getMaxDisplayHeight(void) {
    return mMaxDisplaySize.YHeight;
}

struct XYSize* BlueDisplay::getCurrentDisplaySize(void) {
    return &mCurrentDisplaySize;
}

uint16_t BlueDisplay::getCurrentDisplayWidth(void) {
    return mCurrentDisplaySize.XWidth;
}

uint16_t BlueDisplay::getCurrentDisplayHeight(void) {
    return mCurrentDisplaySize.YHeight;
}

struct XYSize* BlueDisplay::getRequestedDisplaySize(void) {
    return &mRequestedDisplaySize;
}

uint16_t BlueDisplay::getDisplayWidth(void) {
    return mRequestedDisplaySize.XWidth;
}

uint16_t BlueDisplay::getDisplayHeight(void) {
    return mRequestedDisplaySize.YHeight;
}

bool BlueDisplay::isDisplayOrientationLandscape(void) {
    return mOrientationIsLandscape;
}

/*****************************************************************************
 * Vector for ThickLine
 *****************************************************************************/

/**
 * aNewRelEndX + Y are new x and y values relative to start point
 */
void BlueDisplay::refreshVector(struct ThickLine *aLine, int16_t aNewRelEndX, int16_t aNewRelEndY) {
    int16_t tNewEndX = aLine->StartX + aNewRelEndX;
    int16_t tNewEndY = aLine->StartY + aNewRelEndY;
    if (aLine->EndX != tNewEndX || aLine->EndX != tNewEndY) {
        //clear old line
        drawLineWithThickness(aLine->StartX, aLine->StartY, aLine->EndX, aLine->EndY, aLine->Thickness, aLine->BackgroundColor);
        // Draw new line
        /**
         * clipping
         * Ignore warning since we know that values are positive when compared :-)
         */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
        if (tNewEndX < 0) {
            tNewEndX = 0;
        } else if (tNewEndX > mRequestedDisplaySize.XWidth - 1) {
            tNewEndX = mRequestedDisplaySize.XWidth - 1;
        }
        aLine->EndX = tNewEndX;

        if (tNewEndY < 0) {
            tNewEndY = 0;
        } else if (tNewEndY > mRequestedDisplaySize.YHeight - 1) {
            tNewEndY = mRequestedDisplaySize.YHeight - 1;
        }
#pragma GCC diagnostic pop
        aLine->EndY = tNewEndY;

        drawLineWithThickness(aLine->StartX, aLine->StartY, tNewEndX, tNewEndY, aLine->Thickness, aLine->Color);
    }
}

// for use in syscalls.c
extern "C" uint16_t drawTextC(uint16_t aXStart, uint16_t aYStart, const char *aStringPtr, uint16_t aFontSize, color16_t aFGColor,
        uint16_t aBGColor) {
    uint16_t tRetValue = 0;
    if (USART_isBluetoothPaired()) {
        tRetValue = BlueDisplay1.drawText(aXStart, aYStart, (char*) aStringPtr, aFontSize, aFGColor, aBGColor);
    }
    return tRetValue;
}

#ifdef LOCAL_DISPLAY_EXISTS
/**
 * @param aBGColor if COLOR16_NO_BACKGROUND, then do not clear rest of line
 */
void BlueDisplay::drawMLText(uint16_t aPosX, uint16_t aPosY, const char *aStringPtr, uint16_t aTextSize, color16_t aFGColor,
        color16_t aBGColor) {

    LocalDisplay.drawMLText(aPosX, aPosY - getTextAscend(aTextSize), (char *) aStringPtr, getLocalTextSize(aTextSize), aFGColor,
            aBGColor);
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPosX, aPosY, aTextSize, aFGColor, aBGColor, strlen(aStringPtr),
                (uint8_t*) aStringPtr);
    }
}
#endif

#ifdef AVR
uint16_t BlueDisplay::drawTextPGM(uint16_t aPosX, uint16_t aPosY, const char *aPGMString, uint16_t aTextSize, color16_t aFGColor,
        color16_t aBGColor) {
    uint16_t tRetValue = 0;
    uint8_t tTextLength = strlen_P(aPGMString);
    if (tTextLength > STRING_BUFFER_STACK_SIZE) {
        tTextLength = STRING_BUFFER_STACK_SIZE;
    }
    char tStringBuffer[STRING_BUFFER_STACK_SIZE];
    strncpy_P(tStringBuffer, aPGMString, tTextLength);
#ifdef LOCAL_DISPLAY_EXISTS
    tRetValue = LocalDisplay.drawTextPGM(aPosX, aPosY - getTextAscend(aTextSize), aPGMString, getLocalTextSize(aTextSize), aFGColor,
            aBGColor);
#endif
    if (USART_isBluetoothPaired()) {
        tRetValue = aPosX + tTextLength * getTextWidth(aTextSize);
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPosX, aPosY, aTextSize, aFGColor, aBGColor, tTextLength,
                (uint8_t*) tStringBuffer);
    }
    return tRetValue;
}

void BlueDisplay::drawTextPGM(uint16_t aPosX, uint16_t aPosY, const char *aPGMString) {
    uint8_t tTextLength = strlen_P(aPGMString);
    if (tTextLength > STRING_BUFFER_STACK_SIZE) {
        tTextLength = STRING_BUFFER_STACK_SIZE;
    }
    char tStringBuffer[STRING_BUFFER_STACK_SIZE];
    strncpy_P(tStringBuffer, aPGMString, tTextLength);
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 2, aPosX, aPosY, tTextLength, (uint8_t*) tStringBuffer);
    }
}

uint16_t BlueDisplay::drawText(uint16_t aPosX, uint16_t aPosY, const __FlashStringHelper *aPGMString, uint16_t aTextSize,
        color16_t aFGColor, color16_t aBGColor) {
    uint16_t tRetValue = 0;
    PGM_P tPGMString = reinterpret_cast<PGM_P>(aPGMString);

    uint8_t tTextLength = strlen_P(tPGMString);
    if (tTextLength > STRING_BUFFER_STACK_SIZE) {
        tTextLength = STRING_BUFFER_STACK_SIZE;
    }
    char tStringBuffer[STRING_BUFFER_STACK_SIZE];
    strncpy_P(tStringBuffer, tPGMString, tTextLength);
#ifdef LOCAL_DISPLAY_EXISTS
    tRetValue = LocalDisplay.drawTextPGM(aPosX, aPosY - getTextAscend(aTextSize), tPGMString, getLocalTextSize(aTextSize), aFGColor,
            aBGColor);
#endif
    if (USART_isBluetoothPaired()) {
        tRetValue = aPosX + tTextLength * getTextWidth(aTextSize);
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPosX, aPosY, aTextSize, aFGColor, aBGColor, tTextLength,
                (uint8_t*) tStringBuffer);
    }
    return tRetValue;
}

void BlueDisplay::drawText(uint16_t aPosX, uint16_t aPosY, const __FlashStringHelper *aPGMString) {
    PGM_P tPGMString = reinterpret_cast<PGM_P>(aPGMString);

    uint8_t tTextLength = strlen_P(tPGMString);
    if (tTextLength > STRING_BUFFER_STACK_SIZE) {
        tTextLength = STRING_BUFFER_STACK_SIZE;
    }
    char tStringBuffer[STRING_BUFFER_STACK_SIZE];
    strncpy_P(tStringBuffer, tPGMString, tTextLength);
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 2, aPosX, aPosY, tTextLength, (uint8_t*) tStringBuffer);
    }
}
#endif

/***************************************************************************************************************************************************
 *
 * INPUT
 *
 **************************************************************************************************************************************************/
/*
 * If the user enters avalid number and presses OK, it sends a message over bluetooth back to arduino which contains the float value
 */
void BlueDisplay::getNumber(void (*aNumberHandler)(float)) {
    if (USART_isBluetoothPaired()) {
#if __SIZEOF_POINTER__ == 4
        sendUSARTArgs(FUNCTION_GET_NUMBER, 2, aNumberHandler, (reinterpret_cast<uint32_t>(aNumberHandler) >> 16));
#else
        sendUSARTArgs(FUNCTION_GET_NUMBER, 1, aNumberHandler);
#endif
    }
}

/*
 * Message size 1 or 2 shorts
 */
void BlueDisplay::getNumberWithShortPrompt(void (*aNumberHandler)(float), const char *aShortPromptString) {
    if (USART_isBluetoothPaired()) {
#if __SIZEOF_POINTER__ == 4
        sendUSARTArgsAndByteBuffer(FUNCTION_GET_NUMBER_WITH_SHORT_PROMPT, 2, aNumberHandler,
                (reinterpret_cast<uint32_t>(aNumberHandler) >> 16), strlen(aShortPromptString), (uint8_t*) aShortPromptString);
#else
        sendUSARTArgsAndByteBuffer(FUNCTION_GET_NUMBER_WITH_SHORT_PROMPT, 1, aNumberHandler, strlen(aShortPromptString),
                (uint8_t*) aShortPromptString);
#endif
    }
}

/*
 * Message size is 3 (__AVR__) or 4 shorts
 * If cancelled on the Host, nothing is sent back
 */
void BlueDisplay::getNumberWithShortPrompt(void (*aNumberHandler)(float), const char *aShortPromptString, float aInitialValue) {
    if (USART_isBluetoothPaired()) {
        union {
            float floatValue;
            uint16_t shortArray[2];
        } floatToShortArray;
        floatToShortArray.floatValue = aInitialValue;
#if __SIZEOF_POINTER__ == 4
        sendUSARTArgsAndByteBuffer(FUNCTION_GET_NUMBER_WITH_SHORT_PROMPT, 4, aNumberHandler,
                (reinterpret_cast<uint32_t>(aNumberHandler) >> 16), floatToShortArray.shortArray[0],
                floatToShortArray.shortArray[1], strlen(aShortPromptString), (uint8_t*) aShortPromptString);
#else
        sendUSARTArgsAndByteBuffer(FUNCTION_GET_NUMBER_WITH_SHORT_PROMPT, 3, aNumberHandler, floatToShortArray.shortArray[0],
                floatToShortArray.shortArray[1], strlen(aShortPromptString), (uint8_t*) aShortPromptString);
#endif
    }
}

//void BlueDisplay::getText(void (*aTextHandler)(char *)) {
//    if (USART_isBluetoothPaired()) {
//#if __SIZEOF_POINTER__ == 4
//        sendUSARTArgs(FUNCTION_GET_TEXT, 2, aTextHandler, (reinterpret_cast<uint32_t>(aTextHandler) >> 16));
//#else
//        sendUSARTArgs(FUNCTION_GET_TEXT, 1, aTextHandler);
//#endif
//    }
//}

/*
 *  This results in an info event
 */
void BlueDisplay::getInfo(uint8_t aInfoSubcommand, void (*aInfoHandler)(uint8_t, uint8_t, uint16_t, ByteShortLongFloatUnion)) {
    if (USART_isBluetoothPaired()) {
#if __SIZEOF_POINTER__ == 4
        sendUSARTArgs(FUNCTION_GET_INFO, 3, aInfoSubcommand, aInfoHandler, (reinterpret_cast<uint32_t>(aInfoHandler) >> 16));
#else
        sendUSARTArgs(FUNCTION_GET_INFO, 2, aInfoSubcommand, aInfoHandler);
#endif
    }
}

/*
 *  This results in a data event
 */
void BlueDisplay::requestMaxCanvasSize(void) {
    sendUSARTArgs(FUNCTION_REQUEST_MAX_CANVAS_SIZE, 0);
}

#ifdef AVR
void BlueDisplay::getNumberWithShortPromptPGM(void (*aNumberHandler)(float), const char *aPGMShortPromptString) {
    if (USART_isBluetoothPaired()) {
        uint8_t tShortPromptLength = strlen_P(aPGMShortPromptString);
        if (tShortPromptLength > STRING_BUFFER_STACK_SIZE) {
            tShortPromptLength = STRING_BUFFER_STACK_SIZE;
        }
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        strncpy_P(tStringBuffer, aPGMShortPromptString, tShortPromptLength);
        sendUSARTArgsAndByteBuffer(FUNCTION_GET_NUMBER_WITH_SHORT_PROMPT, 1, aNumberHandler, tShortPromptLength,
                (uint8_t*) tStringBuffer);
    }
}

void BlueDisplay::getNumberWithShortPrompt(void (*aNumberHandler)(float), const __FlashStringHelper *aPGMShortPromptString) {
    if (USART_isBluetoothPaired()) {
        PGM_P tPGMShortPromptString = reinterpret_cast<PGM_P>(aPGMShortPromptString);

        uint8_t tShortPromptLength = strlen_P(tPGMShortPromptString);
        if (tShortPromptLength > STRING_BUFFER_STACK_SIZE) {
            tShortPromptLength = STRING_BUFFER_STACK_SIZE;
        }
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        strncpy_P(tStringBuffer, tPGMShortPromptString, tShortPromptLength);
        sendUSARTArgsAndByteBuffer(FUNCTION_GET_NUMBER_WITH_SHORT_PROMPT, 1, aNumberHandler, tShortPromptLength,
                (uint8_t*) tStringBuffer);
    }
}

void BlueDisplay::getNumberWithShortPromptPGM(void (*aNumberHandler)(float), const char *aPGMShortPromptString,
        float aInitialValue) {
    if (USART_isBluetoothPaired()) {
        uint8_t tShortPromptLength = strlen_P(aPGMShortPromptString);
        if (tShortPromptLength > STRING_BUFFER_STACK_SIZE) {
            tShortPromptLength = STRING_BUFFER_STACK_SIZE;
        }
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        strncpy_P(tStringBuffer, aPGMShortPromptString, tShortPromptLength);
        union {
            float floatValue;
            uint16_t shortArray[2];
        } floatToShortArray;
        floatToShortArray.floatValue = aInitialValue;
        sendUSARTArgsAndByteBuffer(FUNCTION_GET_NUMBER_WITH_SHORT_PROMPT, 3, aNumberHandler, floatToShortArray.shortArray[0],
                floatToShortArray.shortArray[1], tShortPromptLength, (uint8_t*) tStringBuffer);
    }
}

void BlueDisplay::getNumberWithShortPrompt(void (*aNumberHandler)(float), const __FlashStringHelper *aPGMShortPromptString,
        float aInitialValue) {
    if (USART_isBluetoothPaired()) {
        PGM_P tPGMShortPromptString = reinterpret_cast<PGM_P>(aPGMShortPromptString);

        uint8_t tShortPromptLength = strlen_P(tPGMShortPromptString);
        if (tShortPromptLength > STRING_BUFFER_STACK_SIZE) {
            tShortPromptLength = STRING_BUFFER_STACK_SIZE;
        }
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        strncpy_P(tStringBuffer, tPGMShortPromptString, tShortPromptLength);
        union {
            float floatValue;
            uint16_t shortArray[2];
        }floatToShortArray;
        floatToShortArray.floatValue = aInitialValue;
        sendUSARTArgsAndByteBuffer(FUNCTION_GET_NUMBER_WITH_SHORT_PROMPT, 3, aNumberHandler, floatToShortArray.shortArray[0],
                floatToShortArray.shortArray[1], tShortPromptLength, (uint8_t*) tStringBuffer);
    }
}

//void BlueDisplay::getTextWithShortPromptPGM(void (*aTextHandler)(const char *), const char *aPGMShortPromptString) {
//    if (USART_isBluetoothPaired()) {
//        uint8_t tShortPromptLength = strlen_P(aPGMShortPromptString);
//        if (tShortPromptLength < STRING_BUFFER_STACK_SIZE) {
//            char tStringBuffer[STRING_BUFFER_STACK_SIZE];
//            strcpy_P(tStringBuffer, aPGMShortPromptString);
//
//#if __SIZEOF_POINTER__ == 4
//            sendUSARTArgsAndByteBuffer(FUNCTION_GET_TEXT_WITH_SHORT_PROMPT, 2, aTextHandler,
//                    (reinterpret_cast<uint32_t>(aTextHandler) >> 16), tShortPromptLength, (uint8_t*) tStringBuffer);
//#else
//            sendUSARTArgsAndByteBuffer(FUNCTION_GET_TEXT_WITH_SHORT_PROMPT, 1, aTextHandler, tShortPromptLength, (uint8_t*) tStringBuffer);
//#endif
//        }
//    }
//}
#endif

/***************************************************************************************************************************************************
 *
 * SENSOR
 *
 **************************************************************************************************************************************************/

/**
 *
 * @param aSensorType one of FLAG_SENSOR_TYPE_ACCELEROMETER, FLAG_SENSOR_TYPE_GYROSCOPE
 * @param aSensorRate one of  {@link #FLAG_SENSOR_DELAY_NORMAL} 200 ms, {@link #FLAG_SENSOR_DELAY_UI} 60 ms,
 *        {@link #FLAG_SENSOR_DELAY_GAME} 20ms, or {@link #FLAG_SENSOR_DELAY_FASTEST}
 */
void BlueDisplay::setSensor(uint8_t aSensorType, bool aDoActivate, uint8_t aSensorRate, uint8_t aFilterFlag) {
    aSensorRate &= 0x03;
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SENSOR_SETTINGS, 4, aSensorType, aDoActivate, aSensorRate, aFilterFlag);
    }
}

/***************************************************************************************************************************************************
 *
 * BUTTONS
 *
 **************************************************************************************************************************************************/

BDButtonHandle_t BlueDisplay::createButton(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY,
        color16_t aButtonColor, const char *aCaption, uint8_t aCaptionSize, uint8_t aFlags, int16_t aValue,
        void (*aOnTouchHandler)(BDButton*, int16_t)) {
    BDButtonHandle_t tButtonNumber = sLocalButtonIndex++;

    if (USART_isBluetoothPaired()) {
#if __SIZEOF_POINTER__ == 4
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 10, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
                aButtonColor, aCaptionSize | (aFlags << 8), aValue, aOnTouchHandler,
                (reinterpret_cast<uint32_t>(aOnTouchHandler) >> 16), strlen(aCaption), aCaption);
#else
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 9, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
                aButtonColor, aCaptionSize | (aFlags << 8), aValue, aOnTouchHandler, strlen(aCaption), aCaption);
#endif
    }
    return tButtonNumber;
}

void BlueDisplay::drawButton(BDButtonHandle_t aButtonNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_DRAW, 1, aButtonNumber);
    }
}

void BlueDisplay::removeButton(BDButtonHandle_t aButtonNumber, color16_t aBackgroundColor) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_REMOVE, 2, aButtonNumber, aBackgroundColor);
    }
}

void BlueDisplay::drawButtonCaption(BDButtonHandle_t aButtonNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_DRAW_CAPTION, 1, aButtonNumber);
    }
}

void BlueDisplay::setButtonCaption(BDButtonHandle_t aButtonNumber, const char *aCaption, bool doDrawButton) {
    if (USART_isBluetoothPaired()) {
        uint8_t tFunctionCode = FUNCTION_BUTTON_SET_CAPTION;
        if (doDrawButton) {
            tFunctionCode = FUNCTION_BUTTON_SET_CAPTION_AND_DRAW_BUTTON;
        }
        sendUSARTArgsAndByteBuffer(tFunctionCode, 1, aButtonNumber, strlen(aCaption), aCaption);
    }
}

void BlueDisplay::setButtonValue(BDButtonHandle_t aButtonNumber, int16_t aValue) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, aButtonNumber, SUBFUNCTION_BUTTON_SET_VALUE, aValue);
    }
}

void BlueDisplay::setButtonValueAndDraw(BDButtonHandle_t aButtonNumber, int16_t aValue) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, aButtonNumber, SUBFUNCTION_BUTTON_SET_VALUE_AND_DRAW, aValue);
    }
}

void BlueDisplay::setButtonColor(BDButtonHandle_t aButtonNumber, color16_t aButtonColor) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, aButtonNumber, SUBFUNCTION_BUTTON_SET_BUTTON_COLOR, aButtonColor);
    }
}

void BlueDisplay::setButtonColorAndDraw(BDButtonHandle_t aButtonNumber, color16_t aButtonColor) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 3, aButtonNumber, SUBFUNCTION_BUTTON_SET_BUTTON_COLOR_AND_DRAW, aButtonColor);
    }
}

void BlueDisplay::setButtonPosition(BDButtonHandle_t aButtonNumber, int16_t aPositionX, int16_t aPositionY) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 4, aButtonNumber, SUBFUNCTION_BUTTON_SET_POSITION, aPositionX, aPositionY);
    }
}

void BlueDisplay::setButtonAutorepeatTiming(BDButtonHandle_t aButtonNumber, uint16_t aMillisFirstDelay, uint16_t aMillisFirstRate,
        uint16_t aFirstCount, uint16_t aMillisSecondRate) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 7, aButtonNumber, SUBFUNCTION_BUTTON_SET_AUTOREPEAT_TIMING, aMillisFirstDelay,
                aMillisFirstRate, aFirstCount, aMillisSecondRate);
    }
}

void BlueDisplay::activateButton(BDButtonHandle_t aButtonNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 2, aButtonNumber, SUBFUNCTION_BUTTON_SET_ACTIVE);
    }
}

void BlueDisplay::deactivateButton(BDButtonHandle_t aButtonNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 2, aButtonNumber, SUBFUNCTION_BUTTON_RESET_ACTIVE);
    }
}

void BlueDisplay::setButtonsGlobalFlags(uint16_t aFlags) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_GLOBAL_SETTINGS, 1, aFlags);
    }
}

/*
 * aToneVolume: value in percent
 */
void BlueDisplay::setButtonsTouchTone(uint8_t aToneIndex, uint8_t aToneVolume) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_GLOBAL_SETTINGS, 3, FLAG_BUTTON_GLOBAL_SET_BEEP_TONE, aToneIndex, aToneVolume);
    }
}

void BlueDisplay::activateAllButtons(void) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_ACTIVATE_ALL, 0);
    }
}

void BlueDisplay::deactivateAllButtons(void) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_BUTTON_DEACTIVATE_ALL, 0);
    }
}

#ifdef AVR
BDButtonHandle_t BlueDisplay::createButtonPGM(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY,
        color16_t aButtonColor, const char *aPGMCaption, uint8_t aCaptionSize, uint8_t aFlags, int16_t aValue,
        void (*aOnTouchHandler)(BDButton *, int16_t)) {
    BDButtonHandle_t tButtonNumber = sLocalButtonIndex++;
    if (USART_isBluetoothPaired()) {
        uint8_t tCaptionLength = strlen_P(aPGMCaption);
        if (tCaptionLength > STRING_BUFFER_STACK_SIZE) {
            tCaptionLength = STRING_BUFFER_STACK_SIZE;
        }
        char StringBuffer[STRING_BUFFER_STACK_SIZE];
        strncpy_P(StringBuffer, aPGMCaption, tCaptionLength);
        sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 9, tButtonNumber, aPositionX, aPositionY, aWidthX, aHeightY,
                aButtonColor, aCaptionSize | (aFlags << 8), aValue, aOnTouchHandler, tCaptionLength, StringBuffer);
    }
    return tButtonNumber;
}

void BlueDisplay::setButtonCaptionPGM(BDButtonHandle_t aButtonNumber, const char *aPGMCaption, bool doDrawButton) {
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
        sendUSARTArgsAndByteBuffer(tFunctionCode, 1, aButtonNumber, tCaptionLength, tStringBuffer);
    }
}
#endif

/***************************************************************************************************************************************************
 *
 * SLIDER
 *
 **************************************************************************************************************************************************/

/**
 * @brief initialization with all parameters except color
 * @param aPositionX determines upper left corner
 * @param aPositionY determines upper left corner
 * @param aBarWidth width of bar (and border) in pixel
 * @param aBarLength size of slider bar in pixel = maximum slider value
 * @param aThresholdValue value where color of bar changes from #SLIDER_DEFAULT_BAR_COLOR to #SLIDER_DEFAULT_BAR_THRESHOLD_COLOR
 * @param aInitalValue
 * @param aSliderColor
 * @param aBarColor
 * @param aOptions see #FLAG_SLIDER_SHOW_BORDER etc.
 * @param aOnChangeHandler - if NULL no update of bar is done on touch
 * @return slider index.
 */
BDSliderHandle_t BlueDisplay::createSlider(uint16_t aPositionX, uint16_t aPositionY, uint8_t aBarWidth, int16_t aBarLength,
        int16_t aThresholdValue, int16_t aInitalValue, color16_t aSliderColor, color16_t aBarColor, uint8_t aFlags,
        void (*aOnChangeHandler)(BDSliderHandle_t*, int16_t)) {
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
    return tSliderNumber;
}

void BlueDisplay::drawSlider(BDSliderHandle_t aSliderNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_DRAW, 1, aSliderNumber);
    }
}

void BlueDisplay::drawSliderBorder(BDSliderHandle_t aSliderNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_DRAW_BORDER, 1, aSliderNumber);
    }
}

void BlueDisplay::setSliderValueAndDrawBar(BDSliderHandle_t aSliderNumber, int16_t aCurrentValue) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, aSliderNumber, SUBFUNCTION_SLIDER_SET_VALUE_AND_DRAW_BAR, aCurrentValue);
    }
}

void BlueDisplay::setSliderColorBarThreshold(BDSliderHandle_t aSliderNumber, uint16_t aBarThresholdColor) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, aSliderNumber, SUBFUNCTION_SLIDER_SET_COLOR_THRESHOLD, aBarThresholdColor);
    }
}

void BlueDisplay::setSliderColorBarBackground(BDSliderHandle_t aSliderNumber, uint16_t aBarBackgroundColor) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 3, aSliderNumber, SUBFUNCTION_SLIDER_SET_COLOR_BAR_BACKGROUND, aBarBackgroundColor);
    }
}

void BlueDisplay::setSliderCaptionProperties(BDSliderHandle_t aSliderNumber, uint8_t aCaptionSize, uint8_t aCaptionPosition,
        uint8_t aCaptionMargin, color16_t aCaptionColor, color16_t aCaptionBackgroundColor) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 7, aSliderNumber, SUBFUNCTION_SLIDER_SET_CAPTION_PROPERTIES, aCaptionSize,
                aCaptionPosition, aCaptionMargin, aCaptionColor, aCaptionBackgroundColor);
    }
}

void BlueDisplay::setSliderCaption(BDSliderHandle_t aSliderNumber, const char *aCaption) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgsAndByteBuffer(FUNCTION_SLIDER_SET_CAPTION, 1, aSliderNumber, strlen(aCaption), aCaption);
    }
}

void BlueDisplay::activateSlider(BDSliderHandle_t aSliderNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 2, aSliderNumber, SUBFUNCTION_SLIDER_SET_ACTIVE);
    }
}

void BlueDisplay::deactivateSlider(BDSliderHandle_t aSliderNumber) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_SETTINGS, 2, aSliderNumber, SUBFUNCTION_SLIDER_RESET_ACTIVE);
    }
}

void BlueDisplay::activateAllSliders(void) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_ACTIVATE_ALL, 0);
    }
}

void BlueDisplay::deactivateAllSliders(void) {
    if (USART_isBluetoothPaired()) {
        sendUSARTArgs(FUNCTION_SLIDER_DEACTIVATE_ALL, 0);
    }
}

/***************************************************************************************************************************************************
 *
 * Utilities
 *
 **************************************************************************************************************************************************/

void clearDisplayAndDisableButtonsAndSliders() {
    BlueDisplay1.clearDisplay();
    BDButton::deactivateAllButtons();
    BDSlider::deactivateAllSliders();
}

void clearDisplayAndDisableButtonsAndSliders(color16_t aColor) {
    BlueDisplay1.clearDisplay(aColor);
    BDButton::deactivateAllButtons();
    BDSlider::deactivateAllSliders();
}

#include <Arduino.h>
#if defined(AVR) && defined(ADCSRA) && defined(ADATE)

/*
 * The next definitions and weak functions are copied from ADCUtils to avoid delivering two additional files
 * for just one BlueDisplay function printVCCAndTemperaturePeriodically().
 */
//#include "ADCUtils.h"
/***************************************
 * ADC Section for VCC and temperature
 ***************************************/
#define ADC_PRESCALE8    3 // 104 microseconds per ADC conversion at 1 MHz
#define ADC_PRESCALE16   4 // 208 microseconds per ADC conversion at 1 MHz
#define ADC_PRESCALE32   5 // 416 microseconds per ADC conversion at 1 MHz
#define ADC_PRESCALE64   6 // 52 microseconds per ADC conversion at 16 MHz
#define ADC_PRESCALE128  7 // 104 microseconds per ADC conversion at 16 MHz
// definitions for 0.1 ms conversion time
#if (F_CPU == 1000000)
#define ADC_PRESCALE ADC_PRESCALE8
#elif (F_CPU == 8000000)
#define ADC_PRESCALE ADC_PRESCALE64
#elif (F_CPU == 16000000)
#define ADC_PRESCALE ADC_PRESCALE128
#endif

#define ADC_TEMPERATURE_CHANNEL_MUX 8
#define ADC_1_1_VOLT_CHANNEL_MUX 14

#define SHIFT_VALUE_FOR_REFERENCE REFS0

uint16_t __attribute__((weak)) readADCChannelWithReferenceOversample(uint8_t aChannelNumber, uint8_t aReference,
        uint8_t aOversampleExponent) {
    uint16_t tSumValue = 0;
    ADMUX = aChannelNumber | (aReference << SHIFT_VALUE_FOR_REFERENCE);

// ADCSRB = 0; // free running mode if ADATE is 1 - is default
// ADSC-StartConversion ADATE-AutoTriggerEnable ADIF-Reset Interrupt Flag
    ADCSRA = (_BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADIF) | ADC_PRESCALE);

    for (uint8_t i = 0; i < _BV(aOversampleExponent); i++) {
        /*
         * wait for free running conversion to finish.
         * Do not wait for ADSC here, since ADSC is only low for 1 ADC Clock cycle on free running conversion.
         */
        loop_until_bit_is_set(ADCSRA, ADIF);

        ADCSRA |= _BV(ADIF); // clear bit to recognize next conversion has finished
        // Add value
        tSumValue += ADCL | (ADCH << 8);        // using myWord does not save space here
        // tSumValue += (ADCH << 8) | ADCL; // this does NOT work!
    }
    ADCSRA &= ~_BV(ADATE); // Disable auto-triggering (free running mode)
    return (tSumValue >> aOversampleExponent);
}

// old name
float getVCCValue(void) {
    return getVCCVoltage();
}

#ifdef INTERNAL1V1
// workaround for MEGA 2560
#define INTERNAL INTERNAL1V1
#endif

/*
 * New name
 * Version which restore the ADC Channel and handle reference switching.
 */
float __attribute__((weak)) getVCCVoltage(void) {
// use AVCC with external capacitor at AREF pin as reference
    uint8_t tOldADMUX = ADMUX;
    /*
     * Must wait >= 200 us if reference has to be switched to VSS
     * Must wait >= 400 us if channel has to be switched to 1.1 volt internal channel from channel with 5 volt input
     */
    if ((ADMUX & (INTERNAL << SHIFT_VALUE_FOR_REFERENCE)) || ((ADMUX & 0x0F) != ADC_1_1_VOLT_CHANNEL_MUX)) {
        // Switch to 1.1 volt channel and AREF to VCC
        ADMUX = ADC_1_1_VOLT_CHANNEL_MUX | (DEFAULT << SHIFT_VALUE_FOR_REFERENCE);
        // and wait for settling
        delayMicroseconds(400); // experimental value is >= 400 us
    }
    uint16_t tVCC = readADCChannelWithReferenceOversample(ADC_1_1_VOLT_CHANNEL_MUX, DEFAULT, 2);
    ADMUX = tOldADMUX;
    /*
     * Do not wait for reference to settle here, since it may not be necessary
     */
    return ((1023 * 1.1) / tVCC);
}

/*
 * Version which restore the ADC Channel and handle reference switching.
 */
float __attribute__((weak)) getTemperature(void) {
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    return 0.0;
#else
// use internal 1.1 volt as reference
    uint8_t tOldADMUX;

    bool tReferenceMustBeChanged = (ADMUX & (DEFAULT << SHIFT_VALUE_FOR_REFERENCE));
    if (tReferenceMustBeChanged) {
        tOldADMUX = ADMUX;
        // set AREF  to 1.1 volt and wait for settling
        ADMUX = ADC_TEMPERATURE_CHANNEL_MUX | (INTERNAL << SHIFT_VALUE_FOR_REFERENCE);
        delayMicroseconds(4000); // measured value is 3500 us
    }

    float tTemp = (readADCChannelWithReferenceOversample(ADC_TEMPERATURE_CHANNEL_MUX, INTERNAL, 2) - 317);

    if (tReferenceMustBeChanged) {
        ADMUX = tOldADMUX;
        // wait for settling back to VCC
        delayMicroseconds(400); // experimental value is > 200 us
    }
    return (tTemp / 1.22);
#endif
}

/*
 * Show temperature and VCC voltage
 */
void BlueDisplay::printVCCAndTemperaturePeriodically(uint16_t aXPos, uint16_t aYPos, uint16_t aTextSize, uint16_t aPeriodMillis) {
    static unsigned long sMillisOfLastVCCInfo = 0;
    uint32_t tMillis = millis();

    if ((tMillis - sMillisOfLastVCCInfo) >= aPeriodMillis) {
        sMillisOfLastVCCInfo = tMillis;

        char tDataBuffer[18];
        char tVCCString[6];
        char tTempString[6];

        float tTemp = getTemperature();
        dtostrf(tTemp, 4, 1, tTempString);

        float tVCCvoltage = getVCCVoltage();
        dtostrf(tVCCvoltage, 4, 2, tVCCString);

        sprintf_P(tDataBuffer, PSTR("%s volt %s\xB0" "C"), tVCCString, tTempString); // \xB0 is degree character
        drawText(aXPos, aYPos, tDataBuffer, aTextSize, COLOR16_BLACK, COLOR16_WHITE);
    }
}
#else
// dummy functions for examples
uint16_t __attribute__((weak)) readADCChannelWithReferenceOversample(uint8_t aChannelNumber, uint8_t aReference,
        uint8_t aOversampleExponent) {
    return 0;
}
float __attribute__((weak)) getTemperature(void) {
    return 0.0;
}
float __attribute__((weak)) getVCCVoltage(void) {
    return 0.0;
}
#  ifdef AVR
void BlueDisplay::printVCCAndTemperaturePeriodically(uint16_t aXPos, uint16_t aYPos, uint16_t aTextSize, uint16_t aPeriodMillis) {
}
#  endif
#endif // defined(AVR) && defined(ADCSRA) && defined(ADATE)

/***************************************************************************************************************************************************
 *
 * Text sizes
 *
 **************************************************************************************************************************************************/
/*
 * TextSize * 1,125 (* (1 + 1/8))
 */
uint16_t getTextHeight(uint16_t aTextSize) {
    if (aTextSize == 11) {
        return TEXT_SIZE_11_HEIGHT;
    }
    if (aTextSize == 22) {
        return TEXT_SIZE_22_HEIGHT;
    }
    return aTextSize + aTextSize / 8; // TextSize * 1,125
}

/*
 * Formula for Monospace Font on Android
 * TextSize * 0.6
 * Integer Formula (rounded): (TextSize *6)+4 / 10
 */
uint16_t getTextWidth(uint16_t aTextSize) {
    if (aTextSize == 11) {
        return TEXT_SIZE_11_WIDTH;
    }
#ifdef PGMSPACE_MATTERS
    return TEXT_SIZE_22_WIDTH;
#else
    if (aTextSize == 22) {
        return TEXT_SIZE_22_WIDTH;
    }
    return ((aTextSize * 6) + 4) / 10;
#endif
}

/*
 * Formula for Monospace Font on Android
 * float: TextSize * 0.76
 * int: (TextSize * 195 + 128) >> 8
 */
uint16_t getTextAscend(uint16_t aTextSize) {
    if (aTextSize == TEXT_SIZE_11) {
        return TEXT_SIZE_11_ASCEND;
    }
#ifdef PGMSPACE_MATTERS
    return TEXT_SIZE_22_ASCEND;
#else
    if (aTextSize == TEXT_SIZE_22) {
        return TEXT_SIZE_22_ASCEND;
    }
    uint32_t tRetvalue = aTextSize;
    tRetvalue = ((tRetvalue * 195) + 128) >> 8;
    return tRetvalue;
#endif
}

/*
 * Formula for Monospace Font on Android
 * float: TextSize * 0.24
 * int: (TextSize * 61 + 128) >> 8
 */
uint16_t getTextDecend(uint16_t aTextSize) {
    if (aTextSize == TEXT_SIZE_11) {
        return TEXT_SIZE_11_ASCEND;
    }
#ifdef PGMSPACE_MATTERS
    return TEXT_SIZE_22_ASCEND;
#else

    if (aTextSize == TEXT_SIZE_22) {
        return TEXT_SIZE_22_ASCEND;
    }
    uint32_t tRetvalue = aTextSize;
    tRetvalue = ((tRetvalue * 61) + 128) >> 8;
    return tRetvalue;
#endif
}
/*
 * Ascend - Decent
 * is used to position text in the middle of a button
 * Formula for positioning:
 * Position = ButtonTop + (ButtonHeight + getTextAscendMinusDescend())/2
 */
uint16_t getTextAscendMinusDescend(uint16_t aTextSize) {
    if (aTextSize == TEXT_SIZE_11) {
        return TEXT_SIZE_11_ASCEND - TEXT_SIZE_11_DECEND;
    }
#ifdef PGMSPACE_MATTERS
    return TEXT_SIZE_22_ASCEND - TEXT_SIZE_22_DECEND;
#else
    if (aTextSize == TEXT_SIZE_22) {
        return TEXT_SIZE_22_ASCEND - TEXT_SIZE_22_DECEND;
    }
    uint32_t tRetvalue = aTextSize;
    tRetvalue = ((tRetvalue * 133) + 128) >> 8;
    return tRetvalue;
#endif
}

/*
 * (Ascend -Decent)/2
 */
uint16_t getTextMiddle(uint16_t aTextSize) {
    if (aTextSize == TEXT_SIZE_11) {
        return (TEXT_SIZE_11_ASCEND - TEXT_SIZE_11_DECEND) / 2;
    }
#ifdef PGMSPACE_MATTERS
    return (TEXT_SIZE_22_ASCEND - TEXT_SIZE_22_DECEND) / 2;
#else
    if (aTextSize == TEXT_SIZE_22) {
        return (TEXT_SIZE_22_ASCEND - TEXT_SIZE_22_DECEND) / 2;
    }
    uint32_t tRetvalue = aTextSize;
    tRetvalue = ((tRetvalue * 66) + 128) >> 8;
    return tRetvalue;
#endif
}

/*****************************************************************************
 * Display and drawing tests
 *****************************************************************************/
/**
 * Draws a star consisting of 4 lines each quadrant
 */
void BlueDisplay::drawStar(int aXPos, int aYPos, int tOffsetCenter, int tLength, int tOffsetDiagonal, int aLengthDiagonal,
        color16_t aColor) {

    int X = aXPos + tOffsetCenter;
// first right then left lines
    for (int i = 0; i < 2; i++) {
        drawLineRel(X, aYPos, tLength, 0, aColor);
        // < 45 degree
        drawLineRel(X, aYPos - tOffsetDiagonal, tLength, -aLengthDiagonal, aColor);
        drawLineRel(X, aYPos + tOffsetDiagonal, tLength, aLengthDiagonal, aColor);
        X = aXPos - tOffsetCenter;
        tLength = -tLength;
    }

    int Y = aYPos + tOffsetCenter;
// first lower then upper lines
    for (int i = 0; i < 2; i++) {
        drawLineRel(aXPos, Y, 0, tLength, aColor);
        drawLineRel(aXPos - tOffsetDiagonal, Y, -aLengthDiagonal, tLength, aColor);
        drawLineRel(aXPos + tOffsetDiagonal, Y, aLengthDiagonal, tLength, aColor);
        Y = aYPos - tOffsetCenter;
        tLength = -tLength;
    }

    X = aXPos + tOffsetCenter;
    int tLengthDiagonal = tLength;
    for (int i = 0; i < 2; i++) {
        // 45 degree
        drawLineRel(X, aYPos - tOffsetCenter, tLength, -tLengthDiagonal, aColor);
        drawLineRel(X, aYPos + tOffsetCenter, tLength, tLengthDiagonal, aColor);
        X = aXPos - tOffsetCenter;
        tLength = -tLength;
    }

    drawPixel(aXPos, aYPos, COLOR16_BLUE);
}

/**
 * Draws a greyscale and 3 color bars
 */
void BlueDisplay::drawGreyscale(uint16_t aXPos, uint16_t tYPos, uint16_t aHeight) {
    uint16_t tY;
    for (int i = 0; i < 256; ++i) {
        tY = tYPos;
        drawLineRel(aXPos, tY, 0, aHeight, COLOR16(i, i, i));
        tY += aHeight;
        drawLineRel(aXPos, tY, 0, aHeight, COLOR16((0xFF - i), (0xFF - i), (0xFF - i)));
        tY += aHeight;
        drawLineRel(aXPos, tY, 0, aHeight, COLOR16(i, 0, 0));
        tY += aHeight;
        drawLineRel(aXPos, tY, 0, aHeight, COLOR16(0, i, 0));
        tY += aHeight;
        // For Test purposes: fillRectRel instead of drawLineRel gives missing pixel on different scale factors
        fillRectRel(aXPos, tY, 1, aHeight, COLOR16(0, 0, i));
        aXPos++;
    }
}

/**
 * Draws test page and a greyscale bar
 */
void BlueDisplay::testDisplay(void) {
    clearDisplay();

    fillRectRel(0, 0, 2, 2, COLOR16_RED);
    fillRectRel(mRequestedDisplaySize.XWidth - 3, 0, 3, 3, COLOR16_GREEN);
    fillRectRel(0, mRequestedDisplaySize.YHeight - 4, 4, 4, COLOR16_BLUE);
    fillRectRel(mRequestedDisplaySize.XWidth - 3, mRequestedDisplaySize.YHeight - 3, 3, 3, COLOR16_BLACK);

    fillRectRel(2, 2, 4, 4, COLOR16_RED);
    fillRectRel(10, 20, 10, 20, COLOR16_RED);
    drawRectRel(8, 18, 14, 24, COLOR16_BLUE, 1);
    drawCircle(15, 30, 5, COLOR16_BLUE, 1);
    fillCircle(20, 10, 10, COLOR16_BLUE);

    drawLineRel(0, mRequestedDisplaySize.YHeight - 1, mRequestedDisplaySize.XWidth, -mRequestedDisplaySize.YHeight,
    COLOR16_GREEN);
    drawLineRel(6, 6, mRequestedDisplaySize.XWidth - 9, mRequestedDisplaySize.YHeight - 9, COLOR16_BLUE);
    drawChar(50, TEXT_SIZE_11_ASCEND, 'y', TEXT_SIZE_11, COLOR16_GREEN, COLOR16_YELLOW);
    drawText(0, 50 + TEXT_SIZE_11_ASCEND, "Calibration", TEXT_SIZE_11, COLOR16_BLACK, COLOR16_WHITE);
    drawText(0, 50 + TEXT_SIZE_11_HEIGHT + TEXT_SIZE_11_ASCEND, "Calibration", TEXT_SIZE_11, COLOR16_WHITE,
    COLOR16_BLACK);

#ifdef LOCAL_DISPLAY_EXISTS
    drawLineOverlap(120, 140, 180, 125, LINE_OVERLAP_MAJOR, COLOR16_RED);
    drawLineOverlap(120, 143, 180, 128, LINE_OVERLAP_MINOR, COLOR16_RED);
    drawLineOverlap(120, 146, 180, 131, LINE_OVERLAP_BOTH, COLOR16_RED);
#endif

    fillRectRel(100, 100, 10, 5, COLOR16_RED);
    fillRectRel(90, 95, 10, 5, COLOR16_RED);
    fillRectRel(100, 90, 10, 10, COLOR16_BLACK);
    fillRectRel(95, 100, 5, 5, COLOR16_BLACK);

    drawStar(200, 120, 4, 6, 2, 2, COLOR16_BLACK);
    drawStar(250, 120, 8, 12, 4, 4, COLOR16_BLACK);

    uint16_t DeltaSmall = 20;
    uint16_t DeltaBig = 100;
    uint16_t tYPos = 30;

    tYPos += 45;
    drawLineWithThickness(10, tYPos, 10 + DeltaSmall, tYPos + DeltaBig, 4, COLOR16_GREEN);
    drawPixel(10, tYPos, COLOR16_BLUE);

    drawLineWithThickness(70, tYPos, 70 - DeltaSmall, tYPos + DeltaBig, 4, COLOR16_GREEN);
    drawPixel(70, tYPos, COLOR16_BLUE);

    tYPos += 10;
    drawLineWithThickness(140, tYPos, 140 - DeltaSmall, tYPos - DeltaSmall, 3, COLOR16_GREEN);
    drawPixel(140, tYPos, COLOR16_BLUE);

    drawLineWithThickness(150, tYPos, 150 + DeltaSmall, tYPos - DeltaSmall, 3, COLOR16_GREEN);
    drawPixel(150, tYPos, COLOR16_BLUE);

#ifdef LOCAL_DISPLAY_EXISTS
    drawThickLine(190, tYPos, 190 - DeltaSmall, tYPos - DeltaSmall, 3, LINE_THICKNESS_DRAW_CLOCKWISE, COLOR16_GREEN);
    drawPixel(190, tYPos, COLOR16_BLUE);

    drawThickLine(200, tYPos, 200 + DeltaSmall, tYPos - DeltaSmall, 3, LINE_THICKNESS_DRAW_CLOCKWISE, COLOR16_GREEN);
    drawPixel(200, tYPos, COLOR16_BLUE);

    tYPos -= 55;
    drawThickLine(140, tYPos, 140 + DeltaBig, tYPos - DeltaSmall, 9, LINE_THICKNESS_DRAW_CLOCKWISE, COLOR16_GREEN);
    drawPixel(140, tYPos, COLOR16_BLUE);

    tYPos += 5;
    drawThickLine(60, tYPos, 60 + DeltaBig, tYPos + DeltaSmall, 9, LINE_THICKNESS_DRAW_CLOCKWISE, COLOR16_GREEN);
    drawPixel(100, tYPos + 5, COLOR16_BLUE);
#endif
    drawGreyscale(5, 180, 10);
}

#define COLOR_SPECTRUM_SEGMENTS 6 // red->yellow, yellow-> green, green-> cyan, cyan-> blue, blue-> magent, magenta-> red
#define COLOR_RESOLUTION 32 // 32 (5 bit) different red colors for 16 bit color (green really has 6 bit, but we don't use 6 bit)
const uint16_t colorIncrement[COLOR_SPECTRUM_SEGMENTS] = { 1 << 6, 0x1FU << 11, 1, 0x3FFU << 6, 1 << 11, 0xFFFFU };

/**
 * generates a full color spectrum beginning with a black line,
 * increasing saturation to full colors and then fading to a white line
 * customized for a 320 x 240 display
 */
void BlueDisplay::generateColorSpectrum(void) {
    clearDisplay();
    uint16_t tColor;
    uint16_t tXPos;
    uint16_t tDelta;
    uint16_t tError;

    uint16_t tColorChangeAmount;
    uint16_t tYpos = mRequestedDisplaySize.YHeight;
    uint16_t tColorLine;
    for (uint16_t line = 4; line < mRequestedDisplaySize.YHeight + 4; ++line) {
        tColorLine = line / 4;
        // colors for line 31 and 32 are identical
        if (tColorLine >= COLOR_RESOLUTION) {
            // line 32 to 63 full saturated basic colors to pure white
            tColorChangeAmount = ((2 * COLOR_RESOLUTION) - 1) - tColorLine; // 31 - 0
            tColor = 0x1f << 11 | (tColorLine - COLOR_RESOLUTION) << 6 | (tColorLine - COLOR_RESOLUTION);
        } else {
            // line 0 - 31 pure black to full saturated basic colors
            tColor = tColorLine << 11; // RED
            tColorChangeAmount = tColorLine; // 0 - 31
        }
        tXPos = 0;
        tYpos--;
        for (int i = 0; i < COLOR_SPECTRUM_SEGMENTS; ++i) {
            tDelta = colorIncrement[i];
//          tError = COLOR_RESOLUTION / 2;
//          for (int j = 0; j < COLOR_RESOLUTION; ++j) {
//              // draw start value + 31 slope values
//              _drawPixel(tXPos++, tYpos, tColor);
//              tError += tColorChangeAmount;
//              if (tError > COLOR_RESOLUTION) {
//                  tError -= COLOR_RESOLUTION;
//                  tColor += tDelta;
//              }
//          }
            tError = ((mRequestedDisplaySize.XWidth / COLOR_SPECTRUM_SEGMENTS) - 1) / 2;
            for (uint16_t j = 0; j < (mRequestedDisplaySize.XWidth / COLOR_SPECTRUM_SEGMENTS) - 1; ++j) {
                drawPixel(tXPos++, tYpos, tColor);
                tError += tColorChangeAmount;
                if (tError > ((mRequestedDisplaySize.XWidth / COLOR_SPECTRUM_SEGMENTS) - 1)) {
                    tError -= ((mRequestedDisplaySize.XWidth / COLOR_SPECTRUM_SEGMENTS) - 1);
                    tColor += tDelta;
                }
            }
            // draw greyscale in the last 8 pixel :-)
//          _drawPixel(mRequestedDisplaySize.XWidth - 2, tYpos, (tColorLine & 0x3E) << 10 | tColorLine << 5 | tColorLine >> 1);
//          _drawPixel(mRequestedDisplaySize.XWidth - 1, tYpos, (tColorLine & 0x3E) << 10 | tColorLine << 5 | tColorLine >> 1);
            drawLine(mRequestedDisplaySize.XWidth - 8, tYpos, mRequestedDisplaySize.XWidth - 1, tYpos,
                    (tColorLine & 0x3E) << 10 | tColorLine << 5 | tColorLine >> 1);

        }
    }
}
