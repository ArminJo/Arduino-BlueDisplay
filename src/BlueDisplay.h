/*
 * BlueDisplay.h
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
 * Text Y and X position is upper left corner of character
 * Text Y bottom position is position + TextSize
 * Text Y middle position is position + TextSize / 2
 *
 * Slider position is upper left corner of slider
 * Button position is upper left corner of button
 * If button color is COLOR16_NO_BACKGROUND only a text button without background is rendered
 */

#ifndef _BLUEDISPLAY_H
#define _BLUEDISPLAY_H

#define VERSION_BLUE_DISPLAY "5.0.0"
#define VERSION_BLUE_DISPLAY_MAJOR 5
#define VERSION_BLUE_DISPLAY_MINOR 0
#define VERSION_BLUE_DISPLAY_PATCH 0
// The change log is at the bottom of the file

/*
 * Macro to convert 3 version parts into an integer
 * To be used in preprocessor comparisons, such as #if VERSION_BLUE_DISPLAY_HEX >= VERSION_HEX_VALUE(3, 7, 0)
 */
#define VERSION_HEX_VALUE(major, minor, patch) ((major << 16) | (minor << 8) | (patch))
#define VERSION_BLUE_DISPLAY_HEX  VERSION_HEX_VALUE(VERSION_BLUE_DISPLAY_MAJOR, VERSION_BLUE_DISPLAY_MINOR, VERSION_BLUE_DISPLAY_PATCH)

#define CONNECTIOM_TIMEOUT_MILLIS   1500    // Timeout for initCommunication() attempts to connect to BD host
#define HELPFUL_DELAY_BETWEEN_DRAWING_CHART_LINES_TO_STABILIZE_USB_CONNECTION   50 // without, the USB connection to my Samsung SM-T560 skips some bytes :-(
#define HELPFUL_DELAY_BETWEEN_DRAWING_CHART_LINES_TO_STABILIZE_BT_CONNECTION   200 // without, the BT connection to my Nexus 7 breaks after a few seconds :-(

// Helper macro for getting a macro definition as string
#if !defined(STR)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif

/*
 * If both macros are enabled, LocalGUI.hpp should be included and TouchButton and TouchSlider instead of BDButton and BDSlider used.
 */
//#define DISABLE_REMOTE_DISPLAY // Allow only drawing on the locally attached display by suppress using Bluetooth serial by defining USART_isBluetoothPaired() to return constant false.
//#define SUPPORT_LOCAL_DISPLAY  // Supports simultaneously drawing on the locally attached display. Not (yet) implemented for all commands!
#if !defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY) && defined(SUPPORT_LOCAL_DISPLAY) && !defined(DISABLE_REMOTE_DISPLAY)
# define SUPPORT_REMOTE_AND_LOCAL_DISPLAY // Both displays used simultaneously. Definition is used internally to avoid #if defined(SUPPORT_LOCAL_DISPLAY) && !defined(DISABLE_REMOTE_DISPLAY)
#endif

#if !defined(SUPPORT_ONLY_REMOTE_DISPLAY) && !defined(SUPPORT_LOCAL_DISPLAY) && !defined(DISABLE_REMOTE_DISPLAY)
# define SUPPORT_ONLY_REMOTE_DISPLAY
#endif

#if !defined(SUPPORT_ONLY_LOCAL_DISPLAY) && defined(SUPPORT_LOCAL_DISPLAY) && defined(DISABLE_REMOTE_DISPLAY)
# define SUPPORT_ONLY_LOCAL_DISPLAY
#endif

#if !defined(SUPPORT_LOCAL_DISPLAY) && defined(DISABLE_REMOTE_DISPLAY)
#error DISABLE_REMOTE_DISPLAY is defined but SUPPORT_LOCAL_DISPLAY is not defined! No-display is not supported ;-). Seems you forgot to #define SUPPORT_LOCAL_DISPLAY.
#endif

#if defined(ARDUINO)
#  if !defined(ESP32)
// For not AVR platforms this contains mapping defines (at least for STM32)
#include <avr/pgmspace.h>
#  endif
#  if defined(strcpy_P) // check if we have mapping defines
#    if ! defined(strncpy_P)
// this define is not included in the pgmspace.h file :-(
#    define strncpy_P(dest, src, size) strncpy((dest), (src), (size))
#    endif
#  endif
#elif !defined(PROGMEM)
#define PROGMEM
#define PGM_P  const char *
#define PSTR(str) (str)
#ifdef __cplusplus
class __FlashStringHelper;
#define F(string_literal) (reinterpret_cast<const __FlashStringHelper *>(PSTR(string_literal)))
#endif
#define _SFR_BYTE(n) (n)
#endif

#include "Colors.h"
#include "GUIHelper.h"

#include "BlueDisplayProtocol.h"
#include "BlueSerial.h"
#include "EventHandler.h"

#ifdef __cplusplus
#include "BDButton.h" // for BDButtonIndex_t
#include "BDSlider.h" // for BDSliderIndex_t
#endif

/*****************************
 * Constants used in protocol
 *****************************/
//#define COLOR16_NO_BACKGROUND   ((color16_t)0XFFFE)
static const float NUMBER_INITIAL_VALUE_DO_NOT_SHOW = 1e-40f;

/**********************
 * Basic
 *********************/
// Sub functions for SET_FLAGS_AND_SIZE / setFlagsAndSize()
// Reset buttons, sliders, sensors, orientation locking, flags (see next lines) and character mappings
static const int BD_FLAG_FIRST_RESET_ALL = 0x01;
//
static const int BD_FLAG_TOUCH_BASIC_DISABLE = 0x02; // Do not send plain touch events (UP, DOWN, MOVE) if no button or slider was touched, send only button and slider events. -> Disables also touch moves.
static const int BD_FLAG_ONLY_TOUCH_MOVE_DISABLE = 0x04; // Do not send MOVE, only UP and DOWN.
static const int BD_FLAG_LONG_TOUCH_ENABLE = 0x08; // If long touch detection is required. This delays the sending of plain DOWN Events.
static const int BD_FLAG_USE_MAX_SIZE = 0x10;      // Use maximum display size for given geometry. -> Scale automatically to screen.
// Here we use the same values as below, but shifted by 8 bytes
/*********************************************
 * Flags for setScreenOrientationLock()
 * We have almost same values as used in Android
 * LANDSCAPE is 0 on Android, but we abuse the value 3 of SCREEN_ORIENTATION_BEHIND for it to have 0 as unlock
 *********************************************/
static const int BD_FLAG_SCREEN_ORIENTATION_LOCK_UNLOCK = 0x0000; // Unlock is -1 on Android, Landscape is 0 on Android,
static const int BD_FLAG_SCREEN_ORIENTATION_LOCK_LANDSCAPE = 0x0300; // Landscape is 0 on Android, 3 is SCREEN_ORIENTATION_BEHIND in Android
static const int BD_FLAG_SCREEN_ORIENTATION_LOCK_PORTRAIT = 0x0100;
static const int BD_FLAG_SCREEN_ORIENTATION_LOCK_USER = 0x0200; // User preferred rotation
static const int BD_FLAG_SCREEN_ORIENTATION_LOCK_SENSOR = 0x0400; // force usage of sensor
static const int BD_FLAG_SCREEN_ORIENTATION_LOCK_NOSENSOR = 0x0500; // ignore sensor
static const int BD_FLAG_SCREEN_ORIENTATION_LOCK_SENSOR_LANDSCAPE = 0x0600; // both landscapes are allowed
static const int BD_FLAG_SCREEN_ORIENTATION_LOCK_SENSOR_PORTRAIT = 0x0700;
static const int BD_FLAG_SCREEN_ORIENTATION_LOCK_REVERSE_LANDSCAPE = 0x0800;
static const int BD_FLAG_SCREEN_ORIENTATION_LOCK_REVERSE_PORTRAIT = 0x0900;
static const int BD_FLAG_SCREEN_ORIENTATION_LOCK_FULLSENSOR = 0x0A00; // enable also 180 degree rotation by sensor
static const int BD_FLAG_SCREEN_ORIENTATION_LOCK_CURRENT = 0x0E00; // lock to current orientation

/*********************************************
 * Flags for setScreenOrientationLock()
 * We have almost same values as used in Android
 *********************************************/
static const int FLAG_SCREEN_ORIENTATION_LOCK_UNLOCK = 0x00;
static const int FLAG_SCREEN_ORIENTATION_LOCK_LANDSCAPE = 0x03;
static const int FLAG_SCREEN_ORIENTATION_LOCK_PORTRAIT = 0x01;
static const int FLAG_SCREEN_ORIENTATION_LOCK_USER = 0x02;
static const int FLAG_SCREEN_ORIENTATION_LOCK_SENSOR = 0x04;
static const int FLAG_SCREEN_ORIENTATION_LOCK_NOSENSOR = 0x0500; // ignore sensor
static const int FLAG_SCREEN_ORIENTATION_LOCK_SENSOR_LANDSCAPE = 0x06; // both landscapes are allowed
static const int FLAG_SCREEN_ORIENTATION_LOCK_SENSOR_PORTRAIT = 0x07;
static const int FLAG_SCREEN_ORIENTATION_LOCK_REVERSE_LANDSCAPE = 0x08;
static const int FLAG_SCREEN_ORIENTATION_LOCK_REVERSE_PORTRAIT = 0x09;
static const int FLAG_SCREEN_ORIENTATION_LOCK_FULLSENSOR = 0x0A; // enable also 180 degree rotation by sensor
static const int FLAG_SCREEN_ORIENTATION_LOCK_CURRENT = 0x0E;

/**********************
 * Tone
 *********************/
// Android system tones
// Codes start with 0 - 15 for DTMF tones and ends with code TONE_CDMA_SIGNAL_OFF=98 for silent tone (which does not work on lollipop)
#define TONE_CDMA_KEYPAD_VOLUME_KEY_LITE 89
#define TONE_PROP_BEEP_OK TONE_CDMA_KEYPAD_VOLUME_KEY_LITE // 120 ms 941 + 1477 Hz - normal tone for OK Feedback
#define TONE_PROP_BEEP_ERROR 28      // 2* 35/200 ms 400 + 1200 Hz - normal tone for ERROR Feedback
#define TONE_PROP_BEEP_ERROR_HIGH 25 // 2* 100/100 ms 1200 Hz - high tone for ERROR Feedback
#define TONE_PROP_BEEP_ERROR_LONG 26 // 2* 35/200 ms 400 + 1200 Hz - normal tone for ERROR Feedback
#define TONE_SILENCE 50              // Since 98 does not work on Android Lollipop
#define TONE_CDMA_ONE_MIN_BEEP 88
#define TONE_DEFAULT TONE_CDMA_KEYPAD_VOLUME_KEY_LITE
#define TONE_LAST_VALID_TONE_INDEX 98

#define FEEDBACK_TONE_OK 0
#define FEEDBACK_TONE_ERROR 1
#define FEEDBACK_TONE_LONG_ERROR TONE_PROP_BEEP_ERROR_LONG
#define FEEDBACK_TONE_HIGH_ERROR TONE_PROP_BEEP_ERROR_HIGH
#define FEEDBACK_TONE_NO_TONE TONE_SILENCE

/**********************
 * Sensors
 *********************/
// See android.hardware.Sensor
static const int FLAG_SENSOR_TYPE_ACCELEROMETER = 1;
static const int FLAG_SENSOR_TYPE_GYROSCOPE = 4;

// Rate of sensor callbacks - see android.hardware.SensorManager
static const int FLAG_SENSOR_DELAY_NORMAL = 3; // 200 ms
static const int FLAG_SENSOR_DELAY_UI = 2; // 60 ms
static const int FLAG_SENSOR_DELAY_GAME = 1; // 20 ms
static const int FLAG_SENSOR_DELAY_FASTEST = 0;
static const int FLAG_SENSOR_NO_FILTER = 0;
static const int FLAG_SENSOR_SIMPLE_FILTER = 1;

#define BD_SCREEN_BRIGHTNESS_USER   255
#define BD_SCREEN_BRIGHTNESS_MIN      0
#define BD_SCREEN_BRIGHTNESS_MAX    100

// No valid button number
#define NO_BUTTON 0xFF
#define NO_SLIDER 0xFF

struct ThickLine {
    int16_t StartX;
    int16_t StartY;
    int16_t EndX;
    int16_t EndY;
    int16_t Thickness;
    color16_t Color;
    color16_t BackgroundColor;
};

#ifdef __cplusplus
#define MAX_NUMBER_OF_ARGS_FOR_BD_FUNCTIONS 12 // for sending

class BlueDisplay {
public:
    BlueDisplay();
    void resetLocal();
    uint_fast16_t initCommunication(void (*aConnectCallback)(), void (*aRedrawCallback)() = nullptr,
            void (*aReorientationCallback)() = nullptr);
#if defined(ARDUINO)
    void initCommunication(Print *aSerial, void (*aConnectCallback)(), void (*aRedrawCallback)() = nullptr,
            void (*aReorientationCallback)() = nullptr);
#endif
    // The result of initCommunication
    bool isConnectionEstablished();
    void sendSync();
    void setFlagsAndSize(uint16_t aFlags, uint16_t aWidth, uint16_t aHeight);
    void setCodePage(uint16_t aCodePageNumber);
    void setCharacterMapping(uint8_t aChar, uint16_t aUnicodeChar); // aChar must be bigger than 0x80

    void playTone();
    void playTone(uint8_t aToneIndex);
    void playTone(uint8_t aToneIndex, int16_t aToneDuration);
    void playTone(uint8_t aToneIndex, int16_t aToneDuration, uint8_t aToneVolume);
    void playFeedbackTone(uint8_t aFeedbackToneType);

    void speakSetLocale(const char *aLocaleString);
    void speakSetVoice(const char *aVoiceString); // One of the Voice strings printed in log at level Info at BD application startup
    void speakString(const char *aString);
    void speakStringAddToQueue(const char *aString);
    void speakStringBlockingWait(const char *aString, boolean aAddToQueue = false);
    void speakSetLocale(const __FlashStringHelper *aLocaleString);
    void speakSetVoice(const __FlashStringHelper *aVoiceString);
    void speakString(const __FlashStringHelper *aPGMString);
    void speakStringAddToQueue(const __FlashStringHelper *aPGMString);
    void speakStringBlockingWait(const __FlashStringHelper *aPGMString, bool aAddToQueue = false);

    void setLongTouchDownTimeout(uint16_t aLongTouchDownTimeoutMillis);

    void clearDisplay(color16_t aColor = COLOR16_WHITE);
    void clearDisplayOptional(color16_t aColor = COLOR16_WHITE);
    void drawDisplayDirect();
    void setScreenOrientationLock(uint8_t aLockMode);
    void setScreenBrightness(uint8_t aScreenBrightness);

    void setPaintSizeAndColor(uint8_t aPaintIndex, uint16_t aPaintSize, color16_t aPaintColor);

    void drawPixel(uint16_t aXPos, uint16_t aYPos, color16_t aColor);
    void drawCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, color16_t aColor, uint16_t aStrokeWidth = 1);
    void fillCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, color16_t aColor);
    void drawRect(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor, uint16_t aStrokeWidth);
    void drawRectRel(uint16_t aStartX, uint16_t aStartY, int16_t aWidth, int16_t aHeight, color16_t aColor, uint16_t aStrokeWidth);
    void fillRect(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor);
    void fillRectRel(uint16_t aStartX, uint16_t aStartY, int16_t aWidth, int16_t aHeight, color16_t aColor);
    uint16_t drawChar(uint16_t aPositionX, uint16_t aPositionY, char aChar, uint16_t aCharSize, color16_t aCharacterColor,
            color16_t aBackgroundColor);
    void drawText(uint16_t aPositionX, uint16_t aPositionY, const char *aString);
    void drawText(uint16_t aPositionX, uint16_t aPositionY, const __FlashStringHelper *aPGMString);
    uint16_t drawText(uint16_t aPositionX, uint16_t aPositionY, const char *aString, uint16_t aFontSize, color16_t aTextColor,
            color16_t aBackgroundColor);
    uint16_t drawText(uint16_t aPositionX, uint16_t aPositionY, const __FlashStringHelper *aPGMString, uint16_t aFontSize,
            color16_t aTextColor, color16_t aBackgroundColor);
    void clearTextArea(uint16_t aPositionX, uint16_t aPositionY, uint8_t aStringLength, uint16_t aFontSize, color16_t aClearColor);
    void drawMLText(uint16_t aPositionX, uint16_t aPositionY, const char *aString, uint16_t aFontSize, color16_t aTextColor,
            color16_t aBackgroundColor);
    void drawMLText(uint16_t aPositionX, uint16_t aPositionY, const __FlashStringHelper *aPGMString, uint16_t aFontSize,
            color16_t aTextColor, color16_t aBackgroundColor);

    uint16_t drawByte(uint16_t aPositionX, uint16_t aPositionY, int8_t aByte, uint16_t aFontSize = TEXT_SIZE_11,
            color16_t aFGColor =
            COLOR16_BLACK, color16_t aBackgroundColor = COLOR16_WHITE);
    uint16_t drawUnsignedByte(uint16_t aPositionX, uint16_t aPositionY, uint8_t aUnsignedByte, uint16_t aFontSize = TEXT_SIZE_11,
            color16_t aFGColor = COLOR16_BLACK, color16_t aBackgroundColor = COLOR16_WHITE);
    uint16_t drawShort(uint16_t aPositionX, uint16_t aPositionY, int16_t aShort, uint16_t aFontSize = TEXT_SIZE_11,
            color16_t aFGColor =
            COLOR16_BLACK, color16_t aBackgroundColor = COLOR16_WHITE);
    uint16_t drawLong(uint16_t aPositionX, uint16_t aPositionY, int32_t aLong, uint16_t aFontSize = TEXT_SIZE_11,
            color16_t aFGColor =
            COLOR16_BLACK, color16_t aBackgroundColor = COLOR16_WHITE);

    void setWriteStringSizeAndColorAndFlag(uint16_t aPrintSize, color16_t aPrintColor, color16_t aPrintBackgroundColor,
            bool aClearOnNewScreen);
    void setWriteStringPosition(uint16_t aPositionX, uint16_t aPositionY);
    void setWriteStringPositionColumnLine(uint16_t aColumnNumber, uint16_t aLineNumber);
    void writeString(const char *aString);
    void writeString(const __FlashStringHelper *aPGMString);
    void writeString(const char *aString, uint8_t aStringLength);

    void debugMessage(const char *aString);
    void debug(const char *aString);
#if defined(F)
    void debug(const __FlashStringHelper *aPGMString);
#endif
    void debug(uint8_t aByte);
    void debug(const char *aMessage, uint8_t aByte);
    void debug(const char *aMessageStart, uint8_t aByte, const char *aMessageEnd);
    void debug(const char *aMessage, int8_t aByte);
    void debug(int8_t aByte);
    void debug(uint16_t aShort);
    void debug(const char *aMessage, uint16_t aShort);
    void debug(const char *aMessageStart, uint16_t aShort, const char *aMessageEnd);
    void debug(int16_t aShort);
    void debug(const char *aMessage, int16_t aShort);
    void debug(uint32_t aLong);
    void debug(const char *aMessage, uint32_t aLong);
    void debug(const char *aMessageStart, uint32_t aLong, const char *aMessageEnd);
    void debug(int32_t aLong);
    void debug(const char *aMessage, int32_t aLong);
    void debug(float aDouble);
    void debug(const char *aMessage, float aDouble);
    void debug(double aDouble);

    void drawLine(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor);
    void drawLineWithAliasing(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor);
    void drawLineRel(uint16_t aStartX, uint16_t aStartY, int16_t aXDelta, int16_t aYDelta, color16_t aColor);
    void drawLineRelWithAliasing(uint16_t aStartX, uint16_t aStartY, int16_t aXDelta, int16_t aYDelta, color16_t aColor);
    void drawLineFastOneX(uint16_t aStartX, uint16_t aStartY, uint16_t aEndY, color16_t aColor);
    void drawVectorDegree(uint16_t aStartX, uint16_t aStartY, uint16_t aLength, int aDegree, color16_t aColor, int16_t aThickness =
            1);
    void drawVectorDegreeWithAliasing(uint16_t aStartX, uint16_t aStartY, uint16_t aLength, int aDegree, color16_t aColor,
            int16_t aThickness = 1);
    void drawVectorRadian(uint16_t aStartX, uint16_t aStartY, uint16_t aLength, float aRadian, color16_t aColor,
            int16_t aThickness = 1);
    void drawLineWithThickness(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor,
            int16_t aThickness);
    void drawLineWithThicknessWithAliasing(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor,
            int16_t aThickness);
    void drawLineRelWithThickness(uint16_t aStartX, uint16_t aStartY, int16_t aXOffset, int16_t aYOffset, color16_t aColor,
            int16_t aThickness);
    void drawLineRelWithThicknessWithAliasing(uint16_t aStartX, uint16_t aStartY, int16_t aXOffset, int16_t aYOffset,
            color16_t aColor, int16_t aThickness);

    void drawChartByteBuffer(uint16_t aXOffset, uint16_t aYOffset, color16_t aColor, color16_t aClearBeforeColor,
            uint8_t *aByteBuffer, size_t aByteBufferLength);
    void drawChartByteBuffer(uint16_t aXOffset, uint16_t aYOffset, color16_t aColor, color16_t aClearBeforeColor,
            uint8_t aChartIndex, bool aDoDrawDirect, uint8_t *aByteBuffer, size_t aByteBufferLength);
    void drawChartByteBufferScaled(uint16_t aXOffset, uint16_t aYOffset, int16_t aIntegerXScaleFactor, float aYScaleFactor,
            uint8_t aLineSize, uint8_t aChartMode, color16_t aColor, color16_t aClearBeforeColor, uint8_t aChartIndex,
            bool aDoDrawDirect, uint8_t *aByteBuffer, size_t aByteBufferLength);

    // The display size / resolution of the Host (mobile or tablet)
    struct XYSize* getHostDisplaySize();
    uint16_t getHostDisplayWidth();
    uint16_t getHostDisplayHeight();

    // The requested / virtual display size / resolution
    struct XYSize* getRequestedDisplaySize();
    uint16_t getRequestedDisplayWidth();
    uint16_t getRequestedDisplayHeight();
    // Implemented by event handler
    bool isDisplayOrientationLandscape();

    uint32_t getHostUnixTimestamp();
    void setHostUnixTimestamp(uint32_t aHostUnixTimestamp);

    void refreshVector(struct ThickLine *aLine, int16_t aNewRelEndX, int16_t aNewRelEndY);

    void getNumber(void (*aNumberHandler)(float));
    void getNumberWithShortPrompt(void (*aNumberHandler)(float), const char *aShortPromptString);
    void getNumberWithShortPrompt(void (*aNumberHandler)(float), const __FlashStringHelper *aPGMShortPromptString);
    void getNumberWithShortPrompt(void (*aNumberHandler)(float), const char *aShortPromptString, float aInitialValue);
    void getNumberWithShortPrompt(void (*aNumberHandler)(float), const __FlashStringHelper *aPGMShortPromptString,
            float aInitialValue);
    // Not yet implemented
    //    void getText(void (*aTextHandler)(const char *));
    //    void getTextWithShortPrompt(void (*aTextHandler)(const char *), const char *aShortPromptString);
    // This call results in a info callback
    void getInfo(uint8_t aInfoSubcommand, void (*aInfoHandler)(uint8_t, uint8_t, uint16_t, ByteShortLongFloatUnion));
    // This call results in a reorientation callback
    void requestMaxCanvasSize();
    uint_fast16_t requestMaxCanvasSizeBlockingWait(uint_fast16_t aTimeoutMillis);

    void setSensor(uint8_t aSensorType, bool aDoActivate, uint8_t aSensorRate, uint8_t aFilterFlag);

#if defined(__AVR__)
    // On non AVR platforms PGM functions are reduced to plain functions
    uint16_t drawTextPGM(uint16_t aPositionX, uint16_t aPositionY, const char *aPGMString, uint16_t aFontSize, color16_t aTextColor,
            color16_t aBackgroundColor);
    void drawTextPGM(uint16_t aPositionX, uint16_t aPositionY, const char *aPGMString);
    void getNumberWithShortPromptPGM(void (*aNumberHandler)(float), const char *aPGMShortPromptString);
    void getNumberWithShortPromptPGM(void (*aNumberHandler)(float), const char *aPGMShortPromptString, float aInitialValue);
#endif

    // Not yet implemented    void getTextWithShortPromptPGM(void (*aTextHandler)(const char *), const __FlashStringHelper *aPGMShortPromptString);

    struct XYSize mRequestedDisplaySize; // contains requested display size
    struct XYSize mHostDisplaySize; // contains real host display size. Is initialized at connection build up and updated at reorientation and redraw event.
    uint32_t mHostUnixTimestamp;

    bool mBlueDisplayConnectionEstablished; // true if BlueDisplayApps responded to requestMaxCanvasSize()
    bool mOrientationIsLandscape;

    /* For tests */
    void drawGreyscale(uint16_t aXPos, uint16_t tYPos, uint16_t aHeight);
    void drawStar(int aXCenter, int aYCenter, int tOffsetCenter, int tLength, int tOffsetDiagonal, int tLengthDiagonal,
            color16_t aColor, int16_t aThickness);
    void testDisplay(BDButton *aBackButton = nullptr, bool *stillInTestPage = nullptr);
    void generateColorSpectrum();

};

#if defined(F) && defined(ARDUINO)
uint8_t _clipAndCopyPGMString(char *aStringBuffer, const __FlashStringHelper *aPGMString);
#endif

// The instance provided by the class itself
extern BlueDisplay BlueDisplay1;
void clearDisplayAndDisableButtonsAndSliders();
void clearDisplayAndDisableButtonsAndSliders(color16_t aColor);

#endif // __cplusplus

extern bool isLocalDisplayAvailable;

#ifdef __cplusplus
extern "C" {
#endif
// For use in syscalls.c
uint16_t drawTextC(uint16_t aPositionX, uint16_t aPositionY, const char *aString, uint16_t aFontSize, color16_t aTextColor,
        color16_t aBackgroundColor);
void writeStringC(const char *aString, uint8_t aStringLength);
#ifdef __cplusplus
}
#endif

/*
 * Dummy definition of functions defined in ADCUtils to compile examples without errors
 */
uint16_t readADCChannelWithReferenceOversample(uint8_t aChannelNumber, uint8_t aReference, uint8_t aOversampleExponent);
float getVCCVoltage(void);
float getTemperature(void) __attribute__ ((deprecated ("Renamed to getCPUTemperature()"))); // deprecated
float getCPUTemperature(void);

// For convenience also included here
#include "BlueSerial.h"
#include "EventHandler.h"

//#if !defined(_BLUEDISPLAY_HPP) && !defined(SUPPRESS_HPP_WARNING)
//#warning You probably must change the line #include "BlueDisplay.h" to #include "BlueDisplay.hpp" in your ino file or define SUPPRESS_HPP_WARNING before the include to suppress this warning.
//#endif

/*
 *
 * Version 5.0.0
 * - Speak functions added.
 * - Text Y and X position is upper left corner of character.
 * - Added `setCallback()` and `setFlags()` for buttons and sliders.
 * - Modified ManySlidersAndButtons example.
 * - Screen orientation flags now also possible in setFlagsAndSize().
 *
 * Version 4.4.0 - The version compatible with app version 4.4.1
 * - Removed mMaxDisplaySize, because it was just a copy of CurrentDisplaySize, which is now HostDisplaySize etc..
 * - Renamed getDisplaySize to getRequestedDisplaySize etc.
 * - Renamed drawVectorDegrees() to drawVectorDegree().
 * - Added function setScreenBrightness().
 * - Added functions draw*WithAliasing().
 * - Added slider function setMinMaxValue().
 * - Changed Slider callback value from uint16_t to int16_t.
 * - Refactored Chart and chart line drawing functions.
 * - Changed "Caption" to "Text" for buttons and renamed fields and functions.
 * - Added a full screen example for a log chart of CO2 values.
 * - Added the convenience function clearTextArea().
 * - Changed value of COLOR16_NO_DELETE.
 * - Renamed DO_NOT_NEED_TOUCH_AND_SWIPE_EVENTS to DO_NOT_NEED_LONG_TOUCH_DOWN_AND_SWIPE_EVENTS.
 * - BD_FLAG_TOUCH_BASIC_DISABLE is always set if DO_NOT_NEED_BASIC_TOUCH_EVENTS is defined.
 *
 * Version 4.0.1
 * - Minor changes and updated 3. party libs.
 *
 * Version 4.0.0
 * - Major refactoring, many bug fixes and seamless support of local display.
 * - All *Rel*() functions now have signed delta parameters.
 * - Fixed bugs in drawLineRelWithThickness() and Button list for local display.
 * - Added debug(const __FlashStringHelper *aString).
 * - Added bool delay(AndCheckForEvent().
 *
 * Version 3.0.2
 * - Added function setPosition() for sliders.
 * - Fixed bug in macros `BLUE ` and `COLOR32_GET_BLUE`.
 * - Swapped last 2 parameters in `drawLineWithThickness()` and `drawLineRelWithThickness()`.
 *
 * Version 3.0.1
 * - ADCUtils now external sources.
 *
 * Version 3.0.0
 * - Renamed *.cpp to *.hpp.
 *
 * Version 2.2.0
 * - Changed default serial for AVR from `USE_SIMPLE_SERIAL` to standard Arduino Serial.
 * - Added ShowSensorValues example.
 * - Renamed mReferenceDisplaySize to mRequestedDisplaySize and renamed related function to getRequestedDisplaySize().
 * - New function `setBarThresholdDefaultColor`. Requires BlueDisplay app version 4.3.
 * - New function `setPositiveNegativeSliders(..., aValue,  aSliderDeadBand)`.
 * - Renamed setPrintf* functions to setWriteString*.
 * - Switched last 2 parameters in `initCommunication()` and the 3. parameter is now optional.
 * - Compatible with MegaCore supported CPU's.
 *
 * Version 2.1.1
 * - New function `setCaptionFromStringArrayPGM()`.
 * - Added flag `sBDEventJustReceived`.
 *
 * Version 2.1.0
 * - Improved initCommunication and late connection handling.
 * - Arduino Due support added.
 *
 * Version 2.0.0
 * - ESP32 and ESP8266 support added. External BT module required for ESP8266.
 *
 * Version 1.3.0
 * - Added `sMillisOfLastReceivedBDEvent` for user timeout detection.
 * - Fixed bug in `debug(const char *aMessage, float aFloat)`.
 * - Added `*LOCK_SENSOR_LANDSCAPE` and `*LOCK_SENSOR_LANDSCAPE` in function `setScreenOrientationLock()`. Requires BD app version 4.2.
 * - Removed unused `mCurrentDisplayHeight` and `mCurrentDisplayWidth` member variables.
 * - Fixed bug in draw function from `drawByte` to `drawLong`.
 * - Added short `drawText` functions. Requires BD app version 4.2.
 *
 * Version 1.2.0
 * - Use type `Print *` instead of `Stream *`.
 * - New function `initSerial()`
 * - Changed parameter aFontSize to uint16_t also for AVR specific functions.
 *
 * This old version numbers corresponds to the version of the BlueDisplay app
 * Version 3.7
 * - Handling of no input for getNumber.
 * - Slider setScaleFactor() does not scale the current value, mostly delivered as initial value at init().
 * Version 3.6 connect, reconnect and autoconnect improved/added. Improved debug() command. Simplified Red/Green button handling.
 * Version 3.5 Slider scaling changed and unit value added.
 * Version 3.4
 *  - Timeout for data messages. Get number initial value fixed.
 *  - Bug autorepeat button in conjunction with UseUpEventForButtons fixed.
 * Version 3.3
 *  - Fixed silent tone bug for Android Lollipop and other bugs. Multiline text /r /n handling.
 *  - Android time accessible on Arduino. Debug messages as toasts. Changed create button.
 *  - Slider values scalable. GUI multi touch.Hex and ASCII output of received Bluetooth data at log level verbose.
 * Version 3.2 Improved tone and fullscreen handling. Internal refactoring. Bugfixes and minor improvements.
 * Version 3.1 Local display of received and sent commands for debug purposes.
 * Version 3.0 Android sensor accessible by Arduino.
 */

#endif // _BLUEDISPLAY_H
