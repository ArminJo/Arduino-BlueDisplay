/*
 * EventHandler.hpp
 *
 * Implements the methods to receive events from the Android BlueDisplay app.
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

#ifndef _EVENTHANDLER_HPP
#define _EVENTHANDLER_HPP

#include "EventHandler.h"
#include "BlueDisplay.h"

#if !defined(ARDUINO)
#include "timing.h" // for getMillisSinceBoot()
#  if defined(USE_STM32F3_DISCO)
#  include "stm32f3_discovery.h"  // For LEDx
#  endif
#include "stm32fx0xPeripherals.h" // For Watchdog_reload()
#include <stdio.h> // for printf
#endif // ! ARDUINO

#include <stdlib.h> // for abs()

bool sBDEventJustReceived = false;
unsigned long sMillisOfLastReceivedBDEvent;

struct TouchEvent sDownPosition;
struct TouchEvent sCurrentPosition;
struct TouchEvent sUpPosition;

#if defined(SUPPORT_LOCAL_DISPLAY)
/*
 * helper variables
 */
//
bool sNothingTouched = false; // = !(sSliderTouched || sAutorepeatButtonTouched)
bool sSliderIsMoveTarget = false; // true if slider was touched by DOWN event

uint32_t sLongTouchDownTimeoutMillis;
uint32_t sPeriodicCallbackPeriodMillis;

#if defined(AUTOREPEAT_BY_USING_LOCAL_EVENT)
/*
 * timer related callbacks
 */
//
void (*sPeriodicTouchCallback)(int, int) = NULL; // return parameter not yet used
struct BluetoothEvent localTouchEvent;
#else
uint32_t sLastMillisOfLastCallOfPeriodicTouchCallback;
#endif

#endif

bool sTouchIsStillDown = false; // To enable simple blocking touch down and touch up detection (without using a touch down or up callback).
bool sDisableTouchUpOnce = false; // Disable next touch up detection. E.g. because we are already in a touch handler and don't want the end of this touch to be interpreted for a newly displayed button.
bool sDisableMoveEventsUntilTouchUpIsDone = false; // To suppress move events after button press to avoid interpreting it for a slider. Useful, if page changed and a slider is presented on the position where the page switch button was.

struct BluetoothEvent remoteEvent; // To hold the current received event
#if defined(USE_SIMPLE_SERIAL)
// Is used for touch down events. If remoteEvent is not empty, it is used as buffer for next regular event to avoid overwriting of remoteEvent
struct BluetoothEvent remoteTouchDownEvent;
#endif

void (*sTouchDownCallback)(struct TouchEvent*) = NULL;
void (*sLongTouchDownCallback)(struct TouchEvent*) = NULL;
void (*sTouchMoveCallback)(struct TouchEvent*) = NULL;

void (*sTouchUpCallback)(struct TouchEvent*) = NULL;
bool sTouchUpCallbackEnabled = false;

void (*sSwipeEndCallback)(struct Swipe*) = NULL;
bool sSwipeEndCallbackEnabled = false;

void (*sConnectCallback)() = NULL;
void (*sRedrawCallback)() = NULL;
void (*sReorientationCallback)() = NULL;

void (*sSensorChangeCallback)(uint8_t aEventType, struct SensorCallback *aSensorCallbackInfo) = NULL;

void copyDisplaySizeAndTimestamp(struct BluetoothEvent *aEvent);

/*
 * Is also called on Connect and Reorientation events
 */
void registerRedrawCallback(void (*aRedrawCallback)()) {
    sRedrawCallback = aRedrawCallback;
}

/**
 * Register a callback routine which is called when touch goes up
 */
void registerTouchUpCallback(void (*aTouchUpCallback)(struct TouchEvent *aCurrentPositionPtr)) {
    sTouchUpCallback = aTouchUpCallback;
    // disable next end touch since we are already in a touch handler and don't want the end of this touch to be interpreted
    if (sTouchIsStillDown) {
        sDisableTouchUpOnce = true;
    }
    sTouchUpCallbackEnabled = (aTouchUpCallback != NULL);
}

// !!! Must be without comment and closed by @formatter:on
// @formatter:off
void (* getRedrawCallback())() {
    return sRedrawCallback;
}

/**
 * return pointer to end touch callback function
 */
void (* getTouchUpCallback())(struct TouchEvent * ) {
    return sTouchUpCallback;
}
// @formatter:on

/*
 * Connect event also calls redraw event
 */
void registerConnectCallback(void (*aConnectCallback)()) {
    sConnectCallback = aConnectCallback;
}

/*
 * Reorientation event also calls redraw event
 */
void registerReorientationCallback(void (*aReorientationCallback)()) {
    sReorientationCallback = aReorientationCallback;
}

#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
void registerTouchDownCallback(void (*aTouchDownCallback)(struct TouchEvent *aCurrentPositionPtr)) {
    sTouchDownCallback = aTouchDownCallback;
}

void registerTouchMoveCallback(void (*aTouchMoveCallback)(struct TouchEvent *aCurrentPositionPtr)) {
    sTouchMoveCallback = aTouchMoveCallback;
}

/**
 * disable or enable touch up callback
 * used by numberpad
 * @param aTouchUpCallbackEnabled
 */
void setTouchUpCallbackEnabled(bool aTouchUpCallbackEnabled) {
    if (aTouchUpCallbackEnabled && sTouchUpCallback != NULL) {
        sTouchUpCallbackEnabled = true;
    } else {
        sTouchUpCallbackEnabled = false;
    }
}
#endif

/**
 * Register a callback routine which is only called after a timeout if screen is still touched
 * Send only timeout value to BD Host
 */
void registerLongTouchDownCallback(void (*aLongTouchDownCallback)(struct TouchEvent*), uint16_t aLongTouchDownTimeoutMillis) {
    sLongTouchDownCallback = aLongTouchDownCallback;
#if defined(SUPPORT_LOCAL_DISPLAY) && !defined(ARDUINO)
    sLongTouchDownTimeoutMillis = aLongTouchDownTimeoutMillis;
    if (aLongTouchDownCallback == NULL) {
        changeDelayCallback(&callbackLongTouchDownTimeout, DISABLE_TIMER_DELAY_VALUE); // housekeeping - disable timeout
    }
#endif
    BlueDisplay1.setLongTouchDownTimeout(aLongTouchDownTimeoutMillis);
}

/**
 * Register a callback routine which is called when touch goes up and swipe detected
 */
void registerSwipeEndCallback(void (*aSwipeEndCallback)(struct Swipe*)) {
    sSwipeEndCallback = aSwipeEndCallback;
    // Disable next end touch since we are already in a touch handler and don't want the end of this touch to be interpreted
    if (sTouchIsStillDown) {
        sDisableTouchUpOnce = true;
    }
    sSwipeEndCallbackEnabled = (aSwipeEndCallback != NULL);
}

void setSwipeEndCallbackEnabled(bool aSwipeEndCallbackEnabled) {
    if (aSwipeEndCallbackEnabled && sSwipeEndCallback != NULL) {
        sSwipeEndCallbackEnabled = true;
    } else {
        sSwipeEndCallbackEnabled = false;
    }
}

/**
 * Values received from accelerator sensor are in g (m/(s*s))
 * @param aSensorType see see android.hardware.Sensor. FLAG_SENSOR_TYPE_ACCELEROMETER, FLAG_SENSOR_TYPE_GYROSCOPE (in BlueDisplay.h)
 * @param aSensorRate see android.hardware.SensorManager (0-3) one of  {@link #FLAG_SENSOR_DELAY_NORMAL} 200 ms, {@link #FLAG_SENSOR_DELAY_UI} 60 ms,
 *        {@link #FLAG_SENSOR_DELAY_GAME} 20ms, or {@link #FLAG_SENSOR_DELAY_FASTEST}
 *        If aSensorRate is > FLAG_SENSOR_DELAY_NORMAL (3) the value is interpreted (by BD app) as milliseconds interval (down do 5 ms).
 * @param aFilterFlag If FLAG_SENSOR_SIMPLE_FILTER, then sensor values are sent via BT only if value changed.
 * To avoid noise (event value is solely switching between 2 values), values are skipped too if they are equal last or second last value.
 * @param aSensorChangeCallback one callback for all sensors types
 */
void registerSensorChangeCallback(uint8_t aSensorType, uint8_t aSensorRate, uint8_t aFilterFlag,
        void (*aSensorChangeCallback)(uint8_t aSensorType, struct SensorCallback *aSensorCallbackInfo)) {
    bool tSensorEnable = true;
    if (aSensorChangeCallback == NULL) {
        tSensorEnable = false;
    }
    BlueDisplay1.setSensor(aSensorType, tSensorEnable, aSensorRate, aFilterFlag);
    sSensorChangeCallback = aSensorChangeCallback;
}

/*
 * Delay, which also checks for events
 * AVR - Is not affected by overflow of millis()!
 */
void delayMillisWithCheckAndHandleEvents(unsigned long aDelayMillis) {
#if defined(ARDUINO)
    unsigned long tStartMillis = millis();
    while (millis() - tStartMillis < aDelayMillis) {
#  if !defined(USE_SIMPLE_SERIAL) && defined(__AVR__)
        // check for Arduino serial - copied code from arduino main.cpp / main()
        if (serialEventRun) {
            serialEventRun(); // this in turn calls serialEvent from BlueSerial.cpp
        }
#  endif
#else // ARDUINO
    unsigned long tStartMillis = getMillisSinceBoot();
    while (getMillisSinceBoot() - tStartMillis < aDelayMillis) {
#endif
        checkAndHandleEvents();
#if defined(ESP8266)
        yield(); // required for ESP8266
#endif
    }
}

/*
 * Special delay function for BlueDisplay. Returns prematurely if Event is received.
 * To be used in blocking functions as delay
 * @return  true - as soon as event received
 */
bool delayMillisAndCheckForEvent(unsigned long aDelayMillis) {
    sBDEventJustReceived = false;
#if defined(ARDUINO)
    unsigned long tStartMillis = millis();
    while (millis() - tStartMillis < aDelayMillis) {
#  if !defined(USE_SIMPLE_SERIAL) && defined(__AVR__)
        // check for Arduino serial - copied code from arduino main.cpp / main()
        if (serialEventRun) {
            serialEventRun(); // this in turn calls serialEvent from BlueSerial.cpp
        }
#  endif
#else // ARDUINO
    unsigned long tStartMillis = getMillisSinceBoot();
    while (getMillisSinceBoot() - tStartMillis < aDelayMillis) {
#endif
        checkAndHandleEvents();
        if (sBDEventJustReceived) {
            return true;
        }
#if defined(ESP8266)
            yield(); // required for ESP8266
#endif
    }
    return false;
}

#if defined(SUPPORT_LOCAL_DISPLAY)
bool sDisplayXYValuesEnabled = false;  // displays touch values on screen

#if defined(AUTOREPEAT_BY_USING_LOCAL_EVENT)
/**
 * set CallbackPeriod
 */
void setPeriodicTouchCallbackPeriod(uint32_t aCallbackPeriod) {
    sPeriodicCallbackPeriodMillis = aCallbackPeriod;
}

/**
 * Callback routine for SysTick handler
 */
void callbackPeriodicTouch() {
    if (sTouchIsStillDown) {
        if (sPeriodicTouchCallback != NULL) {
            // do "normal" callback for autorepeat buttons
            sPeriodicTouchCallback(sCurrentPosition.TouchPosition.PosX, sCurrentPosition.TouchPosition.PosY);
        }
        if (sTouchIsStillDown) {
            // renew systic callback request
            registerDelayCallback(&callbackPeriodicTouch, sPeriodicCallbackPeriodMillis);
        }
    }
}

/**
 * Register a callback routine which is called every CallbackPeriod milliseconds while screen is touched
 */
void registerPeriodicTouchCallback(void (*aPeriodicTouchCallback)(int, int), uint32_t aCallbackPeriodMillis) {
    sPeriodicTouchCallback = aPeriodicTouchCallback;
    sPeriodicCallbackPeriodMillis = aCallbackPeriodMillis;
#if defined(AVR)
    sLastMillisOfLastCallOfPeriodicTouchCallback = millis();
#  else
    changeDelayCallback(&callbackPeriodicTouch, aCallbackPeriodMillis);
#  endif
}

/**
 * Callback routine for SysTick handler
 * Creates event if no Slider was touched and no swipe gesture was started
 * Disabling of touch up handling  (sDisableTouchUpOnce = false) must be done by called handler!!!
 */
void callbackLongTouchDownTimeout() {
    assert_param(sLongTouchDownCallback != NULL);
// No long touch if swipe is made or slider touched
    if (!sSliderIsMoveTarget) {
        /*
         * Check if a swipe is intended (position has moved over threshold).
         * If not, call long touch callback
         */
        if (abs(sDownPosition.TouchPosition.PosX - sCurrentPosition.TouchPosition.PosX) < TOUCH_SWIPE_THRESHOLD
                && abs(sDownPosition.TouchPosition.PosY - sCurrentPosition.TouchPosition.PosY) < TOUCH_SWIPE_THRESHOLD) {
            // fill up event
            localTouchEvent.EventData.TouchEventInfo.TouchPosition = TouchPanel.mTouchLastPosition;
            localTouchEvent.EventType = EVENT_LONG_TOUCH_DOWN_CALLBACK;
        }
    }
}
#  endif
#endif

/**
 * Is called by main loop
 */
void checkAndHandleEvents() {
#if defined(HAL_WWDG_MODULE_ENABLED)
    Watchdog_reload();
#endif

#if defined(SUPPORT_LOCAL_DISPLAY)
    resetTouchFlags();
    /*
     * Check if a local event happened, i.e. the localTouchEvent was written by an touch device interrupt handler
     */
    if (localTouchEvent.EventType != EVENT_NO_EVENT) {
        handleEvent(&localTouchEvent);
    }
#endif

#if !defined(DISABLE_REMOTE_DISPLAY)
#  if defined(ARDUINO)
#    if defined(USE_SIMPLE_SERIAL)
    handleEvent(&remoteTouchDownEvent);
    handleEvent(&remoteEvent);
#    else
    // get Arduino Serial data
    serialEvent(); // calls in turn handleEvent(&remoteEvent);
#    endif
#  else
    /*
     * For non Arduino, check USART buffer, which in turn calls handleEvent() if event was received
     */
    checkAndHandleMessageReceived();
#  endif
#endif
}

/**
 * Interprets the event type and manage the callbacks and flags
 * It is indirectly called by thread in main loop
 */
extern "C" void handleEvent(struct BluetoothEvent *aEvent) {
    // First check if we really have an event here
    if (aEvent->EventType == EVENT_NO_EVENT) {
        return;
    }
#if defined(ESP32) && defined DEBUG
        Serial.print("EventType=0x");
        Serial.println(aEvent->EventType, HEX);
#endif
    uint8_t tEventType = aEvent->EventType;

    // local copy of event since the values in the original event may be overwritten if the handler requires long time for its action
    struct BluetoothEvent tEvent = *aEvent;
    // assignment has the same code size as:
//    struct BluetoothEvent tEvent;
//    memcpy(&tEvent, aEvent, sizeof(BluetoothEvent));

    // avoid using event twice
    aEvent->EventType = EVENT_NO_EVENT;

#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS) && defined(SUPPORT_LOCAL_DISPLAY)
    if (tEventType <= EVENT_TOUCH_ACTION_MOVE && sDisplayXYValuesEnabled) {
        printTPData(30, 2 + TEXT_SIZE_11_ASCEND, COLOR16_BLACK, COLOR16_WHITE);
    }
#endif

    void (*tInfoCallback)(uint8_t, uint8_t, uint16_t, ByteShortLongFloatUnion);
    void (*tNumberCallback)(float);
    void (*tSliderCallback)(BDSliderHandle_t*, int16_t);
    void (*tButtonCallback)(BDButtonHandle_t*, int16_t);

    switch (tEventType) { // switch requires 36 bytes more code but is clearer to understand :-(

#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
    case EVENT_TOUCH_ACTION_DOWN:
//    if (tEventType == EVENT_TOUCH_ACTION_DOWN) {
        // must initialize all positions here!
        sDownPosition = tEvent.EventData.TouchEventInfo;
        sCurrentPosition = tEvent.EventData.TouchEventInfo;
#if defined(USE_STM32F3_DISCO)
        BSP_LED_On(LED_BLUE_2); // BLUE Front
#endif
        sTouchIsStillDown = true;
#if defined(SUPPORT_LOCAL_DISPLAY) && !defined(ARDUINO)
        // start timeout for long touch if it is local event
        if (sLongTouchDownCallback != NULL && aEvent != &remoteEvent) {
            changeDelayCallback(&callbackLongTouchDownTimeout, sLongTouchDownTimeoutMillis); // enable timeout
        }
#endif
        if (sTouchDownCallback != NULL) {
            sTouchDownCallback(&tEvent.EventData.TouchEventInfo);
        }
        break;

    case EVENT_TOUCH_ACTION_MOVE:
//    } else if (tEventType == EVENT_TOUCH_ACTION_MOVE) {
        if (sDisableMoveEventsUntilTouchUpIsDone) {
            return;
        }
        if (sTouchMoveCallback != NULL) {
            sTouchMoveCallback(&tEvent.EventData.TouchEventInfo);
        }
        sCurrentPosition = tEvent.EventData.TouchEventInfo;
        break;

    case EVENT_TOUCH_ACTION_UP:
//    } else if (tEventType == EVENT_TOUCH_ACTION_UP) {
        sUpPosition = tEvent.EventData.TouchEventInfo;
#if defined(USE_STM32F3_DISCO)
        BSP_LED_Off(LED_BLUE_2); // BLUE Front
#endif
        sTouchIsStillDown = false;
#if defined(SUPPORT_LOCAL_DISPLAY)
        // may set sDisableTouchUpOnce
        handleLocalTouchUp();
#endif
        if (sDisableTouchUpOnce || sDisableMoveEventsUntilTouchUpIsDone) {
            sDisableTouchUpOnce = false;
            sDisableMoveEventsUntilTouchUpIsDone = false;
            return;
        }
        if (sTouchUpCallback != NULL) {
            sTouchUpCallback(&tEvent.EventData.TouchEventInfo);
        }
        break;

    case EVENT_TOUCH_ACTION_ERROR:
//    } else if (tEventType == EVENT_TOUCH_ACTION_ERROR) {
        // try to reset touch state
#if defined(USE_STM32F3_DISCO)
        BSP_LED_Off(LED_BLUE_2); // BLUE Front
#endif
        sUpPosition = tEvent.EventData.TouchEventInfo;
        sTouchIsStillDown = false;
//    } else
        break;
#endif // DO_NOT_NEED_BASIC_TOUCH_EVENTS

    case EVENT_BUTTON_CALLBACK:
//    if (tEventType == EVENT_BUTTON_CALLBACK) {
        sTouchIsStillDown = false; // to disable local touch up detection
        tButtonCallback = (void (*)(BDButtonHandle_t*, int16_t)) tEvent.EventData.GuiCallbackInfo.Handler;
#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
        { // "{" must be here to avoid nasty errors
            TouchButton *tLocalButton = TouchButton::getLocalButtonFromBDButtonHandle(tEvent.EventData.GuiCallbackInfo.ObjectIndex);
            tLocalButton->mValue = tEvent.EventData.GuiCallbackInfo.ValueForGuiHandler.uint16Values[0];

            if (tLocalButton->mFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
                tLocalButton->drawButton(); // handle color change for local button too
            }
            if (tLocalButton->mFlags & FLAG_BUTTON_DO_BEEP_ON_TOUCH) {
                playLocalFeedbackTone();
            }
            // The tTempButton button is removed at exit of the function :-)
            BDButton tTempButton = BDButton(tEvent.EventData.GuiCallbackInfo.ObjectIndex, tLocalButton);
            // &tTempButton.mButtonHandle must be a real button pointer, since the callback function may use it for calling button functions for this button
            tButtonCallback(&tTempButton.mButtonHandle, tLocalButton->mValue);
        }
#else
        // BDButton * is the same as BDButtonHandle_t * because BDButton only has one BDButtonHandle_t element
        tButtonCallback((BDButtonHandle_t*) &tEvent.EventData.GuiCallbackInfo.ObjectIndex,
                tEvent.EventData.GuiCallbackInfo.ValueForGuiHandler.uint16Values[0]);
#endif
        break;

    case EVENT_SLIDER_CALLBACK:
//    } else if (tEventType == EVENT_SLIDER_CALLBACK) {
        sTouchIsStillDown = false; // to disable local touch up detection
        tSliderCallback = (void (*)(BDSliderHandle_t*, int16_t)) tEvent.EventData.GuiCallbackInfo.Handler;
#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
        { // "{" must be here to avoid nasty errors
            TouchSlider *tLocalSlider = TouchSlider::getLocalSliderFromBDSliderHandle(tEvent.EventData.GuiCallbackInfo.ObjectIndex);
            tLocalSlider->mActualTouchValue = tEvent.EventData.GuiCallbackInfo.ValueForGuiHandler.uint16Values[0];

            // synchronize local slider - remote one is synchronized by local slider itself
            tLocalSlider->setValueAndDrawBar(tEvent.EventData.GuiCallbackInfo.ValueForGuiHandler.uint16Values[0]);

            // The tTempSlider button is removed at exit of the function :-)
            BDSlider tTempSlider = BDSlider(tEvent.EventData.GuiCallbackInfo.ObjectIndex, tLocalSlider);
            tSliderCallback(&tTempSlider.mSliderHandle, tLocalSlider->mActualTouchValue);
        }
#else
        tSliderCallback((BDSliderHandle_t*) &tEvent.EventData.GuiCallbackInfo.ObjectIndex,
                tEvent.EventData.GuiCallbackInfo.ValueForGuiHandler.uint16Values[0]);
#endif
        break;

    case EVENT_NUMBER_CALLBACK:
//    } else if (tEventType == EVENT_NUMBER_CALLBACK) {
        tNumberCallback = (void (*)(float)) tEvent.EventData.GuiCallbackInfo.Handler;
#if defined(ESP32) && defined DEBUG
        Serial.print("tNumberCallback=0x");
        Serial.println((uint32_t)tNumberCallback, HEX);
#endif
        tNumberCallback(tEvent.EventData.GuiCallbackInfo.ValueForGuiHandler.floatValue);
        break;

    case EVENT_SWIPE_CALLBACK:
//    } else if (tEventType == EVENT_SWIPE_CALLBACK) {
        // reset flags, since swipe is sent at touch up
        sTouchIsStillDown = false;
        if (sSwipeEndCallback != NULL) {
            // compute it locally - not required to send it over the line
            if (tEvent.EventData.SwipeInfo.SwipeMainDirectionIsX) {
                tEvent.EventData.SwipeInfo.TouchDeltaAbsMax = abs(tEvent.EventData.SwipeInfo.TouchDeltaX);
            } else {
                tEvent.EventData.SwipeInfo.TouchDeltaAbsMax = abs(tEvent.EventData.SwipeInfo.TouchDeltaY);
            }
            sSwipeEndCallback(&(tEvent.EventData.SwipeInfo));
        }
        break;

    case EVENT_LONG_TOUCH_DOWN_CALLBACK:
//    } else if (tEventType == EVENT_LONG_TOUCH_DOWN_CALLBACK) {
        if (sLongTouchDownCallback != NULL) {
            sLongTouchDownCallback(&(tEvent.EventData.TouchEventInfo));
        }
        /*
         *  Do not generate a touch up event on end of this long touch.
         *  This avoids detecting a button press on touch up, since we may have changed page
         *  and now a button is presented on the position of the long touch.
         *  Otherwise the touch up after long touch down is interpreted as button press (of the newly presented button).
         */
        sDisableTouchUpOnce = true;
        break;

    case EVENT_INFO_CALLBACK:
//    } else if (tEventType == EVENT_INFO_CALLBACK) {
        tInfoCallback =
                (void (*)(uint8_t, uint8_t, uint16_t, ByteShortLongFloatUnion)) tEvent.EventData.IntegerInfoCallbackData.Handler;
        tInfoCallback(tEvent.EventData.IntegerInfoCallbackData.SubFunction, tEvent.EventData.IntegerInfoCallbackData.ByteInfo,
                tEvent.EventData.IntegerInfoCallbackData.ShortInfo, tEvent.EventData.IntegerInfoCallbackData.LongInfo);
        break;

    case EVENT_REORIENTATION:
    case EVENT_REQUESTED_DATA_CANVAS_SIZE:
//    } else if (tEventType == EVENT_REORIENTATION || tEventType == EVENT_REQUESTED_DATA_CANVAS_SIZE) {
        /*
         * This is the event returned for initCommunication()
         * Got max display size for new orientation and local timestamp
         */
        if (tEvent.EventData.DisplaySize.XWidth > tEvent.EventData.DisplaySize.YHeight) {
            BlueDisplay1.mOrientationIsLandscape = true;
        } else {
            BlueDisplay1.mOrientationIsLandscape = false;
        }
        copyDisplaySizeAndTimestamp(&tEvent); // must be done before call of callback functions

        if (!BlueDisplay1.mBlueDisplayConnectionEstablished) {
            // if this is the first event, which sets mBlueDisplayConnectionEstablished to true, call connection callback anyway
            BlueDisplay1.mBlueDisplayConnectionEstablished = true;
            if (sConnectCallback != NULL) {
                sConnectCallback();
            }
            // Since with simpleSerial we have only buffer for 1 event, only one event is sent and we must also call redraw here
            tEventType = EVENT_REDRAW; // This sets mCurrentDisplaySize
        }

        if (tEventType == EVENT_REORIENTATION) {
            if (sReorientationCallback != NULL) {
                sReorientationCallback();
            }
            // Since with simpleSerial we have only buffer for 1 event, only one event is sent and we must also call redraw here
            tEventType = EVENT_REDRAW;
        }

        break;

    case EVENT_CONNECTION_BUILD_UP:
//    } else if (tEventType == EVENT_CONNECTION_BUILD_UP) {
        /*
         * This is the event sent if
         * Got max display size for new orientation and local timestamp
         */
        if (tEvent.EventData.DisplaySize.XWidth > tEvent.EventData.DisplaySize.YHeight) {
            BlueDisplay1.mOrientationIsLandscape = true;
        } else {
            BlueDisplay1.mOrientationIsLandscape = false;
        }
        copyDisplaySizeAndTimestamp(&tEvent); // must be done before call of sConnectCallback()
        BlueDisplay1.mBlueDisplayConnectionEstablished = true;

        // first write a NOP command for synchronizing
        BlueDisplay1.sendSync();

        if (sConnectCallback != NULL) {
            sConnectCallback();
        }
        // Since with simpleSerial we have only buffer for 1 event, we must also call redraw here
        tEventType = EVENT_REDRAW;

#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
        // do it after sConnectCallback() since the upper tends to send a reset all command
        TouchButton::createAllLocalButtonsAtRemote();
        TouchSlider::createAllLocalSlidersAtRemote();
#endif
        break;

    case EVENT_DISCONNECT:
//    } else if (tEventType == EVENT_DISCONNECT) {
        BlueDisplay1.mBlueDisplayConnectionEstablished = false;
        break;

    default:
        // check for sSensorChangeCallback != NULL since we can still have a few events for sensors even if they are just disabled
        if (tEventType >= EVENT_FIRST_SENSOR_ACTION_CODE && tEventType <= EVENT_LAST_SENSOR_ACTION_CODE
                && sSensorChangeCallback != NULL) {
            sSensorChangeCallback(tEventType - EVENT_FIRST_SENSOR_ACTION_CODE, &tEvent.EventData.SensorCallbackInfo);
        }
        break;
    }

    /*
     * This EventType is set in EVENT_REORIENTATION and EVENT_CONNECTION_BUILD_UP and therefore cannot be included in switch or else if.
     */
    if (tEventType == EVENT_REDRAW) {
        /*
         * Got current display size since host display size has changed (manually)
         */
        copyDisplaySizeAndTimestamp(&tEvent);
        if (sRedrawCallback != NULL) {
            sRedrawCallback();
        }
    }
    sBDEventJustReceived = true;
#if defined(ARDUINO)
    sMillisOfLastReceivedBDEvent = millis(); // set time of (last) event
#else
    sMillisOfLastReceivedBDEvent = getMillisSinceBoot(); // set time of (last) event
#endif
}

void copyDisplaySizeAndTimestamp(struct BluetoothEvent *aEvent) {
    BlueDisplay1.mMaxDisplaySize.XWidth = aEvent->EventData.DisplaySize.XWidth;
    BlueDisplay1.mCurrentDisplaySize.XWidth = aEvent->EventData.DisplaySize.XWidth;
    BlueDisplay1.mMaxDisplaySize.YHeight = aEvent->EventData.DisplaySize.YHeight;
    BlueDisplay1.mCurrentDisplaySize.YHeight = aEvent->EventData.DisplaySize.YHeight;
    BlueDisplay1.mHostUnixTimestamp = aEvent->EventData.DisplaySizeAndTimestamp.UnixTimestamp;
}

#if defined(SUPPORT_LOCAL_DISPLAY)
void resetTouchFlags() {
    sNothingTouched = false;
}

/**
 * Called at Touch Up
 * Handle long callback delay, check for slider and compute swipe info.
 */
void handleLocalTouchUp() {
#  if !defined(ARDUINO)
    if (sLongTouchDownCallback != NULL) {
        //disable local long touch callback
        changeDelayCallback(&callbackLongTouchDownTimeout, DISABLE_TIMER_DELAY_VALUE);
    }
#  endif
    if (sSliderIsMoveTarget) {
        sSliderIsMoveTarget = false;
        sDisableTouchUpOnce = true; // Do not call the touch up callback in handleEvent() since slider does not require one
    } else if (sSwipeEndCallbackEnabled) {
        if (abs(sDownPosition.TouchPosition.PosX - sCurrentPosition.TouchPosition.PosX) >= TOUCH_SWIPE_THRESHOLD
                || abs(sDownPosition.TouchPosition.PosY - sCurrentPosition.TouchPosition.PosY) >= TOUCH_SWIPE_THRESHOLD) {
            /*
             * Swipe recognized here
             * compute SWIPE data and call callback handler
             */
            struct Swipe tSwipeInfo;
            tSwipeInfo.TouchStartX = sDownPosition.TouchPosition.PosX;
            tSwipeInfo.TouchStartY = sDownPosition.TouchPosition.PosY;
            tSwipeInfo.TouchDeltaX = sUpPosition.TouchPosition.PosX - sDownPosition.TouchPosition.PosX;
            uint16_t tTouchDeltaXAbs = abs(tSwipeInfo.TouchDeltaX);
            tSwipeInfo.TouchDeltaY = sUpPosition.TouchPosition.PosY - sDownPosition.TouchPosition.PosY;
            uint16_t tTouchDeltaYAbs = abs(tSwipeInfo.TouchDeltaY);
            if (tTouchDeltaXAbs >= tTouchDeltaYAbs) {
                // X direction
                tSwipeInfo.SwipeMainDirectionIsX = true;
                tSwipeInfo.TouchDeltaAbsMax = tTouchDeltaXAbs;
            } else {
                tSwipeInfo.SwipeMainDirectionIsX = false;
                tSwipeInfo.TouchDeltaAbsMax = tTouchDeltaYAbs;
            }
            sSwipeEndCallback(&tSwipeInfo);
            sDisableTouchUpOnce = true; // Do not call the touch up callback in handleEvent() since we already called a callback above
        }
    }
}

/**
 * A default touch down handler, which checks, if touch is on a slider or on a button
 * and sets variables sSliderIsMoveTarget and sNothingTouched
 * @param aCurrentPositionPtr
 */
void simpleTouchDownHandler(struct TouchEvent *aCurrentPositionPtr) {
    if (TouchSlider::checkAllSliders(aCurrentPositionPtr->TouchPosition.PosX, aCurrentPositionPtr->TouchPosition.PosY)) {
        sSliderIsMoveTarget = true;
    } else {
        if (TouchButton::checkAllButtons(aCurrentPositionPtr->TouchPosition.PosX, aCurrentPositionPtr->TouchPosition.PosY,
        false)) {
            // We touched a button, so disable move events (e.g. for sliders) until touch up
            sDisableMoveEventsUntilTouchUpIsDone = true;
        } else {
            sNothingTouched = true;
        }
    }
}

void simpleTouchHandlerOnlyForButtons(struct TouchEvent *aCurrentPositionPtr) {
    if (!TouchButton::checkAllButtons(aCurrentPositionPtr->TouchPosition.PosX, aCurrentPositionPtr->TouchPosition.PosY)) {
        sNothingTouched = true;
    }
}

void simpleTouchDownHandlerOnlyForSlider(struct TouchEvent *aCurrentPositionPtr) {
    if (TouchSlider::checkAllSliders(aCurrentPositionPtr->TouchPosition.PosX, aCurrentPositionPtr->TouchPosition.PosY)) {
        sSliderIsMoveTarget = true;
    } else {
        sNothingTouched = true;
    }
}

void simpleTouchMoveHandlerForSlider(struct TouchEvent *aCurrentPositionPtr) {
    TouchSlider::checkAllSliders(aCurrentPositionPtr->TouchPosition.PosX, aCurrentPositionPtr->TouchPosition.PosY);
}

/**
 * flag for show touchpanel data on screen
 */
void setDisplayXYValuesFlag(bool aEnableDisplay) {
    sDisplayXYValuesEnabled = aEnableDisplay;
}

bool getDisplayXYValuesFlag() {
    return sDisplayXYValuesEnabled;
}

/**
 * show touchpanel data on screen
 */
void printTPData(int x, int y, color16_t aColor, color16_t aBackColor) {
    char tStringBuffer[12];
    snprintf(tStringBuffer, 12, "X:%03i Y:%03i", sCurrentPosition.TouchPosition.PosX, sCurrentPosition.TouchPosition.PosY);
    BlueDisplay1.drawText(x, y, tStringBuffer, TEXT_SIZE_11, aColor, aBackColor);
}
#endif //SUPPORT_LOCAL_DISPLAY

#endif // _EVENTHANDLER_HPP
