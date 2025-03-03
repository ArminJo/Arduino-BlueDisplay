/*
 * BlueDisplay.hpp
 *
 * C stub for the Android BlueDisplay app (and the local LCD screens with SSD1289 (on HY32D board) or HX8347 (on MI0283QT2 board) controller.
 *
 * It implements also a few display test functions.
 *
 *  SUMMARY
 *  Blue Display is an Open Source Android remote Display for Arduino etc.
 *  It receives basic draw requests from Arduino etc. over Bluetooth and renders it.
 *  It also implements basic GUI elements as buttons and sliders.
 *  GUI callback, touch and sensor events are sent back to Arduino.
 *
 *  Copyright (C) 2014-2025  Armin Joachimsmeyer
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

/*
 * This library can be configured at compile time by the following options / macros:
 * For more details see: https://github.com/ArminJo/Arduino-BlueDisplay?tab=readme-ov-file#compile-options--macros-for-this-library
 *
 * - BLUETOOTH_BAUD_RATE                Activate this, if you have reprogrammed the HC05 module for 115200, otherwise 9600 is used as baud rate.
 * - DO_NOT_NEED_BASIC_TOUCH_EVENTS     Disables basic touch events down, move and up. Saves up to 180 bytes program memory and 14 bytes RAM.
 * - DO_NOT_NEED_LONG_TOUCH_DOWN_AND_SWIPE_EVENTS Disables LongTouchDown and SwipeEnd events.
 * - ONLY_CONNECT_EVENT_REQUIRED        Disables reorientation, redraw and SensorChange events
 * - BD_USE_SIMPLE_SERIAL               Only for AVR! Do not use the Serial object. Saves up to 1250 bytes program memory and 185 bytes RAM, if Serial is not used otherwise.
 * - BD_USE_USB_SERIAL                  Activate it, if you want to force using Serial instead of Serial1 for direct USB cable connection to your smartphone / tablet.
 *
 */

#ifndef _BLUEDISPLAY_HPP
#define _BLUEDISPLAY_HPP

#include "BlueDisplay.h"

#if !defined(DISABLE_REMOTE_DISPLAY)
#include "BlueSerial.hpp"
#endif
#include "EventHandler.hpp"
#include "BDButton.hpp"
#include "BDSlider.hpp"
#include "Chart.hpp"
#include "GUIHelper.hpp"

#if defined(SUPPORT_LOCAL_DISPLAY)
// LocalGUI/LocalTouchButton.hpp etc are included by BDButton.hpp etc. above
#include "LocalGUI/ThickLine.hpp"
#include "LocalGUI/LocalTinyPrint.hpp"
#endif

#include <string.h>  // for strlen
#include <stdio.h> /* for sprintf */
#include <math.h> // for PI
#include <stdlib.h> // for dtostrf()

#if defined(DEBUG)
#define LOCAL_DEBUG
#else
//#define LOCAL_DEBUG // This enables debug output only for this file
#endif

#if defined(TRACE)
#define LOCAL_TRACE
#else
//#define LOCAL_TRACE // This enables debug output only for this file
#endif
//-------------------- Constructor --------------------

BlueDisplay::BlueDisplay() { // @suppress("Class members should be properly initialized")
//    mRequestedDisplaySize.XWidth = DISPLAY_DEFAULT_WIDTH;
//    mRequestedDisplaySize.YHeight = DISPLAY_DEFAULT_HEIGHT;
    mBlueDisplayConnectionEstablished = false;
}

// One instance of BlueDisplay called BlueDisplay1
BlueDisplay BlueDisplay1;

bool isLocalDisplayAvailable = false;

void BlueDisplay::resetLocal() {
    // reset local buttons to be synchronized
    BDButton::resetAll();
    BDSlider::resetAll();
}

/**
 * Sets callback handler and calls android app (host) for requestMaxCanvasSize().
 * If host is still Bluetooth connected, this results in a EVENT_REQUESTED_DATA_CANVAS_SIZE event.
 * This event sets mCurrentDisplaySize and mHostUnixTimestamp and in turn calls the ConnectCallback as well as the RedrawCallback.
 *
 * Wait 300 ms for receiving the EVENT_REQUESTED_DATA_CANVAS_SIZE event.
 *
 * For ESP32 and after power on of the Bluetooth module (HC-05) at other platforms, Bluetooth is just enabled here,
 * but the android app (host) is not manually (re)connected to us, so we are most likely not connected here.
 *
 * At the time of manual (re)connection in the android app, a connection start event is send,
 * which has the same content as the EVENT_REQUESTED_DATA_CANVAS_SIZE requested here.
 * This event is then processed by the periodic call of checkAndHandleEvents() in the main loop.
 *
 * Redraw event is intended to redraw screen, if size of display changes.
 * The app itself does the scaling of the screen content, but this can be coarse, especially when inflating
 * so we will get better quality, if we redraw the content.
 * Size changes will not happen, if BD_FLAG_USE_MAX_SIZE is set, and no reorientation (implying size changes) happens.
 *
 * The redraw event handler is automatically called directly after the reconnect or reorientation handler.
 *
 * Reorientation callback function is only required if we have a responsive layout,
 * since connect and reorientation event also calls the redraw callback.
 *
 * Usage:
 *  uint16_t tConnectDurationMillis = BlueDisplay1.initCommunication(&initDisplay, &drawGui); // introduces up to 1.5 seconds delay
 *  #if !defined(BD_USE_SIMPLE_SERIAL)
 *  if (tConnectDurationMillis > 0) {
 *      Serial.print("Connection established after ");
 *      Serial.print(tConnectDurationMillis);
 *      Serial.println(" ms");
 *  } else {
 *      Serial.println(F("No connection after " STR(CONNECTIOM_TIMEOUT_MILLIS) " ms"));
 *  }
 *  #endif
 *
 * @return The milliseconds used to connect, 0 if no connection was made
 */
uint_fast16_t BlueDisplay::initCommunication(void (*aConnectCallback)(), void (*aRedrawCallback)(),
        void (*aReorientationCallback)()) {
    registerConnectCallback(aConnectCallback);
#if !defined(ONLY_CONNECT_EVENT_REQUIRED)
    registerRedrawCallback(aRedrawCallback);
    registerReorientationCallback(aReorientationCallback);
#else
    (void) aRedrawCallback;
    (void) aReorientationCallback;
#endif

    mBlueDisplayConnectionEstablished = false;
#if defined(ARDUINO)
    // consume up old received data
    clearSerialInputBuffer();
#endif

// This results in a data event, which sends size and timestamp
    requestMaxCanvasSize();

    for (uint_fast16_t i = 0; i < CONNECTIOM_TIMEOUT_MILLIS; i += 10) {
        /*
         * Wait 300 ms for receiving the EVENT_REQUESTED_DATA_CANVAS_SIZE event.
         * Time measured is between 50 and 150 ms (or mostly between 80 and 120) for Bluetooth.
         * But I have seen 990 ms too :-(( , which corresponds to a communication lag of 500 ms in each direction,
         * even if we have a 1 second delay before initCommunication();
         */
        delayMillisWithCheckAndHandleEvents(10);
        if (mBlueDisplayConnectionEstablished) { // is set by delay(WithCheckAndHandleEvents()
            // Handler are called initially by the received canvas size event
            return i;
            break;
        }
    }
    return 0;
}

/*
 * Call with BlueDisplay1.initCommunication(&Serial, &connectHandler);
 */
#if defined(ARDUINO)
void BlueDisplay::initCommunication(Print *aSerial, void (*aConnectCallback)(), void (*aRedrawCallback)(),
        void (*aReorientationCallback)()) {
#  if defined(BD_USE_SIMPLE_SERIAL)
    (void) aSerial;
    BlueDisplay1.initCommunication(aConnectCallback,aRedrawCallback,aReorientationCallback);
#  else
    uint_fast16_t tConnectDurationMillis = BlueDisplay1.initCommunication(aConnectCallback, aRedrawCallback,
            aReorientationCallback);
    if (tConnectDurationMillis > 0) {
        aSerial->print("Connection established after ");
        aSerial->print(tConnectDurationMillis);
        aSerial->println(" ms");
    } else {
        aSerial->println(F("No connection after " STR(CONNECTIOM_TIMEOUT_MILLIS) " ms"));
    }
#  endif
}
#endif

bool BlueDisplay::isConnectionEstablished() {
    return mBlueDisplayConnectionEstablished;
}

// sends 4 byte function and 36 byte data message containing 32 0x00
void BlueDisplay::sendSync() {
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
#if defined(LOCAL_TRACE) && !defined(BD_USE_SIMPLE_SERIAL)
            Serial.println("Send reset all");
#endif
            // reset local buttons to be synchronized
            BDButton::resetAll();
            BDSlider::resetAll();
        }
#if defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
        aFlags |= BD_FLAG_TOUCH_BASIC_DISABLE;
#endif
        sendUSARTArgs(FUNCTION_GLOBAL_SETTINGS, 4, SUBFUNCTION_GLOBAL_SET_FLAGS_AND_SIZE, aFlags, aWidth, aHeight);
    }
}

/**
 *
 * @param aCodePage Number for ISO_8859_<Number>
 */
void BlueDisplay::setCodePage(uint16_t aCodePageNumber) {
    sendUSARTArgs(FUNCTION_GLOBAL_SETTINGS, 2, SUBFUNCTION_GLOBAL_SET_CODEPAGE, aCodePageNumber);
}

/*
 * aChar must be bigger than 0x80!
 *  0x03A9 is Omega in UTF16
 *  0x0394 is Delta in UTF16
 *  0x21B2 is Enter in UTF16
 *  0x21E7 is Ascending in UTF16 - for printInfo()
 *  0x21E9 is Descending in UTF16 - for printInfo()
 *  0x2302 is Home in UTF16
 *  0x2227 is UP (logical AND) in UTF16
 *  0x2228 is Down (logical OR) in UTF16
 *  0x2195 is UP/Down in UTF16
 *  0x2103 is Degree Celsius in UTF16
 */
void BlueDisplay::setCharacterMapping(uint8_t aChar, uint16_t aUnicodeChar) {
    sendUSARTArgs(FUNCTION_GLOBAL_SETTINGS, 3, SUBFUNCTION_GLOBAL_SET_CHARACTER_CODE_MAPPING, aChar, aUnicodeChar);
}

void BlueDisplay::setLongTouchDownTimeout(uint16_t aLongTouchDownTimeoutMillis) {
    sendUSARTArgs(FUNCTION_GLOBAL_SETTINGS, 2, SUBFUNCTION_GLOBAL_SET_LONG_TOUCH_DOWN_TIMEOUT, aLongTouchDownTimeoutMillis);
}

/**
 * @param aLockMode one of FLAG_SCREEN_ORIENTATION_LOCK_LANDSCAPE, FLAG_SCREEN_ORIENTATION_LOCK_PORTRAIT,
 *         FLAG_SCREEN_ORIENTATION_LOCK_CURRENT or FLAG_SCREEN_ORIENTATION_LOCK_UNLOCK
 */
void BlueDisplay::setScreenOrientationLock(uint8_t aLockMode) {
    sendUSARTArgs(FUNCTION_GLOBAL_SETTINGS, 2, SUBFUNCTION_GLOBAL_SET_SCREEN_ORIENTATION_LOCK, aLockMode);
}

// 255 / BD_SCREEN_BRIGHTNESS_USER is user default, 0 / BD_SCREEN_BRIGHTNESS_MIN is dark and BD_SCREEN_BRIGHTNESS_MAX / 100 is full bright
void BlueDisplay::setScreenBrightness(uint8_t aScreenBrightness) {
    sendUSARTArgs(FUNCTION_GLOBAL_SETTINGS, 2, SUBFUNCTION_GLOBAL_SET_SCREEN_BRIGHTNESS, aScreenBrightness);
}

void BlueDisplay::playTone() {
    sendUSARTArgs(FUNCTION_PLAY_TONE, 1, TONE_DEFAULT);
}

/*
 * index is from android.media.ToneGenerator see also
 * http://developer.android.com/reference/android/media/ToneGenerator.html
 * Tone index 0 is DTMF tone for key 0: 1336Hz, 941Hz, continuous
 * defaults are:
 * ToneDuration = as long as the tone on android lasts (100ms?)
 */
void BlueDisplay::playTone(uint8_t aToneIndex) {
    sendUSARTArgs(FUNCTION_PLAY_TONE, 1, aToneIndex);
}

/*
 * aToneDuration -1 means forever
 * but except the value -1 aToneDuration is taken as unsigned so -2 will give 65534 micros
 */
void BlueDisplay::playTone(uint8_t aToneIndex, int16_t aToneDuration) {
    sendUSARTArgs(FUNCTION_PLAY_TONE, 2, aToneIndex, aToneDuration);
}

/*
 * aToneDuration -1 means forever
 * but except the value -1 aToneDuration is taken as unsigned so -2 will give 65534 micros
 */
void BlueDisplay::playTone(uint8_t aToneIndex, int16_t aToneDuration, uint8_t aToneVolume) {
    sendUSARTArgs(FUNCTION_PLAY_TONE, 3, aToneIndex, aToneDuration, aToneVolume);
}

void BlueDisplay::playFeedbackTone(uint8_t aFeedbackToneType) {
    if (aFeedbackToneType == FEEDBACK_TONE_OK) {
        playTone(TONE_PROP_BEEP_OK);
    } else if (aFeedbackToneType == FEEDBACK_TONE_ERROR) {
        playTone(TONE_PROP_BEEP_ERROR);
    } else if (aFeedbackToneType == FEEDBACK_TONE_NO_TONE) {
        return;
    } else {
        playTone(aFeedbackToneType);
    }
}

void BlueDisplay::clearDisplay(color16_t aColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.clearDisplay(aColor);
#endif
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
//        sendUSARTBufferNoSizeCheck((uint8_t*) &tParamBuffer[0], 1 * 2 + 4, nullptr, 0);
    sendUSARTArgs(FUNCTION_CLEAR_DISPLAY, 1, aColor);
}

/*
 * If the buffer of the display device is full, commands up to this command may be skipped and display cleared.
 * Useful if we send commands faster than the display may able to handle, to avoid increasing delay between sending and rendering.
 */
void BlueDisplay::clearDisplayOptional(color16_t aColor) {
    sendUSARTArgs(FUNCTION_CLEAR_DISPLAY_OPTIONAL, 1, aColor);
}

// forces an rendering of the drawn bitmap
void BlueDisplay::drawDisplayDirect() {
    sendUSARTArgs(FUNCTION_DRAW_DISPLAY, 0);
}

void BlueDisplay::drawPixel(uint16_t aXPos, uint16_t aYPos, color16_t aColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.drawPixel(aXPos, aYPos, aColor);
#endif
    sendUSARTArgs(FUNCTION_DRAW_PIXEL, 3, aXPos, aYPos, aColor);
}

/*
 * Line drawn with this functions can be removed without residual
 * Horizontal or vertical lines are always drawn using aliasing Paint and thus can always be removed without residual
 * Other lines drawn with this functions can NOT be overwritten/removed without residual -> use drawLineWithAliasing()
 */
void BlueDisplay::drawLine(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.drawLine(aStartX, aStartY, aEndX, aEndY, aColor);
#endif
    sendUSART5Args(FUNCTION_DRAW_LINE, aStartX, aStartY, aEndX, aEndY, aColor);
}

/*
 * The relative parameters are added to start positions to get the end position!
 * I.e. if both are 0, then line is a pixel of width 1 and length 1!
 */
void BlueDisplay::drawLineRel(uint16_t aStartX, uint16_t aStartY, int16_t aXDelta, int16_t aYDelta, color16_t aColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.drawLine(aStartX, aStartY, aStartX + aXDelta, aStartY + aYDelta, aColor);
#endif
    sendUSART5Args(FUNCTION_DRAW_LINE_REL, aStartX, aStartY, aXDelta, aYDelta, aColor);
}

/*
 * Line drawn with this functions can be overwritten/removed without residual
 * Horizontal or vertical lines are always drawn using aliasing Paint and thus can always be overwritten/removed without residual
 */
void BlueDisplay::drawLineWithAliasing(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.drawLine(aStartX, aStartY, aEndX, aEndY, aColor);
#endif
    sendUSART5Args(FUNCTION_DRAW_LINE, aStartX, aStartY | 0x8000, aEndX, aEndY, aColor); // highest bit in aStartY signals use of aliasing paint
}
void BlueDisplay::drawLineRelWithAliasing(uint16_t aStartX, uint16_t aStartY, int16_t aXDelta, int16_t aYDelta, color16_t aColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.drawLine(aStartX, aStartY, aStartX + aXDelta, aStartY + aYDelta, aColor);
#endif
    sendUSART5Args(FUNCTION_DRAW_LINE_REL, aStartX, aStartY | 0x8000, aXDelta, aYDelta, aColor); // highest bit in aStartY signals use of aliasing paint
}
void BlueDisplay::drawVectorDegreeWithAliasing(uint16_t aStartX, uint16_t aStartY, uint16_t aLength, int aDegree, color16_t aColor,
        int16_t aThickness) {
    sendUSARTArgs(FUNCTION_DRAW_VECTOR_DEGREE, 6, aStartX, aStartY | 0x8000, aLength, aDegree, aColor, aThickness);
}

/**
 * Fast routine for drawing data charts
 * draws a line only from x to x+1
 * first pixel is omitted because it is drawn by preceding line
 * uses setArea instead if drawPixel to speed up drawing
 */
void BlueDisplay::drawLineFastOneX(uint16_t aStartX, uint16_t aStartY, uint16_t aEndY, color16_t aColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.drawLineFastOneX(aStartX, aStartY, aEndY, aColor);
#endif
    // Just draw plain line, no need to speed up
    sendUSART5Args(FUNCTION_DRAW_LINE, aStartX, aStartY, aStartX + 1, aEndY, aColor);
}

/*
 * aDegree in degree, not radian
 */
void BlueDisplay::drawVectorDegree(uint16_t aStartX, uint16_t aStartY, uint16_t aLength, int aDegree, color16_t aColor,
        int16_t aThickness) {
    sendUSARTArgs(FUNCTION_DRAW_VECTOR_DEGREE, 6, aStartX, aStartY, aLength, aDegree, aColor, aThickness);
}


/*
 * aRadian in float radian, not degree
 */
void BlueDisplay::drawVectorRadian(uint16_t aStartX, uint16_t aStartY, uint16_t aLength, float aRadian, color16_t aColor,
        int16_t aThickness) {

    if (USART_isBluetoothPaired()) {
        union {
            float floatValue;
            uint16_t shortArray[2];
        } floatToShortArray;
        floatToShortArray.floatValue = aRadian;
        sendUSARTArgs(FUNCTION_DRAW_VECTOR_RADIAN, 7, aStartX, aStartY, aLength, floatToShortArray.shortArray[0],
                floatToShortArray.shortArray[1], aColor, aThickness);
    }
}

void BlueDisplay::drawLineWithThickness(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor,
        int16_t aThickness) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    drawThickLine(aStartX, aStartY, aEndX, aEndY, aThickness, LINE_THICKNESS_MIDDLE, aColor);
#endif
    sendUSARTArgs(FUNCTION_DRAW_LINE, 6, aStartX, aStartY, aEndX, aEndY, aColor, aThickness);
}

void BlueDisplay::drawLineRelWithThickness(uint16_t aStartX, uint16_t aStartY, int16_t aXOffset, int16_t aYOffset, color16_t aColor,
        int16_t aThickness) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    drawThickLine(aStartX, aStartY, aStartX + aXOffset, aStartY + aYOffset, aThickness, LINE_THICKNESS_MIDDLE, aColor);
#endif
    sendUSARTArgs(FUNCTION_DRAW_LINE_REL, 6, aStartX, aStartY, aXOffset, aYOffset, aColor, aThickness);
}

void BlueDisplay::drawLineWithThicknessWithAliasing(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY,
        color16_t aColor, int16_t aThickness) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    drawThickLine(aStartX, aStartY, aEndX, aEndY, aThickness, LINE_THICKNESS_MIDDLE, aColor);
#endif
    sendUSARTArgs(FUNCTION_DRAW_LINE, 6, aStartX, aStartY | 0x8000, aEndX, aEndY, aColor, aThickness);
}

void BlueDisplay::drawLineRelWithThicknessWithAliasing(uint16_t aStartX, uint16_t aStartY, int16_t aXOffset, int16_t aYOffset,
        color16_t aColor, int16_t aThickness) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    drawThickLine(aStartX, aStartY, aStartX + aXOffset, aStartY + aYOffset, aThickness, LINE_THICKNESS_MIDDLE, aColor);
#endif
    sendUSARTArgs(FUNCTION_DRAW_LINE_REL, 6, aStartX, aStartY | 0x8000, aXOffset, aYOffset, aColor, aThickness);
}

void BlueDisplay::drawRect(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor,
        uint16_t aStrokeWidth) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.drawRect(aStartX, aStartY, aEndX - 1, aEndY - 1, aColor);
#endif
    sendUSARTArgs(FUNCTION_DRAW_RECT, 6, aStartX, aStartY, aEndX, aEndY, aColor, aStrokeWidth);
}

/*
 * If width or height is 0 rect will no be rendered :-)
 */
void BlueDisplay::drawRectRel(uint16_t aStartX, uint16_t aStartY, int16_t aXWidth, int16_t aHeight, color16_t aColor,
        uint16_t aStrokeWidth) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.drawRect(aStartX, aStartY, aStartX + aXWidth - 1, aStartY + aHeight - 1, aColor);
#endif
    sendUSARTArgs(FUNCTION_DRAW_RECT_REL, 6, aStartX, aStartY, aXWidth, aHeight, aColor, aStrokeWidth);
}

void BlueDisplay::fillRect(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.fillRect(aStartX, aStartY, aEndX, aEndY, aColor);
#endif
    sendUSART5Args(FUNCTION_FILL_RECT, aStartX, aStartY, aEndX, aEndY, aColor);
}

/*
 * If width or height is 0 rect will no be rendered :-)
 */
void BlueDisplay::fillRectRel(uint16_t aStartX, uint16_t aStartY, int16_t aXWidth, int16_t aHeight, color16_t aColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.fillRect(aStartX, aStartY, aStartX + aXWidth - 1, aStartY + aHeight - 1, aColor);
#endif
    sendUSART5Args(FUNCTION_FILL_RECT_REL, aStartX, aStartY, aXWidth, aHeight, aColor);
}

void BlueDisplay::drawCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, color16_t aColor, uint16_t aStrokeWidth) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.drawCircle(aXCenter, aYCenter, aRadius, aColor);
#endif
    sendUSART5Args(FUNCTION_DRAW_CIRCLE, aXCenter, aYCenter, aRadius, aColor, aStrokeWidth);
}

void BlueDisplay::fillCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, color16_t aColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.fillCircle(aXCenter, aYCenter, aRadius, aColor);
#endif
    sendUSARTArgs(FUNCTION_FILL_CIRCLE, 4, aXCenter, aYCenter, aRadius, aColor);
}

void BlueDisplay::clearTextArea(uint16_t aPositionX, uint16_t aPositionY, uint8_t aStringLength, uint16_t aFontSize,
        color16_t aClearColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.fillRect(aPositionX, aPositionY, aStringLength * getTextWidth(aFontSize), getTextHeight(aFontSize), aClearColor);
#endif
    fillRectRel(aPositionX, aPositionY - getTextAscend(aFontSize), aStringLength * getTextWidth(aFontSize),
            getTextHeight(aFontSize), aClearColor);
}

/**
 * @param aPositionX left position
 * @param aPositionY baseline position - use (upper_position + getTextAscend(<aFontSize>))
 * @return start x for next character / x + getTextWidth(aCharSize)
 */
uint16_t BlueDisplay::drawChar(uint16_t aPositionX, uint16_t aPositionY, char aChar, uint16_t aCharSize, color16_t aCharacterColor,
        color16_t aBackgroundColor) {
    uint16_t tRetValue = 0;
#if defined(SUPPORT_LOCAL_DISPLAY)
    tRetValue = LocalDisplay.drawChar(aPositionX, aPositionY - getTextAscend(aCharSize), aChar, getFontScaleFactorFromTextSize(aCharSize), aCharacterColor,
            aBackgroundColor);
#endif
    if (USART_isBluetoothPaired()) {
        tRetValue = aPositionX + getTextWidth(aCharSize);
        sendUSARTArgs(FUNCTION_DRAW_CHAR, 6, aPositionX, aPositionY, aCharSize, aCharacterColor, aBackgroundColor, aChar);
    }
    return tRetValue;
}

/**
 * Draws a text of almost unlimited size on the screen
 * @param aPositionX left position or STRING_ALIGN_RIGHT_XPOS or STRING_ALIGN_MIDDLE_XPOS
 * @param aPositionY baseline position is (upper_position + getTextAscend(<aFontSize>))
 * @param aStringPtr  If /r is used as newline character, rest of line will be cleared, if /n is used, rest of line will not be cleared.
 * @param aFontSize FontSize of text
 * @param aTextColor Foreground/text color
 * @param aBackgroundColor if COLOR16_NO_BACKGROUND, then the background will not be filled
 * @return uint16_t start x for next character - next x Parameter
 */
uint16_t BlueDisplay::drawText(uint16_t aPositionX, uint16_t aPositionY, const char *aStringPtr, uint16_t aFontSize,
        color16_t aTextColor, color16_t aBackgroundColor) {
    uint16_t tRetValue = 0;
#if defined(SUPPORT_LOCAL_DISPLAY)
    tRetValue = LocalDisplay.drawText(aPositionX, aPositionY - getTextAscend(aFontSize), (char *) aStringPtr, aFontSize,
            aTextColor, aBackgroundColor);
#endif
    tRetValue = aPositionX + strlen(aStringPtr) * getTextWidth(aFontSize);
    sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPositionX, aPositionY, aFontSize, aTextColor, aBackgroundColor,
            strlen(aStringPtr), (uint8_t*) aStringPtr);
    return tRetValue;
}

/*
 * Take size and colors from preceding drawText command
 */
void BlueDisplay::drawText(uint16_t aPositionX, uint16_t aPositionY, const char *aStringPtr) {
    sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 2, aPositionX, aPositionY, strlen(aStringPtr), (uint8_t*) aStringPtr);
}

/**
 * Draws a PGM text of maximum 32 characters (STRING_BUFFER_STACK_SIZE) on the screen
 * @param aPositionX left position or STRING_ALIGN_RIGHT_XPOS or STRING_ALIGN_MIDDLE_XPOS
 * @param aPositionY baseline position is (upper_position + getTextAscend(<aFontSize>))
 * @param aStringPtr  If /r is used as newline character, rest of line will be cleared, if /n is used, rest of line will not be cleared.
 * @param aFontSize FontSize of text
 * @param aTextColor Foreground/text color
 * @param aBackgroundColor if COLOR16_NO_BACKGROUND, then the background will not be filled
 * @return uint16_t start x for next character - next x Parameter
 */
uint16_t BlueDisplay::drawText(uint16_t aPositionX, uint16_t aPositionY, const __FlashStringHelper *aPGMString, uint16_t aFontSize,
        color16_t aTextColor, color16_t aBackgroundColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.drawText(aPositionX, aPositionY - getTextAscend(aFontSize), aPGMString, aFontSize, aTextColor,
            aBackgroundColor);
#endif
#if defined (AVR)
    char tStringBuffer[STRING_BUFFER_STACK_SIZE];
    uint8_t tTextLength = _clipAndCopyPGMString(tStringBuffer, aPGMString);
    uint16_t tRetValue = aPositionX + tTextLength * getTextWidth(aFontSize);
    sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPositionX, aPositionY, aFontSize, aTextColor, aBackgroundColor,
            tTextLength, (uint8_t*) tStringBuffer);
#else
    uint8_t tTextLength = strlen(reinterpret_cast<const char*>(aPGMString));
    uint16_t tRetValue = aPositionX + (tTextLength * getTextWidth(aFontSize));
    sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPositionX, aPositionY, aFontSize, aTextColor, aBackgroundColor,
            tTextLength, (uint8_t*) aPGMString);
#endif
    return tRetValue;
}

/**
 * Draws a PGM text of maximum 32 characters (STRING_BUFFER_STACK_SIZE) on the screen
 * Take size and colors from preceding drawText command
 */
void BlueDisplay::drawText(uint16_t aPositionX, uint16_t aPositionY, const __FlashStringHelper *aPGMString) {
#  if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.drawText(aPositionX, aPositionY, aPGMString, TEXT_SIZE_11, COLOR16_BLACK, COLOR16_WHITE);
#  endif
#if defined (AVR)
    char tStringBuffer[STRING_BUFFER_STACK_SIZE];
    uint8_t tTextLength = _clipAndCopyPGMString(tStringBuffer, aPGMString);
    sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 2, aPositionX, aPositionY, tTextLength, (uint8_t*) tStringBuffer);
#else
    uint8_t tTextLength = strlen(reinterpret_cast<const char*>(aPGMString));
    sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 2, aPositionX, aPositionY, tTextLength, (uint8_t*) aPGMString);
#endif
}

/**
 * @param aBackgroundColor if COLOR16_NO_BACKGROUND, then do not clear rest of line
 */
void BlueDisplay::drawMLText(uint16_t aPositionX, uint16_t aPositionY, const char *aStringPtr, uint16_t aFontSize,
        color16_t aTextColor, color16_t aBackgroundColor) {

#if defined(SUPPORT_LOCAL_DISPLAY)
    // here we have a special (and bigger) function, which handles multiple lines.
    LocalDisplay.drawMLText(aPositionX, aPositionY - getTextAscend(aFontSize), aStringPtr, aFontSize, aTextColor,
            aBackgroundColor);
#endif
    // the same as the drawText() function
    sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPositionX, aPositionY, aFontSize, aTextColor, aBackgroundColor,
            strlen(aStringPtr), (uint8_t*) aStringPtr);
}

/**
 * @param aBackgroundColor if COLOR16_NO_BACKGROUND, then do not clear rest of line
 */
void BlueDisplay::drawMLText(uint16_t aPositionX, uint16_t aPositionY, const __FlashStringHelper *aPGMString, uint16_t aFontSize,
        color16_t aTextColor, color16_t aBackgroundColor) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    // here we have a special (and bigger) function, which handles multiple lines.
    LocalDisplay.drawMLText(aPositionX, aPositionY - getTextAscend(aFontSize), aPGMString, aFontSize, aTextColor,
            aBackgroundColor);
#endif
    // the same as the drawText() function
#if defined(__AVR__)
    char tStringBuffer[STRING_BUFFER_STACK_SIZE];
    uint8_t tTextLength = _clipAndCopyPGMString(tStringBuffer, aPGMString);
    sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPositionX, aPositionY, aFontSize, aTextColor, aBackgroundColor,
            tTextLength, (uint8_t*) tStringBuffer);
#else
    uint8_t tTextLength = strlen(reinterpret_cast<const char*>(aPGMString));
    sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPositionX, aPositionY, aFontSize, aTextColor, aBackgroundColor,
            tTextLength, (uint8_t*) aPGMString);
#endif
}

uint16_t BlueDisplay::drawByte(uint16_t aPositionX, uint16_t aPositionY, int8_t aByte, uint16_t aFontSize, color16_t aFGColor,
        color16_t aBackgroundColor) {
    uint16_t tRetValue = 0;
    char tStringBuffer[5];
#if defined(__AVR__)
    snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("%4hhd"), aByte);
#else
    snprintf(tStringBuffer, sizeof(tStringBuffer), "%4hhd", aByte);
#endif
#if defined(SUPPORT_LOCAL_DISPLAY)
    tRetValue = LocalDisplay.drawText(aPositionX, aPositionY - getTextAscend(aFontSize), tStringBuffer, aFontSize, aFGColor,
            aBackgroundColor);
#endif
    if (USART_isBluetoothPaired()) {
        tRetValue = aPositionX + 4 * getTextWidth(aFontSize);
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPositionX, aPositionY, aFontSize, aFGColor, aBackgroundColor, 4,
                (uint8_t*) tStringBuffer);
    }
    return tRetValue;
}

uint16_t BlueDisplay::drawUnsignedByte(uint16_t aPositionX, uint16_t aPositionY, uint8_t aUnsignedByte, uint16_t aFontSize,
        color16_t aFGColor, color16_t aBackgroundColor) {
    uint16_t tRetValue = 0;
    char tStringBuffer[4];
#if defined(__AVR__)
    snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("%3u"), aUnsignedByte);
#else
    snprintf(tStringBuffer, sizeof(tStringBuffer), "%3u", aUnsignedByte);
#endif
#if defined(SUPPORT_LOCAL_DISPLAY)
    tRetValue = LocalDisplay.drawText(aPositionX, aPositionY - getTextAscend(aFontSize), tStringBuffer, aFontSize, aFGColor,
            aBackgroundColor);
#endif
    if (USART_isBluetoothPaired()) {
        tRetValue = aPositionX + 3 * getTextWidth(aFontSize);
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPositionX, aPositionY, aFontSize, aFGColor, aBackgroundColor, 3,
                (uint8_t*) tStringBuffer);
    }
    return tRetValue;
}

uint16_t BlueDisplay::drawShort(uint16_t aPositionX, uint16_t aPositionY, int16_t aShort, uint16_t aFontSize, color16_t aFGColor,
        color16_t aBackgroundColor) {
    uint16_t tRetValue = 0;
    char tStringBuffer[7];
#if defined(__AVR__)
    snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("%6hd"), aShort);
#else
    snprintf(tStringBuffer, sizeof(tStringBuffer), "%6hd", aShort);
#endif
#if defined(SUPPORT_LOCAL_DISPLAY)
    tRetValue = LocalDisplay.drawText(aPositionX, aPositionY - getTextAscend(aFontSize), tStringBuffer, aFontSize, aFGColor,
            aBackgroundColor);
#endif
    if (USART_isBluetoothPaired()) {
        tRetValue = aPositionX + 6 * getTextWidth(aFontSize);
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPositionX, aPositionY, aFontSize, aFGColor, aBackgroundColor, 6,
                (uint8_t*) tStringBuffer);
    }
    return tRetValue;
}

uint16_t BlueDisplay::drawLong(uint16_t aPositionX, uint16_t aPositionY, int32_t aLong, uint16_t aFontSize, color16_t aFGColor,
        color16_t aBackgroundColor) {
    uint16_t tRetValue = 0;
    char tStringBuffer[12];
#if defined(__AVR__)
    snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("%11ld"), aLong);
#elif defined(__XTENSA__)
    snprintf(tStringBuffer, sizeof(tStringBuffer), "%11ld", (long) aLong);
#else
    snprintf(tStringBuffer, sizeof(tStringBuffer), "%11ld", aLong);
#endif
#if defined(SUPPORT_LOCAL_DISPLAY)
    tRetValue = LocalDisplay.drawText(aPositionX, aPositionY - getTextAscend(aFontSize), tStringBuffer, aFontSize, aFGColor,
            aBackgroundColor);
#endif
    if (USART_isBluetoothPaired()) {
        tRetValue = aPositionX + 11 * getTextWidth(aFontSize);
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPositionX, aPositionY, aFontSize, aFGColor, aBackgroundColor, 11,
                (uint8_t*) tStringBuffer);
    }
    return tRetValue;
}

void BlueDisplay::setPaintSizeAndColor(uint8_t aPaintIndex, uint16_t aPaintSize, color16_t aPaintColor) {
    sendUSARTArgs(FUNCTION_WRITE_SETTINGS, 3, aPaintIndex, aPaintSize, aPaintColor);
}

/*
 * for writeString implementation
 */
void BlueDisplay::setWriteStringSizeAndColorAndFlag(uint16_t aPrintSize, color16_t aPrintColor, color16_t aPrintBackgroundColor,
        bool aClearOnNewScreen) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    printSetOptions(getFontScaleFactorFromTextSize(aPrintSize), aPrintColor, aPrintBackgroundColor, aClearOnNewScreen);
#endif
    sendUSART5Args(FUNCTION_WRITE_SETTINGS, FLAG_WRITE_SETTINGS_SET_SIZE_AND_COLORS_AND_FLAGS, aPrintSize, aPrintColor,
            aPrintBackgroundColor, aClearOnNewScreen);
}

/*
 * Sets the Y position and the X start position after a newline
 * Positions are in pixel
 */
void BlueDisplay::setWriteStringPosition(uint16_t aPositionX, uint16_t aPositionY) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    printSetPosition(aPositionX, aPositionY);
#endif
    sendUSARTArgs(FUNCTION_WRITE_SETTINGS, 3, FLAG_WRITE_SETTINGS_SET_POSITION, aPositionX, aPositionY);
}

/*
 * Sets the Y position and the X start position after a newline
 * Positions are in character units :-)
 */
void BlueDisplay::setWriteStringPositionColumnLine(uint16_t aColumnNumber, uint16_t aLineNumber) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    printSetPositionColumnLine(aColumnNumber, aLineNumber);
#endif
    sendUSARTArgs(FUNCTION_WRITE_SETTINGS, 3, FLAG_WRITE_SETTINGS_SET_LINE_COLUMN, aColumnNumber, aLineNumber);
}

void BlueDisplay::writeString(const char *aStringPtr) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    myPrint(aStringPtr, strlen(aStringPtr));
#endif
    sendUSARTArgsAndByteBuffer(FUNCTION_WRITE_STRING, 0, strlen(aStringPtr), (uint8_t*) aStringPtr);
}

/*
 * Do an automatic break before a new word, which will not fit on the remainder of line!
 * \n is interpreted as a newline starting at left of screen
 * \r is interpreted as a space
 */
void BlueDisplay::writeString(const __FlashStringHelper *aPGMString) {
    if (USART_isBluetoothPaired()) {
#if defined(__AVR__)
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        uint8_t tStringLength = _clipAndCopyPGMString(tStringBuffer, aPGMString);
        sendUSARTArgsAndByteBuffer(FUNCTION_WRITE_STRING, 0, tStringLength, tStringBuffer);
#else
        uint8_t tStringLength = strlen(reinterpret_cast<const char*>(aPGMString));
        sendUSARTArgsAndByteBuffer(FUNCTION_WRITE_STRING, 0, tStringLength, (uint8_t*) aPGMString);
#endif
    }
}

void BlueDisplay::writeString(const char *aStringPtr, uint8_t aStringLength) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    myPrint(aStringPtr, aStringLength);
#endif
    sendUSARTArgsAndByteBuffer(FUNCTION_WRITE_STRING, 0, aStringLength, (uint8_t*) aStringPtr);
}

// for use in syscalls.c
extern "C" void writeStringC(const char *aStringPtr, uint8_t aStringLength) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    myPrint(aStringPtr, aStringLength);
#endif
    sendUSARTArgsAndByteBuffer(FUNCTION_WRITE_STRING, 0, aStringLength, (uint8_t*) aStringPtr);
}

/**
 * Output String as warning to log and present as toast for at least 500 ms, i.e. subsequent debugs are suppressed during 500 ms
 */
void BlueDisplay::debugMessage(const char *aStringPtr) {
    sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(aStringPtr), (uint8_t*) aStringPtr);
}

void BlueDisplay::debug(const char *aStringPtr) {
    sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(aStringPtr), (uint8_t*) aStringPtr);
}

void BlueDisplay::debug(const __FlashStringHelper *aPGMString) {
    if (USART_isBluetoothPaired()) {
#if defined(__AVR__)
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        uint8_t tStringLength = _clipAndCopyPGMString(tStringBuffer, aPGMString);
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, tStringLength, tStringBuffer);
#else
        uint8_t tStringLength = strlen(reinterpret_cast<const char*>(aPGMString));
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, tStringLength, (uint8_t*) aPGMString);
#endif
    }
}

/**
 * Output as warning to log and present as toast every 500 ms
 * Value is also printed in HEX notation
 */
void BlueDisplay::debug(uint8_t aByte) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[9];
// hhu -> unsigned char instead of unsigned int with u
#if defined(__AVR__)
        snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("%u 0x%02X"), aByte, aByte);
#else
        snprintf(tStringBuffer, sizeof(tStringBuffer), "%u 0x%02X", aByte, aByte);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessage string is 25 character.
 * Value is also printed in HEX notation
 */
void BlueDisplay::debug(const char *aMessage, uint8_t aByte) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE];
// hhu -> unsigned char instead of unsigned int with u
#if defined(__AVR__)
        snprintf_P(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, PSTR("%s%u 0x%02X"), aMessage, aByte, aByte);
#else
        snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%u 0x%02X", aMessage, aByte, aByte);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessage string is 30 character.
 * Value is NOT printed in HEX notation
 */
void BlueDisplay::debug(const char *aMessageStart, uint8_t aByte, const char *aMessageEnd) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE];
// hhu -> unsigned char instead of unsigned int with u
#if defined(__AVR__)
        snprintf_P(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, PSTR("%s%u%s"), aMessageStart, aByte,
                aMessageEnd);
#else
        snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%u%s", aMessageStart, aByte, aMessageEnd);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessage string is 24 character.
 * Value is also printed in HEX notation
 */
void BlueDisplay::debug(const char *aMessage, int8_t aByte) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE];
// hhd -> signed char instead of signed int with d
#if defined(__AVR__)
        snprintf_P(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, PSTR("%s%d 0x%02X"), aMessage, aByte, aByte);
#else
        snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%d 0x%02X", aMessage, aByte, aByte);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

void BlueDisplay::debug(int8_t aByte) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[10];
// hhd -> signed char instead of int with d
#if defined(__AVR__)
        snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("%d 0x%02X"), aByte, aByte);
#else
        snprintf(tStringBuffer, sizeof(tStringBuffer), "%d 0x%02X", aByte, aByte);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

void BlueDisplay::debug(uint16_t aShort) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[13]; //5 decimal + 3 " 0x" + 4 hex +1
// hu -> unsigned short int instead of unsigned int with u
#if defined(__AVR__)
        snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("%u 0x%04X"), aShort, aShort);
#else
        snprintf(tStringBuffer, sizeof(tStringBuffer), "%u 0x%04X", aShort, aShort);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

void BlueDisplay::debug(int16_t aShort) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[14]; //6 decimal + 3 " 0x" + 4 hex +1
// hd -> short int instead of int with d
#if defined(__AVR__)
        snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("%6 0x%04X"), aShort, aShort);
#else
        snprintf(tStringBuffer, sizeof(tStringBuffer), "%d 0x%04X", aShort, aShort);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessage string is 21 character.
 * Value is also printed in HEX notation
 */
void BlueDisplay::debug(const char *aMessage, uint16_t aShort) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE];
// hd -> short int instead of int with d
#if defined(__AVR__)
        snprintf_P(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, PSTR("%s%u 0x%04X"), aMessage, aShort, aShort);
#else
        snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%u 0x%04X", aMessage, aShort, aShort);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessageStart + aMessageEnd string is 28 character.
 * Value is NOT printed in HEX notation
 */
void BlueDisplay::debug(const char *aMessageStart, uint16_t aShort, const char *aMessageEnd) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE];
// hd -> short int instead of int with d
#if defined(__AVR__)
        snprintf_P(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, PSTR("%s%u%s"), aMessageStart, aShort,
                aMessageEnd);
#else
        snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%u%s", aMessageStart, aShort, aMessageEnd);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessage string is 20 character.
 * Value is also printed in HEX notation
 */
void BlueDisplay::debug(const char *aMessage, int16_t aShort) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE];
// hd -> short int instead of int with d
#if defined(__AVR__)
        snprintf_P(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, PSTR("%s%d 0x%04X"), aMessage, aShort, aShort);
#else
        snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%d 0x%04X", aMessage, aShort, aShort);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

void BlueDisplay::debug(uint32_t aLong) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[22]; //10 decimal + 3 " 0x" + 8 hex +1
#if defined(__AVR__)
        snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("%lu 0x%0lX"), aLong, aLong);
#elif defined(__XTENSA__)
        snprintf(tStringBuffer, sizeof(tStringBuffer), "%lu 0x%0lX", (long) aLong, (long) aLong);
#else
        snprintf(tStringBuffer, sizeof(tStringBuffer), "%lu 0x%0lX", aLong, aLong);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

void BlueDisplay::debug(int32_t aLong) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[23]; //11 decimal + 3 " 0x" + 8 hex +1
#if defined(__AVR__)
        snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("%ld 0x%0lX"), aLong, aLong);
#elif defined(__XTENSA__)
        snprintf(tStringBuffer, sizeof(tStringBuffer), "%ld 0x%0lX", (long) aLong, (long) aLong);
#else
        snprintf(tStringBuffer, sizeof(tStringBuffer), "%ld 0x%0lX", aLong, aLong);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessage string is 13 to 20 character depending on content of aLong.
 * Value is also printed in HEX notation
 */
void BlueDisplay::debug(const char *aMessage, uint32_t aLong) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE];
#if defined(__AVR__)
        snprintf_P(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, PSTR("%s%lu 0x%0lX"), aMessage, aLong, aLong);
#elif defined(__XTENSA__)
        snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%lu 0x%0lX", aMessage, (long) aLong, (long) aLong);
#else
        snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%lu 0x%0lX", aMessage, aLong, aLong);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessageStart + aMessageEnd string is 23 character.
 * Value is NOT printed in HEX notation
 */
void BlueDisplay::debug(const char *aMessageStart, uint32_t aLong, const char *aMessageEnd) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE];
#if defined(__AVR__)
        snprintf_P(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, PSTR("%s%lu%s"), aMessageStart, aLong,
                aMessageEnd);
#elif defined(__XTENSA__)
        snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%lu%s", aMessageStart, (long) aLong, aMessageEnd);
#else
        snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%lu%s", aMessageStart, aLong, aMessageEnd);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessage string is 12 to 19 character depending on content of aLong.
 * Value is also printed in HEX notation
 */
void BlueDisplay::debug(const char *aMessage, int32_t aLong) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE];
#if defined(__AVR__)
        snprintf_P(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, PSTR("%s%ld 0x%0lX"), aMessage, aLong, aLong);
#elif defined(__XTENSA__)
        snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%ld 0x%0lX", aMessage, (long) aLong, (long) aLong);
#else
        snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%ld 0x%0lX", aMessage, aLong, aLong);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

void BlueDisplay::debug(float aFloat) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[22];
#if defined(__AVR__)
        dtostrf(aFloat, 16, 7, tStringBuffer);
#else
        snprintf(tStringBuffer, sizeof(tStringBuffer), "%f", aFloat);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/*
 * Maximum size of aMessage string is 12 character because 17 characters on content of aFloat.
 */
void BlueDisplay::debug(const char *aMessage, float aFloat) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE]; // 34 chars
#if defined(__AVR__)
        strncpy(tStringBuffer, aMessage, (STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE - 22));
        tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE - 22] = '\0'; // Terminate strings which are too long
        dtostrf(aFloat, 16, 7, &tStringBuffer[strlen(tStringBuffer)]);
        tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE - 1] = '\0'; // Terminate floats which are too long
//    snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%f", aMessage, (double)aFloat); // requires ca. 800 bytes more
#else
        snprintf(tStringBuffer, STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE, "%s%f", aMessage, aFloat);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

void BlueDisplay::debug(double aDouble) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[22];
#if defined(__AVR__)
        dtostrf(aDouble, 16, 7, tStringBuffer);
#else
        snprintf(tStringBuffer, sizeof(tStringBuffer), "%f", aDouble);
#endif
        sendUSARTArgsAndByteBuffer(FUNCTION_DEBUG_STRING, 0, strlen(tStringBuffer), tStringBuffer);
    }
}

/**
 * if aClearBeforeColor != 0 then previous line is cleared before
 */
void BlueDisplay::drawChartByteBuffer(uint16_t aXOffset, uint16_t aYOffset, color16_t aColor, color16_t aClearBeforeColor,
        uint8_t *aByteBuffer, size_t aByteBufferLength) {
    sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_CHART, 4, aXOffset, aYOffset, aColor, aClearBeforeColor, aByteBufferLength,
            aByteBuffer);
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

/**
 * if aClearBeforeColor != 0 then previous line is cleared before
 * chart index is coded in the upper 4 bits of aYOffset
 *
 * aIntegerScaleFactor > 1 : expansion by factor aIntegerScaleFactor
 * Factor == 1 : expansion by 1.5
 * Factor == 0 : identity
 * Factor == -1 : compression by 1.5
 * Factor < -1 : compression by factor aIntegerScaleFactor
 */
void BlueDisplay::drawChartByteBufferScaled(uint16_t aXOffset, uint16_t aYOffset, int16_t aIntegerXScaleFactor, float aYScaleFactor,
        uint8_t aLineSize, uint8_t aChartMode, color16_t aColor, color16_t aClearBeforeColor, uint8_t aChartIndex,
        bool aDoDrawDirect, uint8_t *aByteBuffer, size_t aByteBufferLength) {
    if (USART_isBluetoothPaired()) {
        aYOffset = aYOffset | ((aChartIndex & 0x0F) << 12);
        uint8_t tFunctionTag = FUNCTION_DRAW_SCALED_CHART_WITHOUT_DIRECT_RENDERING;
        if (aDoDrawDirect) {
            tFunctionTag = FUNCTION_DRAW_SCALED_CHART;
        }
        union {
            float floatValue;
            uint16_t shortArray[2];
        } floatToShortArray;
        floatToShortArray.floatValue = aYScaleFactor;
        sendUSARTArgsAndByteBuffer(tFunctionTag, 9, aXOffset, aYOffset, aIntegerXScaleFactor, floatToShortArray.shortArray[0],
                floatToShortArray.shortArray[1], aLineSize, aChartMode, aColor, aClearBeforeColor, aByteBufferLength, aByteBuffer);
    }
}

uint32_t BlueDisplay::getHostUnixTimestamp() {
    return mHostUnixTimestamp;
}

void BlueDisplay::setHostUnixTimestamp(uint32_t aHostUnixTimestamp) {
    mHostUnixTimestamp = aHostUnixTimestamp;
}

struct XYSize* BlueDisplay::getHostDisplaySize() {
    return &mHostDisplaySize;
}

uint16_t BlueDisplay::getHostDisplayWidth() {
    return mHostDisplaySize.XWidth;
}

uint16_t BlueDisplay::getHostDisplayHeight() {
    return mHostDisplaySize.YHeight;
}

struct XYSize* BlueDisplay::getRequestedDisplaySize() {
    return &mRequestedDisplaySize;
}

uint16_t BlueDisplay::getRequestedDisplayWidth() {
    return mRequestedDisplaySize.XWidth;
}

uint16_t BlueDisplay::getRequestedDisplayHeight() {
    return mRequestedDisplaySize.YHeight;
}

bool BlueDisplay::isDisplayOrientationLandscape() {
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
        drawLineWithThickness(aLine->StartX, aLine->StartY, aLine->EndX, aLine->EndY, aLine->BackgroundColor, aLine->Thickness);
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

        drawLineWithThickness(aLine->StartX, aLine->StartY, tNewEndX, tNewEndY, aLine->Color, aLine->Thickness);
    }
}

// for use in syscalls.c
extern "C" uint16_t drawTextC(uint16_t aPositionX, uint16_t aPositionY, const char *aStringPtr, uint16_t aFontSize,
        color16_t aTextColor, uint16_t aBackgroundColor) {
    uint16_t tRetValue = 0;
    if (USART_isBluetoothPaired()) {
        tRetValue = BlueDisplay1.drawText(aPositionX, aPositionY, (char*) aStringPtr, aFontSize, aTextColor, aBackgroundColor);
    }
    return tRetValue;
}

#if defined(__AVR__)
uint16_t BlueDisplay::drawTextPGM(uint16_t aPositionX, uint16_t aPositionY, const char *aPGMString, uint16_t aFontSize,
        color16_t aTextColor, color16_t aBackgroundColor) {
    uint16_t tRetValue = 0;
    char tStringBuffer[STRING_BUFFER_STACK_SIZE];
    uint8_t tTextLength = _clipAndCopyPGMString(tStringBuffer, reinterpret_cast<const __FlashStringHelper*>(aPGMString));
#  if defined(SUPPORT_LOCAL_DISPLAY)
    tRetValue = LocalDisplay.drawText(aPositionX, aPositionY - getTextAscend(aFontSize),
            reinterpret_cast<const __FlashStringHelper*>(aPGMString), aFontSize, aTextColor, aBackgroundColor);
#  endif
    if (USART_isBluetoothPaired()) {
        tRetValue = aPositionX + tTextLength * getTextWidth(aFontSize);
        sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 5, aPositionX, aPositionY, aFontSize, aTextColor, aBackgroundColor,
                tTextLength, (uint8_t*) tStringBuffer);
    }
    return tRetValue;
}

void BlueDisplay::drawTextPGM(uint16_t aPositionX, uint16_t aPositionY, const char *aPGMString) {
    char tStringBuffer[STRING_BUFFER_STACK_SIZE];
    uint8_t tTextLength = _clipAndCopyPGMString(tStringBuffer, reinterpret_cast<const __FlashStringHelper*>(aPGMString));
#  if defined(SUPPORT_LOCAL_DISPLAY)
    LocalDisplay.drawText(aPositionX, aPositionY - getTextAscend(TEXT_SIZE_11),
            reinterpret_cast<const __FlashStringHelper*>(aPGMString), tTextLength, COLOR16_BLACK, COLOR16_WHITE);
#  endif
    sendUSARTArgsAndByteBuffer(FUNCTION_DRAW_STRING, 2, aPositionX, aPositionY, tTextLength, (uint8_t*) tStringBuffer);
}
#endif // defined(__AVR__)

/***************************************************************************************************************************************************
 *
 * INPUT
 *
 **************************************************************************************************************************************************/
/*
 * If the user enters avalid number and presses OK, it sends a message over bluetooth back to arduino which contains the float value
 */
void BlueDisplay::getNumber(void (*aNumberHandler)(float)) {
#if __SIZEOF_POINTER__ == 4
    sendUSARTArgs(FUNCTION_GET_NUMBER, 2, aNumberHandler, (reinterpret_cast<uint32_t>(aNumberHandler) >> 16));
#else
    sendUSARTArgs(FUNCTION_GET_NUMBER, 1, aNumberHandler);
#endif
}

/*
 * Message size 1 or 2 shorts
 */
void BlueDisplay::getNumberWithShortPrompt(void (*aNumberHandler)(float), const char *aShortPromptString) {
#if __SIZEOF_POINTER__ == 4
    sendUSARTArgsAndByteBuffer(FUNCTION_GET_NUMBER_WITH_SHORT_PROMPT, 2, aNumberHandler,
            (reinterpret_cast<uint32_t>(aNumberHandler) >> 16), strlen(aShortPromptString), (uint8_t*) aShortPromptString);
#else
    sendUSARTArgsAndByteBuffer(FUNCTION_GET_NUMBER_WITH_SHORT_PROMPT, 1, aNumberHandler, strlen(aShortPromptString),
            (uint8_t*) aShortPromptString);
#endif
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
#if __SIZEOF_POINTER__ == 4
    sendUSARTArgs(FUNCTION_GET_INFO, 3, aInfoSubcommand, aInfoHandler, (reinterpret_cast<uint32_t>(aInfoHandler) >> 16));
#else
    sendUSARTArgs(FUNCTION_GET_INFO, 2, aInfoSubcommand, aInfoHandler);
#endif
}

/*
 *  This results in a data event
 */
void BlueDisplay::requestMaxCanvasSize() {
    sendUSARTArgs(FUNCTION_REQUEST_MAX_CANVAS_SIZE, 0);
}

#if defined(__AVR__)
void BlueDisplay::getNumberWithShortPromptPGM(void (*aNumberHandler)(float), const char *aPGMShortPromptString) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        uint8_t tShortPromptLength = _clipAndCopyPGMString(tStringBuffer,
                reinterpret_cast<const __FlashStringHelper*>(aPGMShortPromptString));
        sendUSARTArgsAndByteBuffer(FUNCTION_GET_NUMBER_WITH_SHORT_PROMPT, 1, aNumberHandler, tShortPromptLength,
                (uint8_t*) tStringBuffer);
    }
}

void BlueDisplay::getNumberWithShortPromptPGM(void (*aNumberHandler)(float), const char *aPGMShortPromptString,
        float aInitialValue) {
    if (USART_isBluetoothPaired()) {
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        uint8_t tShortPromptLength = _clipAndCopyPGMString(tStringBuffer,
                reinterpret_cast<const __FlashStringHelper*>(aPGMShortPromptString));
        union {
            float floatValue;
            uint16_t shortArray[2];
        } floatToShortArray;
        floatToShortArray.floatValue = aInitialValue;
        sendUSARTArgsAndByteBuffer(FUNCTION_GET_NUMBER_WITH_SHORT_PROMPT, 3, aNumberHandler, floatToShortArray.shortArray[0],
                floatToShortArray.shortArray[1], tShortPromptLength, (uint8_t*) tStringBuffer);
    }
}
#endif

void BlueDisplay::getNumberWithShortPrompt(void (*aNumberHandler)(float), const __FlashStringHelper *aPGMShortPromptString) {
    if (USART_isBluetoothPaired()) {
#if defined(__AVR__)
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        uint8_t tShortPromptLength = _clipAndCopyPGMString(tStringBuffer, aPGMShortPromptString);
        sendUSARTArgsAndByteBuffer(FUNCTION_GET_NUMBER_WITH_SHORT_PROMPT, 1, aNumberHandler, tShortPromptLength,
                (uint8_t*) tStringBuffer);
#else
        getNumberWithShortPrompt(aNumberHandler, (const char*) aPGMShortPromptString);
#endif
    }
}

void BlueDisplay::getNumberWithShortPrompt(void (*aNumberHandler)(float), const __FlashStringHelper *aPGMShortPromptString,
        float aInitialValue) {
    if (USART_isBluetoothPaired()) {
#if defined(__AVR__)
        char tStringBuffer[STRING_BUFFER_STACK_SIZE];
        uint8_t tShortPromptLength = _clipAndCopyPGMString(tStringBuffer, aPGMShortPromptString);
        union {
            float floatValue;
            uint16_t shortArray[2];
        } floatToShortArray;
        floatToShortArray.floatValue = aInitialValue;
        sendUSARTArgsAndByteBuffer(FUNCTION_GET_NUMBER_WITH_SHORT_PROMPT, 3, aNumberHandler, floatToShortArray.shortArray[0],
                floatToShortArray.shortArray[1], tShortPromptLength, (uint8_t*) tStringBuffer);
#else
        getNumberWithShortPrompt(aNumberHandler, (const char*) aPGMShortPromptString, aInitialValue);
#endif
    }
}

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
    sendUSARTArgs(FUNCTION_SENSOR_SETTINGS, 4, aSensorType, aDoActivate, aSensorRate, aFilterFlag);
}

/***************************************************************************************************************************************************
 *
 * Utilities
 *
 **************************************************************************************************************************************************/
#if defined(F) && defined(ARDUINO)
/*
 * Used internally
 */
uint8_t _clipAndCopyPGMString(char *aStringBuffer, const __FlashStringHelper *aPGMString) {
    PGM_P tPGMString = reinterpret_cast<PGM_P>(aPGMString);
    /*
     * compute string length
     */
    uint8_t tLength = strlen_P(tPGMString);
    if (tLength > STRING_BUFFER_STACK_SIZE) {
        tLength = STRING_BUFFER_STACK_SIZE;
    }
    /*
     * copy string up to length
     */
    strncpy_P(aStringBuffer, tPGMString, tLength);
    return tLength;
}
#endif

void clearDisplayAndDisableButtonsAndSliders() {
    BlueDisplay1.clearDisplay();
    BDButton::deactivateAll();
    BDSlider::deactivateAll();
}

void clearDisplayAndDisableButtonsAndSliders(color16_t aColor) {
    BlueDisplay1.clearDisplay(aColor);
    BDButton::deactivateAll();
    BDSlider::deactivateAll();
}

/*****************************************************************************
 * Display and drawing tests
 *****************************************************************************/
/**
 * Draws a star consisting of 4 lines each quadrant
 * Draw 6 lines with aliasing Paint, which can be removed without resiudual
 */
void BlueDisplay::drawStar(int aXCenter, int aYCenter, int tOffsetCenter, int tLength, int tOffsetDiagonal, int aLengthDiagonal,
        color16_t aColor, int16_t aThickness) {

    int X = aXCenter + tOffsetCenter;
// first draw right then left lines
    for (int i = 0; i < 2; i++) {
        // horizontal line
        drawLineRelWithThickness(X, aYCenter, tLength, 0, aColor, aThickness);
        // two lines adjacent to horizontal line ( < 45 degree)
        drawLineRelWithThicknessWithAliasing(X, aYCenter - tOffsetDiagonal, tLength, -aLengthDiagonal, aColor, aThickness);
        drawLineRelWithThickness(X, aYCenter + tOffsetDiagonal, tLength, aLengthDiagonal, aColor, aThickness);
        X = aXCenter - tOffsetCenter;
        tLength = -tLength;
    }

    int Y = aYCenter + tOffsetCenter;
// first draw lower then upper lines
    for (int i = 0; i < 2; i++) {
        // vertical line
        drawLineRelWithThickness(aXCenter, Y, 0, tLength, aColor, aThickness);
        // two lines adjacent to vertical line
        drawLineRelWithThicknessWithAliasing(aXCenter - tOffsetDiagonal, Y, -aLengthDiagonal, tLength, aColor, aThickness);
        drawLineRelWithThickness(aXCenter + tOffsetDiagonal, Y, aLengthDiagonal, tLength, aColor, aThickness);
        Y = aYCenter - tOffsetCenter;
        tLength = -tLength;
    }

    X = aXCenter + tOffsetCenter;
    int tLengthDiagonal = tLength;
    for (int i = 0; i < 2; i++) {
        // draw two 45 degree lines
        drawLineRelWithThicknessWithAliasing(X, aYCenter - tOffsetCenter, tLength, -tLengthDiagonal, aColor, aThickness);
        drawLineRelWithThickness(X, aYCenter + tOffsetCenter, tLength, tLengthDiagonal, aColor, aThickness);
        X = aXCenter - tOffsetCenter;
        tLength = -tLength;
    }

    drawPixel(aXCenter, aYCenter, COLOR16_BLUE);
}

/**
 * Draws two greyscales and 3 color bars
 */
void BlueDisplay::drawGreyscale(uint16_t aXPos, uint16_t tYPos, uint16_t aHeight) {
    uint16_t tY;
    for (int i = 0; i <= 0xFF; ++i) {
        tY = tYPos;
        drawLineRel(aXPos, tY, 0, aHeight, COLOR16(i, i, i));
        tY += aHeight;
        drawLineRel(aXPos, tY, 0, aHeight, COLOR16((0xFF - i), (0xFF - i), (0xFF - i)));
        tY += aHeight;
        drawLineRel(aXPos, tY, 0, aHeight, COLOR16(i, 0, 0));
        tY += aHeight;
        drawLineRel(aXPos, tY, 0, aHeight, COLOR16(0, i, 0));
        tY += aHeight;
        // For Test purposes: drawLineRel instead of fillRectRel gives missing lines on e.g. Nexus 7
        fillRectRel(aXPos, tY, 1, aHeight, COLOR16(0, 0, i));
        aXPos++;
#ifdef HAL_WWDG_MODULE_ENABLED
        Watchdog_reload();
#endif
    }
}

/**
 * Draws test page and a greyscale bar
 */
void BlueDisplay::testDisplay() {
    BlueDisplay1.clearDisplay(COLOR16_WHITE);
    drawText(0, 11, F("Left"), 11, COLOR16_BLACK, COLOR16_WHITE);
    drawText(STRING_ALIGN_MIDDLE_XPOS, 11, F("Middle"));
    drawText(STRING_ALIGN_RIGHT_XPOS, 11, F("Right"));
    delay(1000);

    // array for colors, which can be cleared for 2. loop
    color16_t tColorArray[5] = { COLOR16_RED, COLOR16_GREEN, COLOR16_BLUE, COLOR16_BLACK, COLOR16_YELLOW };

    for (uint8_t i = 0; i < 2; ++i) {

        /*
         * rectangles in all 4 corners
         */
        fillRectRel(0, 0, 2, 2, tColorArray[0]);
        fillRectRel(mRequestedDisplaySize.XWidth, 0, 3, -3, tColorArray[1]);
        fillRectRel(0, mRequestedDisplaySize.YHeight, 4, -4, tColorArray[2]);
        fillRectRel(mRequestedDisplaySize.XWidth, mRequestedDisplaySize.YHeight, -3, -3, tColorArray[3]);
        /*
         * small graphics in the upper left corner
         */
        fillRectRel(2, 2, 4, 4, tColorArray[0]);
        fillRectRel(10, 20, 10, 20, tColorArray[0]);
        drawRectRel(8, 18, 14, 24, tColorArray[2], 1);
        drawCircle(15, 30, 5, tColorArray[2], 1);
        fillCircle(20, 10, 10, tColorArray[2]);

        /*
         * Diagonal blue and green line
         */
        drawLineRel(0, mRequestedDisplaySize.YHeight - 1, mRequestedDisplaySize.XWidth - 1, -(mRequestedDisplaySize.YHeight - 1),
                tColorArray[1]);
        // Top left to bottom right
        drawLineRelWithAliasing(6, 6, mRequestedDisplaySize.XWidth - 9, mRequestedDisplaySize.YHeight - 9, tColorArray[2]);

        /*
         * Character and text
         */
        drawChar(150, TEXT_SIZE_11_ASCEND, 'y', TEXT_SIZE_11, tColorArray[1], COLOR16_YELLOW);
        drawText(0, 50 + TEXT_SIZE_11_ASCEND, "Calibration", TEXT_SIZE_11, COLOR16_BLACK, COLOR16_WHITE);
        drawText(0, 50 + TEXT_SIZE_11_HEIGHT + TEXT_SIZE_11_ASCEND, "Calibration", TEXT_SIZE_11, COLOR16_WHITE, COLOR16_BLACK);

#if defined(SUPPORT_LOCAL_DISPLAY)
    /*
     * 4 red lines in the middle with different overlaps
     */
    drawLineOverlap(120, 160, 180, 120, LINE_OVERLAP_NONE, tColorArray[0]);
    drawLineOverlap(120, 164, 180, 124, LINE_OVERLAP_MAJOR, tColorArray[0]);
    drawLineOverlap(120, 168, 180, 128, LINE_OVERLAP_MINOR, tColorArray[0]);
    drawLineOverlap(120, 172, 180, 132, LINE_OVERLAP_BOTH, tColorArray[0]);
#endif

        /*
         * 4 small red and black rectangles middle left
         */
        fillRectRel(100, 100, 10, 5, tColorArray[0]);
        fillRectRel(90, 95, 10, 5, tColorArray[0]);
        fillRectRel(100, 90, 10, 10, tColorArray[3]);
        fillRectRel(95, 100, 5, 5, tColorArray[3]);

        /*
         * stars middle
         */
        drawStar(130, 120, 4, 6, 2, 2, tColorArray[3], 1);
        drawStar(210, 120, 8, 12, 4, 4, tColorArray[3], 1);
        drawStar(255, 120, 8, 12, 4, 4, tColorArray[1], 3);

        /*
         * draw grid, to detect different line size dependent of line position
         */
        for (uint8_t tLineoffset = 0; tLineoffset < 40; tLineoffset += 8) {
            // draw 1 pixel vertical lines
            drawLineRel(280 + tLineoffset, 80, 0, 32, tColorArray[3]);
            // draw 1 pixel horizontal lines
            drawLineRelWithThickness(280, 80 + tLineoffset, 32, 0, tColorArray[3], 1);

            // draw 1 pixel vertical lines using different paint object on host
            drawLineRel(280 + tLineoffset, 130, 0, 32, tColorArray[3]);
            // draw 1 pixel horizontal lines using different paint object on host
            drawLineRelWithThickness(280, 130 + tLineoffset, 32, 0, tColorArray[3], 1);
        }

        uint16_t DeltaSmall = 20;
        uint16_t DeltaBig = 100;
        uint16_t tYPos = 30;

        /*
         * Two 4 pixel thick lines left
         */
        tYPos = 75;
        drawLineWithThickness(10, tYPos, 10 + DeltaSmall, tYPos + DeltaBig, tColorArray[1], 4);
        drawPixel(10, tYPos, tColorArray[2]);

        drawLineWithThicknessWithAliasing(70, tYPos, 70 - DeltaSmall, tYPos + DeltaBig, tColorArray[1], 4);
        drawPixel(70, tYPos, tColorArray[2]);

        /*
         * Cross with 3 pixel thick lines top middle
         */
        tYPos = 55;
        drawLineWithThickness(140, tYPos, 140 - DeltaSmall, tYPos - DeltaSmall, tColorArray[1], 3);
        drawPixel(140, tYPos, tColorArray[2]);

        drawLineWithThicknessWithAliasing(150, tYPos, 150 + DeltaSmall, tYPos - DeltaSmall, tColorArray[1], 3);
        drawPixel(150, tYPos, tColorArray[2]);

        tYPos += 10;
        drawLineWithThickness(140, tYPos, 140 - DeltaSmall, tYPos + DeltaSmall, tColorArray[1], 3);
        drawPixel(140, tYPos, tColorArray[2]);

        drawLineWithThicknessWithAliasing(150, tYPos, 150 + DeltaSmall, tYPos + DeltaSmall, tColorArray[1], 3);
        drawPixel(150, tYPos, tColorArray[2]);

#if defined(SUPPORT_LOCAL_DISPLAY)
    /*
     * 2 3 pixel thick lines top middle-right drawn clockwise by drawThickLine()
     */
    tYPos = 55;
    drawThickLine(200, tYPos, 200 - DeltaSmall, tYPos - DeltaSmall, 3, LINE_THICKNESS_MIDDLE, tColorArray[0]);
    drawPixel(200, tYPos, tColorArray[2]);

    drawThickLine(210, tYPos, 210 + DeltaSmall, tYPos - DeltaSmall, 3, LINE_THICKNESS_MIDDLE, tColorArray[0]);
    drawPixel(210, tYPos, tColorArray[2]);

    tYPos += 10;
    drawThickLine(200, tYPos, 200 - DeltaSmall, tYPos + DeltaSmall, 3, LINE_THICKNESS_DRAW_CLOCKWISE, tColorArray[0]);
    drawPixel(200, tYPos, tColorArray[2]);

    drawThickLine(210, tYPos, 210 + DeltaSmall, tYPos + DeltaSmall, 3, LINE_THICKNESS_DRAW_COUNTERCLOCKWISE, tColorArray[0]);
    drawPixel(210, tYPos, tColorArray[2]);

    /*
     * 2 9 pixel thick lines top middle drawn clockwise by drawThickLine()
     */
    tYPos = 30;
    drawThickLine(140, tYPos, 140 - DeltaBig, tYPos - DeltaSmall, 9, LINE_THICKNESS_MIDDLE, tColorArray[0]);
    drawPixel(140, tYPos, tColorArray[2]);

    drawThickLine(145, tYPos, 145 + DeltaBig, tYPos - DeltaSmall, 9, LINE_THICKNESS_MIDDLE, tColorArray[0]);
    drawPixel(145, tYPos, tColorArray[2]);
#endif
        /*
         * Draw two greyscales and 3 color bars
         */
        drawGreyscale(5, 180, 10);

        /*
         * Clear color array for next loop
         */
        for (uint8_t j = 0; j < sizeof(tColorArray) / sizeof(tColorArray[0]); ++j) {
            tColorArray[j] = COLOR16_WHITE;
        }
        delayMillisAndCheckForEvent(10000); // wait 10 seconds and then clear items, to see residual
    }
}

#define COLOR_SPECTRUM_SEGMENTS 6 // red->yellow, yellow-> green, green-> cyan, cyan-> blue, blue-> magent, magenta-> red
#define COLOR_RESOLUTION 32 // 32 (5 bit) different red colors for 16 bit color (green really has 6 bit, but we don't use 6 bit)
const uint16_t colorIncrement[COLOR_SPECTRUM_SEGMENTS] = { 1 << 6, 0x1FU << 11, 1, 0x3FFU << 6, 1 << 11, 0xFFFFU };

/**
 * generates a full color spectrum beginning with a black line,
 * increasing saturation to full colors and then fading to a white line
 * customized for a 320 x 240 display
 */
void BlueDisplay::generateColorSpectrum() {
    clearDisplay();
    uint16_t tColor;
    uint16_t tXPos;
    uint16_t tDelta;
    uint16_t tError;

    uint16_t tColorChangeAmount;
    uint16_t tYpos = mRequestedDisplaySize.YHeight;
    uint16_t tColorLine;
    for (unsigned int line = 4; line < mRequestedDisplaySize.YHeight + 4U; ++line) {
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
        for (unsigned int i = 0; i < COLOR_SPECTRUM_SEGMENTS; ++i) {
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
            for (unsigned int j = 0; j < (mRequestedDisplaySize.XWidth / COLOR_SPECTRUM_SEGMENTS) - 1U; ++j) {
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
#if defined(LOCAL_DEBUG)
#undef LOCAL_DEBUG
#endif
#if defined(LOCAL_TRACE)
#undef LOCAL_TRACE
#endif
#endif // _BLUEDISPLAY_HPP
