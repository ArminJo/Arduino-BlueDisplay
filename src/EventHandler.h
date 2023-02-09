/*
 * EventHandler.h
 *
 *
 *  Copyright (C) 2014-2023  Armin Joachimsmeyer
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

#ifndef _EVENTHANDLER_H
#define _EVENTHANDLER_H

#include "Colors.h"

/*
 * If defined, registerDelayCallback() and changeDelayCallback() is used to control a timer for checking for
 * auto repeats, moves and long touch.
 * Otherwise the main loop has to call checkAndHandleTouchPanelEvents() to detect long touch.
 * Move and swipe recognition is currently not implemented in ADS7846.hpp, if USE_TIMER_FOR_PERIODIC_LOCAL_TOUCH_CHECKS is not defined.
 */
//#define USE_TIMER_FOR_PERIODIC_LOCAL_TOUCH_CHECKS // Use registerDelayCallback() and changeDelayCallback() for periodic touch checks
#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
//#define DO_NOT_NEED_BASIC_TOUCH_EVENTS // Disables basic touch events like down, move and up. Saves 620 bytes program memory and 36 bytes RAM
#endif

#if !defined(DISABLE_REMOTE_DISPLAY)
#include "BlueDisplayProtocol.h"
#endif

extern bool sBDEventJustReceived; // is set to true by handleEvent() and can be reset by main loop.
extern unsigned long sMillisOfLastReceivedBDEvent; // is updated with millis() at each received event. Can be used for timeout detection.

extern bool sDisableTouchUpOnce; // set normally by application if long touch action was made
extern bool sDisableMoveEventsUntilTouchUpIsDone; // Skip all touch move and touch up events until touch is released

extern struct BluetoothEvent remoteEvent;
#if defined(AVR)
// Is used for touch down events and stores its position. If remoteEvent is not empty, it is used as buffer for next regular event to avoid overwriting of remoteEvent
extern struct BluetoothEvent remoteTouchDownEvent;
#endif
#if defined(SUPPORT_LOCAL_DISPLAY) && defined(LOCAL_DISPLAY_GENERATES_BD_EVENTS)
extern struct BluetoothEvent localTouchEvent;
#endif

#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
// Callbacks
void registerTouchDownCallback(void (*aTouchDownCallback)(struct TouchEvent *aActualPositionPtr));
void registerTouchMoveCallback(void (*aTouchMoveCallback)(struct TouchEvent *aActualPositionPtr));
// Touch UP callback
void registerTouchUpCallback(void (*aTouchUpCallback)(struct TouchEvent *aActualPositionPtr));
void (* getTouchUpCallback(void))(struct TouchEvent * ); // returns current callback function
void setTouchUpCallbackEnabled(bool aTouchUpCallbackEnabled);
extern bool sTouchUpCallbackEnabled;
#endif

/*
 * Long touch down stuff
 */
#define TOUCH_STANDARD_LONG_TOUCH_TIMEOUT_MILLIS 800 // Millis after which a touch is classified as a long touch
extern void (*sLongTouchDownCallback)(struct TouchEvent*);
extern uint32_t sLongTouchDownTimeoutMillis;

#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
extern struct TouchEvent sCurrentPosition; // for printEventTouchPositionData()
extern bool sTouchIsStillDown;

#  if !defined(ESP32)
// Display X Y touch position on screen
bool isDisplayXYValuesEnabled(void);
void setDisplayXYValuesFlag(bool aEnableDisplay);
void printEventTouchPositionData(int x, int y, color16_t aColor, color16_t aBackColor);
#  endif
#endif

void delayMillisWithCheckAndHandleEvents(unsigned long aDelayMillis);
bool delayMillisAndCheckForEvent(unsigned long aDelayMillis);

void checkAndHandleEvents(void);

void registerLongTouchDownCallback(void (*aLongTouchCallback)(struct TouchEvent*), uint16_t aLongTouchTimeoutMillis);

void registerSwipeEndCallback(void (*aSwipeEndCallback)(struct Swipe*));
void setSwipeEndCallbackEnabled(bool aSwipeEndCallbackEnabled);
extern bool sSwipeEndCallbackEnabled;  // for temporarily disabling swipe callbacks

void registerConnectCallback(void (*aConnectCallback)(void));
void registerReorientationCallback(void (*aReorientationCallback)(void));

/*
 * Connect and reorientation always include a redraw
 */
void registerRedrawCallback(void (*aRedrawCallback)(void));
void (* getRedrawCallback(void))(void);

void registerSensorChangeCallback(uint8_t aSensorType, uint8_t aSensorRate, uint8_t aFilterFlag,
        void (*aSensorChangeCallback)(uint8_t aSensorType, struct SensorCallback *aSensorCallbackInfo));

// defines for backward compatibility
#define registerSimpleConnectCallback(aConnectCallback) registerConnectCallback(aConnectCallback)
#define registerSimpleResizeAndReconnectCallback(aRedrawCallback) registerRedrawCallback(aRedrawCallback)
#define registerSimpleResizeAndConnectCallback registerRedrawCallback(aRedrawCallback)
#define registerSimpleResizeCallback(aRedrawCallback) registerRedrawCallback(aRedrawCallback)
#define getSimpleResizeAndConnectCallback() getRedrawCallback()

#ifdef __cplusplus
extern "C" {
#endif
void handleEvent(struct BluetoothEvent *aEvent); // may be called by plain C function
#ifdef __cplusplus
}
#endif

#endif // _EVENTHANDLER_H
