/*
 * BlueDisplay.h
 *
 *  SUMMARY
 *  Blue Display is an Open Source Android remote Display for Arduino etc.
 *  It receives basic draw requests from Arduino etc. over Bluetooth and renders it.
 *  It also implements basic GUI elements as buttons and sliders.
 *  GUI callback, touch and sensor events are sent back to Arduino.
 *
 *  Copyright (C) 2014  Armin Joachimsmeyer
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

#ifndef BLUEDISPLAY_H_
#define BLUEDISPLAY_H_

#include "Colors.h"

#include "BlueDisplayProtocol.h"
#include "BlueSerial.h"
#include "EventHandler.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef AVR
#include <avr/pgmspace.h>
#endif

#ifdef __cplusplus
#include "BDButton.h" // for BDButtonHandle_t
#include "BDSlider.h" // for BDSliderHandle_t
#endif

#define VERSION_BLUE_DISPLAY "3.7"

/***************************
 * Origin 0.0 is upper left
 **************************/

#define DISPLAY_DEFAULT_HEIGHT 240 // value to use if not connected
#define DISPLAY_DEFAULT_WIDTH 320
#define STRING_BUFFER_STACK_SIZE 22 // Size for buffer allocated on stack with "char tStringBuffer[STRING_BUFFER_STACK_SIZE]" for ...PGM() functions.
#define STRING_BUFFER_STACK_SIZE_FOR_DEBUG_WITH_MESSAGE 34 // Size for buffer allocated on stack with "char tStringBuffer[STRING_BUFFER_STACK_SIZE_FOR_DEBUG]" for debug(const char* aMessage,...) functions.

/*
 * Android Text sizes which are closest to the 8*12 font used locally
 */
#define TEXT_SIZE_11 11
#define TEXT_SIZE_13 13
#define TEXT_SIZE_14 14
#define TEXT_SIZE_16 16
#define TEXT_SIZE_18 18
#define TEXT_SIZE_11 11
// for factor 2 of 8*12 font
#define TEXT_SIZE_22 22
#define TEXT_SIZE_26 26
// for factor 3 of 8*12 font
#define TEXT_SIZE_33 33
// for factor 4 of 8*12 font
#define TEXT_SIZE_44 44
// TextSize * 0.6
#ifdef LOCAL_DISPLAY_EXISTS
// 8/16 instead of 7/13 to be compatible with 8*12 font
#define TEXT_SIZE_11_WIDTH 8
#define TEXT_SIZE_22_WIDTH 16
#else
#define TEXT_SIZE_11_WIDTH 7
#define TEXT_SIZE_13_WIDTH 8
#define TEXT_SIZE_14_WIDTH 8
#define TEXT_SIZE_16_WIDTH 10
#define TEXT_SIZE_18_WIDTH 11
#define TEXT_SIZE_22_WIDTH 13
#endif

// TextSize * 0.93
// 12 instead of 11 to be compatible with 8*12 font and have a margin
#define TEXT_SIZE_11_HEIGHT 12
#define TEXT_SIZE_22_HEIGHT 24

// TextSize * 0.93
// 9 instead of 8 to have ASCEND + DECEND = HEIGHT
#define TEXT_SIZE_11_ASCEND 9
#define TEXT_SIZE_13_ASCEND 10
#define TEXT_SIZE_14_ASCEND 11
#define TEXT_SIZE_16_ASCEND 12
#define TEXT_SIZE_18_ASCEND 14
// 18 instead of 17 to have ASCEND + DECEND = HEIGHT
#define TEXT_SIZE_22_ASCEND 18

// TextSize * 0.24
#define TEXT_SIZE_11_DECEND 3
// 6 instead of 5 to have ASCEND + DECEND = HEIGHT
#define TEXT_SIZE_22_DECEND 6

uint16_t getTextHeight(uint16_t aTextSize);
uint16_t getTextWidth(uint16_t aTextSize);
uint16_t getTextAscend(uint16_t aTextSize);
uint16_t getTextAscendMinusDescend(uint16_t aTextSize);
uint16_t getTextMiddle(uint16_t aTextSize);

/*
 * Layout for 320 x 240 screen size
 */
#define LAYOUT_320_WIDTH 320
#define LAYOUT_240_HEIGHT 240
#define LAYOUT_256_HEIGHT 256

/**********************
 * Constants used in protocol
 *********************/
//#define COLOR_NO_BACKGROUND   ((color16_t)0XFFFE)
static const float NUMBER_INITIAL_VALUE_DO_NOT_SHOW = 1e-40f;

/**********************
 * Basic
 *********************/
// Sub functions for SET_FLAGS_AND_SIZE
// Reset buttons, sliders, sensors, orientation locking, flags (see next lines) and character mappings
static const int BD_FLAG_FIRST_RESET_ALL = 0x01;
// disables also touch moves
static const int BD_FLAG_TOUCH_BASIC_DISABLE = 0x02;
static const int BD_FLAG_ONLY_TOUCH_MOVE_DISABLE = 0x04;
static const int BD_FLAG_LONG_TOUCH_ENABLE = 0x08;
static const int BD_FLAG_USE_MAX_SIZE = 0x10;

/**********************
 * Button
 *********************/
// Flags for BUTTON_GLOBAL_SETTINGS
// old
#define USE_UP_EVENTS_FOR_BUTTONS FLAG_BUTTON_GLOBAL_USE_UP_EVENTS_FOR_BUTTONS
#define BUTTONS_SET_BEEP_TONE FLAG_BUTTON_GLOBAL_SET_BEEP_TONE
// new
static const int FLAG_BUTTON_GLOBAL_USE_DOWN_EVENTS_FOR_BUTTONS = 0x00;
static const int FLAG_BUTTON_GLOBAL_USE_UP_EVENTS_FOR_BUTTONS = 0x01;
static const int FLAG_BUTTON_GLOBAL_SET_BEEP_TONE = 0x02;

// Flags for init
//old
#define BUTTON_FLAG_NO_BEEP_ON_TOUCH FLAG_BUTTON_NO_BEEP_ON_TOUCH
#define BUTTON_FLAG_DO_BEEP_ON_TOUCH FLAG_BUTTON_DO_BEEP_ON_TOUCH
#define BUTTON_FLAG_TYPE_AUTO_RED_GREEN FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN
#define BUTTON_FLAG_TYPE_TOGGLE_RED_GREEN FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN
#define BUTTON_FLAG_TYPE_AUTOREPEAT FLAG_BUTTON_TYPE_AUTOREPEAT

//new
static const int FLAG_BUTTON_NO_BEEP_ON_TOUCH = 0x00;
static const int FLAG_BUTTON_DO_BEEP_ON_TOUCH = 0x01;
static const int FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN = 0x02;
static const int FLAG_BUTTON_TYPE_AUTOREPEAT = 0x04;
static const int FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN_MANUAL_REFRESH = 0x0A; // must be manually drawn after event to show new caption/color

#ifdef USE_BUTTON_POOL
#define INTERNAL_FLAG_MASK 0x80
#define FLAG_IS_ALLOCATED 0x80 // for use with get and releaseButton
#endif

/**********************
 * Slider
 *********************/
// Flags for slider options
static const int FLAG_SLIDER_VERTICAL = 0x00;
static const int FLAG_SLIDER_VERTICAL_SHOW_NOTHING = 0x00;
static const int FLAG_SLIDER_SHOW_BORDER = 0x01;
// if set, ASCII value is printed along with change of bar value
static const int FLAG_SLIDER_SHOW_VALUE = 0x02;
static const int FLAG_SLIDER_IS_HORIZONTAL = 0x04;
static const int FLAG_SLIDER_IS_INVERSE = 0x08;
// if set,  bar (+ ASCII) value will be set by callback handler, not by touch
static const int FLAG_SLIDER_VALUE_BY_CALLBACK = 0x10;
static const int FLAG_SLIDER_IS_ONLY_OUTPUT = 0x20;

// Flags for slider caption position
static const int FLAG_SLIDER_CAPTION_ALIGN_LEFT_BELOW = 0x00;
static const int FLAG_SLIDER_CAPTION_ALIGN_LEFT = 0x00;
static const int FLAG_SLIDER_CAPTION_ALIGN_RIGHT = 0x01;
static const int FLAG_SLIDER_CAPTION_ALIGN_MIDDLE = 0x02;
static const int FLAG_SLIDER_CAPTION_BELOW = 0x00;
static const int FLAG_SLIDER_CAPTION_ABOVE = 0x04;

/**********************
 * Tone
 *********************/
// Android system tones
// codes start with 0 - 15 for DTMF tones and ends with code TONE_CDMA_SIGNAL_OFF=98 for silent tone (which does not work on lollipop)
#define TONE_CDMA_KEYPAD_VOLUME_KEY_LITE 89
#define TONE_PROP_BEEP_OK TONE_CDMA_KEYPAD_VOLUME_KEY_LITE // 120 ms 941 + 1477Hz - normal tone for OK Feedback
#define TONE_PROP_BEEP_ERROR 28 // 2* 35/200 ms 400 + 1200Hz - normal tone for ERROR Feedback
#define TONE_PROP_BEEP_ERROR_HIGH 25 // 2* 100/100 ms 1200Hz - high tone for ERROR Feedback
#define TONE_PROP_BEEP_ERROR_LONG 26 // 2* 35/200 ms 400 + 1200Hz - normal tone for ERROR Feedback
#define TONE_SILENCE 50 // since 98 does not work on lollipop
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
// see android.hardware.Sensor
static const int FLAG_SENSOR_TYPE_ACCELEROMETER = 1;
static const int FLAG_SENSOR_TYPE_GYROSCOPE = 4;

// rate of sensor callbacks - see android.hardware.SensorManager
static const int FLAG_SENSOR_DELAY_NORMAL = 3; // 200 ms
static const int FLAG_SENSOR_DELAY_UI = 2; // 60 ms
static const int FLAG_SENSOR_DELAY_GAME = 1; // 20 ms
static const int FLAG_SENSOR_DELAY_FASTEST = 0;
static const int FLAG_SENSOR_NO_FILTER = 0;
static const int FLAG_SENSOR_SIMPLE_FILTER = 1;

// Flags for SET_SCREEN_ORIENTATION_LOCK
static const int FLAG_SCREEN_ORIENTATION_LOCK_LANDSCAPE = 0x00;
static const int FLAG_SCREEN_ORIENTATION_LOCK_PORTRAIT = 0x01;
static const int FLAG_SCREEN_ORIENTATION_LOCK_ACTUAL = 0x02;
static const int FLAG_SCREEN_ORIENTATION_LOCK_UNLOCK = 0x03;

// no valid button number
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
    void resetLocal(void);
    void initCommunication(void (*aConnectCallback)(void), void (*aReorientationCallback)(void), void (*aRedrawCallback)(void));
    // with combined callbacks
    void initCommunication(void (*aConnectAndReorientationCallback)(void), void (*aRedrawCallback)(void));
    // The result of initCommunication
    bool isConnectionEstablished();
    void sendSync(void);
    void setFlagsAndSize(uint16_t aFlags, uint16_t aWidth, uint16_t aHeight);
    void setCodePage(uint16_t aCodePageNumber);
    void setCharacterMapping(uint8_t aChar, uint16_t aUnicodeChar); // aChar must be bigger than 0x80

    void playTone(void);
    void playTone(uint8_t aToneIndex);
    void playTone(uint8_t aToneIndex, int16_t aToneDuration);
    void playTone(uint8_t aToneIndex, int16_t aToneDuration, uint8_t aToneVolume);
    void playFeedbackTone(uint8_t isError);
    void setLongTouchDownTimeout(uint16_t aLongTouchDownTimeoutMillis);

    void clearDisplay(color16_t aColor);
    void drawDisplayDirect(void);
    void setScreenOrientationLock(uint8_t aLockMode);

    void drawPixel(uint16_t aXPos, uint16_t aYPos, color16_t aColor);
    void drawCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, color16_t aColor, uint16_t aStrokeWidth);
    void fillCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, color16_t aColor);
    void drawRect(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, color16_t aColor, uint16_t aStrokeWidth);
    void drawRectRel(uint16_t aXStart, uint16_t aYStart, uint16_t aWidth, uint16_t aHeight, color16_t aColor,
            uint16_t aStrokeWidth);
    void fillRect(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, color16_t aColor);
    void fillRectRel(uint16_t aXStart, uint16_t aYStart, uint16_t aWidth, uint16_t aHeight, color16_t aColor);
    uint16_t drawChar(uint16_t aPosX, uint16_t aPosY, char aChar, uint16_t aCharSize, color16_t aFGColor, color16_t aBGColor);
    uint16_t drawText(uint16_t aXStart, uint16_t aYStart, const char *aStringPtr, uint16_t aFontSize, color16_t aFGColor,
            color16_t aBGColor);

    uint16_t drawByte(uint16_t aPosX, uint16_t aPosY, int8_t aByte, uint16_t aTextSize = TEXT_SIZE_11, color16_t aFGColor =
    COLOR_BLACK, color16_t aBGColor = COLOR_WHITE);
    uint16_t drawUnsignedByte(uint16_t aPosX, uint16_t aPosY, uint8_t aUnsignedByte, uint16_t aTextSize = TEXT_SIZE_11,
            color16_t aFGColor = COLOR_BLACK, color16_t aBGColor = COLOR_WHITE);
    uint16_t drawShort(uint16_t aPosX, uint16_t aPosY, int16_t aShort, uint16_t aTextSize = TEXT_SIZE_11, color16_t aFGColor =
    COLOR_BLACK, color16_t aBGColor = COLOR_WHITE);
    uint16_t drawLong(uint16_t aPosX, uint16_t aPosY, int32_t aLong, uint16_t aTextSize = TEXT_SIZE_11, color16_t aFGColor =
    COLOR_BLACK, color16_t aBGColor = COLOR_WHITE);

    void setPrintfSizeAndColorAndFlag(uint16_t aPrintSize, color16_t aPrintColor, color16_t aPrintBackgroundColor,
    bool aClearOnNewScreen);
    void setPrintfPosition(uint16_t aPosX, uint16_t aPosY);
    void setPrintfPositionColumnLine(uint16_t aColumnNumber, uint16_t aLineNumber);
    void writeString(const char *aStringPtr, uint8_t aStringLength);

    void debugMessage(const char *aStringPtr);
    void debug(const char *aStringPtr);
    void debug(uint8_t aByte);
    void debug(const char* aMessage, uint8_t aByte);
    void debug(const char* aMessage, int8_t aByte);
    void debug(int8_t aByte);
    void debug(uint16_t aShort);
    void debug(const char* aMessage, uint16_t aShort);
    void debug(int aShort);
    void debug(const char* aMessage, int aShort);
    void debug(uint32_t aLong);
    void debug(const char* aMessage, uint32_t aLong);
    void debug(int32_t aLong);
    void debug(const char* aMessage, int32_t aLong);
    void debug(float aDouble);
    void debug(const char* aMessage, float aDouble);
    void debug(double aDouble);

    void drawLine(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, color16_t aColor);
    void drawLineRel(uint16_t aXStart, uint16_t aYStart, uint16_t aXDelta, uint16_t aYDelta, color16_t aColor);
    void drawLineFastOneX(uint16_t x0, uint16_t y0, uint16_t y1, color16_t aColor);
    void drawVectorDegrees(uint16_t aXStart, uint16_t aYStart, uint16_t aLength, int aDegrees, color16_t aColor,
            int16_t aThickness = 1);
    void drawVectorRadian(uint16_t aXStart, uint16_t aYStart, uint16_t aLength, float aRadian, color16_t aColor,
            int16_t aThickness = 1);
    void drawLineWithThickness(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, int16_t aThickness,
            color16_t aColor);
    void drawLineRelWithThickness(uint16_t aXStart, uint16_t aYStart, uint16_t aXDelta, uint16_t aYDelta, int16_t aThickness,
            color16_t aColor);

    void drawChartByteBuffer(uint16_t aXOffset, uint16_t aYOffset, color16_t aColor, color16_t aClearBeforeColor,
            uint8_t *aByteBuffer, size_t aByteBufferLength);
    void drawChartByteBuffer(uint16_t aXOffset, uint16_t aYOffset, color16_t aColor, color16_t aClearBeforeColor,
            uint8_t aChartIndex,
            bool aDoDrawDirect, uint8_t *aByteBuffer, size_t aByteBufferLength);

    struct XYSize * getMaxDisplaySize(void);
    uint16_t getMaxDisplayWidth(void);
    uint16_t getMaxDisplayHeight(void);
    struct XYSize * getActualDisplaySize(void);
    uint16_t getActualDisplayWidth(void);
    uint16_t getActualDisplayHeight(void);
    // returns requested size
    struct XYSize * getReferenceDisplaySize(void);
    uint16_t getDisplayWidth(void);
    uint16_t getDisplayHeight(void);
    // implemented by event handler
    bool isDisplayOrientationLandscape(void);

    void refreshVector(struct ThickLine * aLine, int16_t aNewRelEndX, int16_t aNewRelEndY);

    void getNumber(void (*aNumberHandler)(float));
    void getNumberWithShortPrompt(void (*aNumberHandler)(float), const char *aShortPromptString);
    void getNumberWithShortPrompt(void (*aNumberHandler)(float), const char *aShortPromptString, float aInitialValue);
    // Not yet implemented
    //    void getText(void (*aTextHandler)(const char *));
    //    void getTextWithShortPrompt(void (*aTextHandler)(const char *), const char *aShortPromptString);
    // results in a info callback
    void getInfo(uint8_t aInfoSubcommand, void (*aInfoHandler)(uint8_t, uint8_t, uint16_t, ByteShortLongFloatUnion));
    // results in a reorientation callback
    void requestMaxCanvasSize(void);

    void setSensor(uint8_t aSensorType, bool aDoActivate, uint8_t aSensorRate, uint8_t aFilterFlag);

#ifdef LOCAL_DISPLAY_EXISTS
    void drawMLText(uint16_t aPosX, uint16_t aPosY, const char *aStringPtr, uint16_t aTextSize, color16_t aFGColor, color16_t aBGColor);
#endif

#ifdef AVR
    uint16_t drawTextPGM(uint16_t aXStart, uint16_t aYStart, const char * aPGMString, uint8_t aFontSize, color16_t aFGColor,
            color16_t aBGColor);
    void getNumberWithShortPromptPGM(void (*aNumberHandler)(float), const char *aPGMShortPromptString);
    void getNumberWithShortPromptPGM(void (*aNumberHandler)(float), const char *aPGMShortPromptString, float aInitialValue);
    // Not yet implemented    void getTextWithShortPromptPGM(void (*aTextHandler)(const char *), const char *aPGMShortPromptString);

    void printVCCAndTemperaturePeriodically(uint16_t aXPos, uint16_t aYPos, uint8_t aTextSize, uint16_t aPeriodMillis);
#endif
    /*
     * Button stuff
     */
    BDButtonHandle_t createButton(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY,
            color16_t aButtonColor, const char * aCaption, uint8_t aCaptionSize, uint8_t aFlags, int16_t aValue,
            void (*aOnTouchHandler)(BDButton *, int16_t));
    void drawButton(BDButtonHandle_t aButtonNumber);
    void removeButton(BDButtonHandle_t aButtonNumber, color16_t aBackgroundColor);
    void drawButtonCaption(BDButtonHandle_t aButtonNumber);
    void setButtonCaption(BDButtonHandle_t aButtonNumber, const char * aCaption, bool doDrawButton);
    void setButtonValue(BDButtonHandle_t aButtonNumber, int16_t aValue);
    void setButtonValueAndDraw(BDButtonHandle_t aButtonNumber, int16_t aValue);
    void setButtonColor(BDButtonHandle_t aButtonNumber, color16_t aButtonColor);
    void setButtonColorAndDraw(BDButtonHandle_t aButtonNumber, color16_t aButtonColor);
    void setButtonPosition(BDButtonHandle_t aButtonNumber, int16_t aPositionX, int16_t aPositionY);
    void setButtonAutorepeatTiming(BDButtonHandle_t aButtonNumber, uint16_t aMillisFirstDelay, uint16_t aMillisFirstRate,
            uint16_t aFirstCount, uint16_t aMillisSecondRate);

    void activateButton(BDButtonHandle_t aButtonNumber);
    void deactivateButton(BDButtonHandle_t aButtonNumber);
    void activateAllButtons(void);
    void deactivateAllButtons(void);
    void setButtonsGlobalFlags(uint16_t aFlags);
    void setButtonsTouchTone(uint8_t aToneIndex, uint8_t aToneVolume);

#ifdef AVR
    BDButtonHandle_t createButtonPGM(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY,
            color16_t aButtonColor, const char * aPGMCaption, uint8_t aCaptionSize, uint8_t aFlags, int16_t aValue,
            void (*aOnTouchHandler)(BDButton *, int16_t));
    void setButtonCaptionPGM(BDButtonHandle_t aButtonNumber, const char * aPGMCaption, bool doDrawButton);
#endif

    /*
     * Slider stuff
     */
    BDSliderHandle_t createSlider(uint16_t aPositionX, uint16_t aPositionY, uint8_t aBarWidth, int16_t aBarLength,
            int16_t aThresholdValue, int16_t aInitalValue, color16_t aSliderColor, color16_t aBarColor, uint8_t aFlags,
            void (*aOnChangeHandler)(BDSliderHandle_t *, int16_t));
    void drawSlider(BDSliderHandle_t aSliderNumber);
    void drawSliderBorder(BDSliderHandle_t aSliderNumber);
    void setSliderActualValueAndDrawBar(BDSliderHandle_t aSliderNumber, int16_t aActualValue);
    void setSliderColorBarThreshold(BDSliderHandle_t aSliderNumber, uint16_t aBarThresholdColor);
    void setSliderColorBarBackground(BDSliderHandle_t aSliderNumber, uint16_t aBarBackgroundColor);

    void setSliderCaptionProperties(BDSliderHandle_t aSliderNumber, uint8_t aCaptionSize, uint8_t aCaptionPosition,
            uint8_t aCaptionMargin, color16_t aCaptionColor, color16_t aCaptionBackgroundColor);
    void setSliderCaption(BDSliderHandle_t aSliderNumber, const char * aCaption);

    void activateSlider(BDSliderHandle_t aSliderNumber);
    void deactivateSlider(BDSliderHandle_t aSliderNumber);
    void activateAllSliders(void);
    void deactivateAllSliders(void);

    struct XYSize mReferenceDisplaySize; // contains requested display size
    struct XYSize mActualDisplaySize;
    struct XYSize mMaxDisplaySize;
    uint32_t mHostUnixTimestamp;

    volatile bool mConnectionEstablished;
    volatile bool mOrientationIsLandscape;

    /* for tests */
    void drawGreyscale(uint16_t aXPos, uint16_t tYPos, uint16_t aHeight);
    void drawStar(int aXPos, int aYPos, int tOffsetCenter, int tLength, int tOffsetDiagonal, int tLengthDiagonal, color16_t aColor);
    void testDisplay(void);
    void generateColorSpectrum(void);

private:
    uint16_t mActualDisplayHeight;
    uint16_t mActualDisplayWidth;
};

// The instance provided by the class itself
extern BlueDisplay BlueDisplay1;

void clearDisplayAndDisableButtonsAndSliders(color16_t aColor);

#ifdef LOCAL_DISPLAY_EXISTS
#include <MI0283QT2.h>

/*
 * MI0283QT2 TFTDisplay - must provided by main program
 * external declaration saves ROM (210 Bytes) and RAM ( 20 Bytes)
 * and avoids missing initialization :-)
 */
extern MI0283QT2 LocalDisplay;
// to be provided by local display library
extern const unsigned int LOCAL_DISPLAY_HEIGHT;
extern const unsigned int LOCAL_DISPLAY_WIDTH;
#endif
// to be provided by another source (main.cpp)
extern const unsigned int REMOTE_DISPLAY_HEIGHT;
extern const unsigned int REMOTE_DISPLAY_WIDTH;

#endif // __cplusplus

extern bool isLocalDisplayAvailable;

#ifdef __cplusplus
extern "C" {
#endif
// for use in syscalls.c
uint16_t drawTextC(uint16_t aXStart, uint16_t aYStart, const char *aStringPtr, uint16_t aFontSize, color16_t aFGColor,
        color16_t aBGColor);
void writeStringC(const char *aStringPtr, uint8_t aStringLength);
#ifdef __cplusplus
}
#endif

/*
 * Utilities used also internal
 */
#ifdef AVR
uint16_t getADCValue(uint8_t aChannel, uint8_t aReference);
float getVCCValue(void);
float getVCCVoltage(void);
float getTemperature(void);
#endif

// for convenience also included here
#include "BlueSerial.h"
#include "EventHandler.h"

#endif /* BLUEDISPLAY_H_ */

