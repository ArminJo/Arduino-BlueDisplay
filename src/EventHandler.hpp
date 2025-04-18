/*
 * EventHandler.hpp
 *
 * Implements the methods to receive events from the Android BlueDisplay app.
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

unsigned long sMillisOfLastReceivedBDEvent;

#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
struct TouchEvent sCurrentPosition; // for printEventTouchPositionData()
bool sTouchIsStillDown = false; // To enable simple blocking touch down and touch up detection (without using a touch down or up callback).
bool sDisableMoveEventsUntilTouchUpIsDone = false; // To suppress move events after button press to avoid interpreting it for a slider. Useful, if page changed and a slider is presented on the position where the page switch button was.
#endif

bool sBDEventJustReceived = false;
bool sButtonCalledWithFalse = false; // true if last button callback has the value false/zero. If Red/Green toggle buttons are turning red, this is true. Used for stop detection.

struct BluetoothEvent remoteEvent; // To hold the current received event
#if defined(BD_USE_SIMPLE_SERIAL)
// Is used for touch down events. If remoteEvent is not empty, it is used as buffer for next regular event to avoid overwriting of remoteEvent
struct BluetoothEvent remoteTouchDownEvent;
#endif
#if defined(SUPPORT_LOCAL_DISPLAY)
struct BluetoothEvent localTouchEvent;
#  if defined(USE_TIMER_FOR_PERIODIC_LOCAL_TOUCH_CHECKS)
#  else
uint32_t sLastMillisOfLastCallOfPeriodicTouchCallback;
#  endif
#endif // defined(SUPPORT_LOCAL_DISPLAY)

bool sDisplayXYValuesEnabled = false; // displays touch values on screen

/*
 * Event handler support
 */
#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
void (*sTouchDownCallback)(struct TouchEvent*) = nullptr;
void (*sTouchMoveCallback)(struct TouchEvent*) = nullptr;
void (*sTouchUpCallback)(struct TouchEvent*) = nullptr;
bool sDisableTouchUpOnce = false; // Disable next touch up detection. E.g. because we are already in a touch handler and don't want the end of this touch to be interpreted for a newly displayed button.
bool sTouchUpCallbackEnabled = false;
#endif

#if !defined(DO_NOT_NEED_LONG_TOUCH_DOWN_AND_SWIPE_EVENTS)
#if defined(SUPPORT_LOCAL_LONG_TOUCH_DOWN_DETECTION)
uint32_t sLongTouchDownTimeoutMillis;
#endif
void (*sLongTouchDownCallback)(struct TouchEvent*) = nullptr; // The callback handler
void (*sSwipeEndCallback)(struct Swipe*) = nullptr; // can be called by event handler and by local touch up handler, which recognizes the swipe
bool sSwipeEndCallbackEnabled = false;  // for temporarily disabling swipe callbacks
#endif
#if !defined(DO_NOT_NEED_SPEAK_EVENTS)
void (*sSpeakingDoneCallback)(int16_t tErrorCode) = nullptr;
BluetoothEvent sBDSpecialEventJustReceived; // complete Event structure used, if events are polled with sBDSpecialEventJustReceived
bool sBDSpecialEventWasJustReceived = false;
void registerSpeakingDoneCallback(void (*aSpeakingDoneCallback)(int16_t tErrorCode)) {
    sSpeakingDoneCallback = aSpeakingDoneCallback;
}
#endif

void (*sConnectCallback)() = nullptr;
void (*sRedrawCallback)() = nullptr; // Intended to redraw screen, if size of display changes.
void (*sReorientationCallback)() = nullptr;
void (*sSensorChangeCallback)(uint8_t aEventType, struct SensorCallback *aSensorCallbackInfo) = nullptr;

void copyDisplaySizeAndTimestampAndSetOrientation(struct BluetoothEvent *aEvent);

/*
 * Connect event also calls redraw event
 */
void registerConnectCallback(void (*aConnectCallback)()) {
    sConnectCallback = aConnectCallback;
}

/*
 * Redraw event is intended to redraw screen, if size of display changes.
 * The app itself does the scaling of the screen content, but this can be coarse, especially when inflating
 * so we will get better quality, if we redraw the content.
 * Size changes will not happen, if BD_FLAG_USE_MAX_SIZE is set, and no reorientation (implying size changes) happens.
 *
 * The redraw event handler is automatically called directly after the reconnect or reorientation handler.
 */
void registerRedrawCallback(void (*aRedrawCallback)()) {
    sRedrawCallback = aRedrawCallback;
}

// !!! Must be without comment and closed by @formatter:on
// @formatter:off
void (* getRedrawCallback())() {
    return sRedrawCallback;
}
// @formatter:on

/*
 * Reorientation event also calls redraw event
 */
void registerReorientationCallback(void (*aReorientationCallback)()) {
    sReorientationCallback = aReorientationCallback;
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
    if (aSensorChangeCallback == nullptr) {
        tSensorEnable = false;
    }
    BlueDisplay1.setSensor(aSensorType, tSensorEnable, aSensorRate, aFilterFlag);
    sSensorChangeCallback = aSensorChangeCallback;
}

#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
void registerTouchDownCallback(void (*aTouchDownCallback)(struct TouchEvent *aCurrentPositionPtr)) {
    sTouchDownCallback = aTouchDownCallback;
}

void registerTouchMoveCallback(void (*aTouchMoveCallback)(struct TouchEvent *aCurrentPositionPtr)) {
    sTouchMoveCallback = aTouchMoveCallback;
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
    sTouchUpCallbackEnabled = (aTouchUpCallback != nullptr);
}
/**
 * disable or enable touch up callback
 * used by numberpad
 * @param aTouchUpCallbackEnabled
 */
void setTouchUpCallbackEnabled(bool aTouchUpCallbackEnabled) {
    if (aTouchUpCallbackEnabled && sTouchUpCallback != nullptr) {
        sTouchUpCallbackEnabled = true;
    } else {
        sTouchUpCallbackEnabled = false;
    }
}

// @formatter:off
/**
 * return pointer to end touch callback function
 */
void (* getTouchUpCallback())(struct TouchEvent * ) {
    return sTouchUpCallback;
}
// @formatter:on

#endif // !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)

#if !defined(DO_NOT_NEED_LONG_TOUCH_DOWN_AND_SWIPE_EVENTS)
/**
 * Register a callback routine which is only called after a timeout if screen is still touched
 * Send only timeout value to BD Host
 */
void registerLongTouchDownCallback(void (*aLongTouchDownCallback)(struct TouchEvent*), uint16_t aLongTouchDownTimeoutMillis) {
    sLongTouchDownCallback = aLongTouchDownCallback;
#if defined(SUPPORT_LOCAL_LONG_TOUCH_DOWN_DETECTION)
    sLongTouchDownTimeoutMillis = aLongTouchDownTimeoutMillis;
#  if defined(USE_TIMER_FOR_PERIODIC_LOCAL_TOUCH_CHECKS)
    // initialize timer for long touch detection
    if (aLongTouchDownCallback == nullptr) {
        changeDelayCallback(&callbackHandlerForLongTouchDownTimeout, DISABLE_TIMER_DELAY_VALUE); // housekeeping - disable timeout
    }
#  endif
#endif
#if !defined(DISABLE_REMOTE_DISPLAY)
    BlueDisplay1.setLongTouchDownTimeout(aLongTouchDownTimeoutMillis);
#endif
}

/**
 * Register a callback routine which is called when touch goes up and swipe detected
 */
void registerSwipeEndCallback(void (*aSwipeEndCallback)(struct Swipe*)) {
    sSwipeEndCallback = aSwipeEndCallback;
#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
    // Disable next end touch since we are already in a touch handler and don't want the end of this touch to be interpreted for a swipe
    if (sTouchIsStillDown) {
        sDisableTouchUpOnce = true;
    }
#endif
    sSwipeEndCallbackEnabled = (aSwipeEndCallback != nullptr);
}

void setSwipeEndCallbackEnabled(bool aSwipeEndCallbackEnabled) {
    if (aSwipeEndCallbackEnabled && sSwipeEndCallback != nullptr) {
        sSwipeEndCallbackEnabled = true;
    } else {
        sSwipeEndCallbackEnabled = false;
    }
}
#endif // #if !defined(DO_NOT_NEED_LONG_TOUCH_DOWN_AND_SWIPE_EVENTS)

/*
 * Delay, which also checks for events
 * AVR - Is not affected by overflow of millis()!
 */
void delayMillisWithCheckAndHandleEvents(unsigned long aDelayMillis) {
    unsigned long tStartMillis = millis();
    while (millis() - tStartMillis < aDelayMillis) {
        checkAndHandleEvents();
#if defined(ESP8266)
        yield(); // required for ESP8266
#endif
    }
}

/*
 * Like delayMillisWithCheckAndHandleEvents, but extends wait, if receiving is active
 */
void delayMillisWithCheckForStartedReceivingAndHandleEvents(unsigned long aDelayMillis) {
    unsigned long tStartMillis = millis();
    while (millis() - tStartMillis < aDelayMillis || isReceivingActive()) {
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

    unsigned long tStartMillis = millis();
    while (millis() - tStartMillis < aDelayMillis) {
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

/*
 * Assume that stop is requested, if a button is called with value 0/false, i.e. a Red/Green toggle button is set to red.
 */
bool isStopRequested() {
    checkAndHandleEvents();
    return sButtonCalledWithFalse;
}

/*
 * Special delay function for the Red/Green toggle buttons. Returns prematurely if a button turns to red.
 * To be used in blocking functions as delay
 * @return  true - as soon as a turn to red is received
 */
bool delayMillisAndCheckForStop(uint16_t aDelayMillis) {
    unsigned long tStartMillis = millis();
    while (millis() - tStartMillis < aDelayMillis) {
        if (isStopRequested()) {
            return true;
        }
#if defined(ESP8266)
            yield(); // required for ESP8266
#endif
    }
    return false;
}

/*
 * If receiving was started, wait until event was completely received and processed, but timeout after 40 ms
 */
void checkForStartedReceivingAndHandleEvents() {
    auto tStartMillis = millis();
    do {
        checkAndHandleEvents();
        if (millis() - tStartMillis >= 40) {
            // If receiving does not terminate within 40 ms, we assume a sync problem because of a skipped input byte
            sReceiveBufferOutOfSync = true;
            break;
        }
    } while (isReceivingActive());
}

/**
 * Is called by main loop
 * Transmission time of a slider event is 1300 us
 */
void checkAndHandleEvents() {
#if defined(HAL_WWDG_MODULE_ENABLED)
    Watchdog_reload();
#endif

#if defined(SUPPORT_LOCAL_DISPLAY) && defined(LOCAL_DISPLAY_GENERATES_BD_EVENTS)
    /*
     * Check if a local event happened, i.e. the localTouchEvent was written by an touch device interrupt handler
     */
    if (localTouchEvent.EventType != EVENT_NO_EVENT) {
        handleEvent(&localTouchEvent);
    }
#endif

#if !defined(DISABLE_REMOTE_DISPLAY)
#  if defined(ARDUINO)
#    if defined(BD_USE_SIMPLE_SERIAL)
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
    // get actual DMA byte count
    int32_t tBytesAvailable = getReceiveBytesAvailable();
    if (tBytesAvailable != 0) {
        serialEvent();
    }
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
    if (tEventType <= EVENT_TOUCH_ACTION_MOVE && isDisplayXYValuesEnabled()) {
        printEventTouchPositionData(30, 2, COLOR16_BLACK, COLOR16_WHITE);
    }
#endif

    switch (tEventType) { // switch requires 36 bytes more code but is clearer to understand :-(

#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
    case EVENT_TOUCH_ACTION_DOWN:
//    if (tEventType == EVENT_TOUCH_ACTION_DOWN) {
        sCurrentPosition = tEvent.EventData.TouchEventInfo;
#  if defined(USE_STM32F3_DISCO)
        BSP_LED_On(LED_BLUE_2); // BLUE Front
#  endif
        sTouchIsStillDown = true;
        if (sTouchDownCallback != nullptr) {
            sTouchDownCallback(&tEvent.EventData.TouchEventInfo);
        }
        break;

    case EVENT_TOUCH_ACTION_MOVE:
//    } else if (tEventType == EVENT_TOUCH_ACTION_MOVE) {
        if (sDisableMoveEventsUntilTouchUpIsDone) {
            return;
        }
        if (sTouchMoveCallback != nullptr) {
            sTouchMoveCallback(&tEvent.EventData.TouchEventInfo);
        }
        sCurrentPosition = tEvent.EventData.TouchEventInfo;
        break;

    case EVENT_TOUCH_ACTION_UP:
//    } else if (tEventType == EVENT_TOUCH_ACTION_UP) {
#  if defined(USE_STM32F3_DISCO)
        BSP_LED_Off(LED_BLUE_2); // BLUE Front
#  endif
        sTouchIsStillDown = false;
        if (sDisableTouchUpOnce || sDisableMoveEventsUntilTouchUpIsDone) {
            sDisableTouchUpOnce = false;
            sDisableMoveEventsUntilTouchUpIsDone = false;
            return;
        }
        if (sTouchUpCallback != nullptr) {
            sTouchUpCallback(&tEvent.EventData.TouchEventInfo);
        }
        break;

    case EVENT_TOUCH_ACTION_ERROR:
//    } else if (tEventType == EVENT_TOUCH_ACTION_ERROR) {
        // try to reset touch state
#if defined(USE_STM32F3_DISCO)
        BSP_LED_Off(LED_BLUE_2); // BLUE Front
#endif
        sTouchIsStillDown = false;
//    } else
        break;
#endif // DO_NOT_NEED_BASIC_TOUCH_EVENTS

    case EVENT_BUTTON_CALLBACK:
//    if (tEventType == EVENT_BUTTON_CALLBACK) {
#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
        sTouchIsStillDown = false; // to disable local touch up detection
#endif
#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
        { // "{" must be here to avoid nasty errors
            LocalTouchButton *tLocalButton = LocalTouchButton::getLocalTouchButtonFromBDButtonHandle(tEvent.EventData.GuiCallbackInfo.ObjectIndex);
            tLocalButton->mValue = tEvent.EventData.GuiCallbackInfo.ValueForGUICallback.int16Values[0]; // we support only 16 bit values for buttons
            /*
             * We can not call performTouchAction() of the local button here.
             * It is because for autorepeat buttons, CallbackFunctionAddress is the mOriginalButtonOnTouchHandler and not the mOnTouchHandler
             * and Red/Green toggle button handling will loop between local and remote.
             */
            if ((tLocalButton->mFlags & FLAG_BUTTON_TYPE_TOGGLE) && !(tLocalButton->mFlags & FLAG_BUTTON_TYPE_MANUAL_REFRESH)) {
                tLocalButton->drawButton(); // handle color change for local button too
            }
            if (tLocalButton->mFlags & FLAG_BUTTON_DO_BEEP_ON_TOUCH) {
                LocalTouchButton::playFeedbackTone();
            }
            // for autorepeat buttons, this is the mOriginalButtonOnTouchHandler and not the mOnTouchHandler
            ((void (*)(BDButton*, int16_t)) tEvent.EventData.GuiCallbackInfo.CallbackFunctionAddress)(tLocalButton->mBDButtonPtr, tLocalButton->mValue);
        }
#elif !defined(SUPPORT_LOCAL_DISPLAY)
        sButtonCalledWithFalse = (tEvent.EventData.GuiCallbackInfo.ValueForGUICallback.uint16Values[0] == false);
        // The BDButton object is just a 8 bit (16 bit) unsigned integer holding the remote index of the button.
        // So every pointer to a memory location holding the index of the button is a valid pointer to BDButton :-).
        ((void (*)(BDButton*, int16_t)) tEvent.EventData.GuiCallbackInfo.CallbackFunctionAddress)(
                (BDButton*) &tEvent.EventData.GuiCallbackInfo.ObjectIndex, // Pointer to button object (remote index number) (on stack)
                tEvent.EventData.GuiCallbackInfo.ValueForGUICallback.int16Values[0]); // we support only 16 bit values for buttons
#endif
        break;

    case EVENT_SLIDER_CALLBACK:
//    } else if (tEventType == EVENT_SLIDER_CALLBACK) {
#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
        sTouchIsStillDown = false; // to disable local touch up detection
#endif
#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
        { // "{" must be here to avoid nasty errors
            LocalTouchSlider *tLocalSlider = LocalTouchSlider::getLocalSliderFromBDSliderHandle(tEvent.EventData.GuiCallbackInfo.ObjectIndex);
            tLocalSlider->mActualTouchValue = tEvent.EventData.GuiCallbackInfo.ValueForGUICallback.int16Values[0];
            // synchronize local slider - remote one is synchronized by local slider itself
            tLocalSlider->setValueAndDrawBar(tEvent.EventData.GuiCallbackInfo.ValueForGUICallback.int16Values[0]);
            ((void (*)(BDSlider*, int16_t)) tEvent.EventData.GuiCallbackInfo.CallbackFunctionAddress)(tLocalSlider->mBDSliderPtr, tLocalSlider->mActualTouchValue);
        }
#else
        // The BDSlider object is just a 8 bit (16 bit) unsigned integer holding the remote index of the slider.
        // So every pointer to a memory location holding the index of the slider is a valid pointer to BDSlider :-).
        ((void (*)(BDSlider*, int16_t)) tEvent.EventData.GuiCallbackInfo.CallbackFunctionAddress)(
                (BDSlider*) &tEvent.EventData.GuiCallbackInfo.ObjectIndex, // Pointer to slider object (remote index number) (on stack)
                tEvent.EventData.GuiCallbackInfo.ValueForGUICallback.int16Values[0]);
#endif
        break;

    case EVENT_NUMBER_CALLBACK:
//    } else if (tEventType == EVENT_NUMBER_CALLBACK) {
#if defined(ESP32) && defined DEBUG
        Serial.print("tNumberCallback=0x");
        Serial.println((uint32_t)tEvent.EventData.GuiCallbackInfo.CallbackFunctionAddress, HEX);
#endif
        ((void (*)(float)) tEvent.EventData.GuiCallbackInfo.CallbackFunctionAddress)(
                tEvent.EventData.GuiCallbackInfo.ValueForGUICallback.floatValue);
        break;

    case EVENT_SWIPE_CALLBACK:
//    } else if (tEventType == EVENT_SWIPE_CALLBACK) {
#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
        // reset flags, since swipe is sent at touch up
        sTouchIsStillDown = false;
#endif
#if defined(DO_NOT_NEED_LONG_TOUCH_DOWN_AND_SWIPE_EVENTS)
        break;
#else
        if (sSwipeEndCallback != nullptr) {
            // compute it locally - not required to send it over the line
            if (tEvent.EventData.SwipeInfo.SwipeMainDirectionIsX) {
                tEvent.EventData.SwipeInfo.TouchDeltaAbsMax = abs(tEvent.EventData.SwipeInfo.TouchDeltaX);
            } else {
                tEvent.EventData.SwipeInfo.TouchDeltaAbsMax = abs(tEvent.EventData.SwipeInfo.TouchDeltaY);
            }
            sSwipeEndCallback(&(tEvent.EventData.SwipeInfo));
        }
#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
        // If we get an additional touch up event from the touch up, which triggers the swipe detection, we should ignore it.
        sDisableTouchUpOnce = true;
#endif
        break;

    case EVENT_LONG_TOUCH_DOWN_CALLBACK:
//    } else if (tEventType == EVENT_LONG_TOUCH_DOWN_CALLBACK) {
        if (sLongTouchDownCallback != nullptr) {
            sLongTouchDownCallback(&(tEvent.EventData.TouchEventInfo));
        }
#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
        /*
         *  Do not generate a touch up event on end of this long touch.
         *  This avoids detecting a button press on touch up, since we may have changed page
         *  and now a button is presented on the position of the long touch.
         *  Otherwise the touch up after long touch down is interpreted as button press (of the newly presented button).
         */
        sDisableTouchUpOnce = true;
#endif
        break;
#endif

    case EVENT_INFO_CALLBACK:
//    } else if (tEventType == EVENT_INFO_CALLBACK) {
        ((void (*)(uint8_t, uint8_t, uint16_t, ByteShortLongFloatUnion)) tEvent.EventData.IntegerInfoCallbackData.CallbackFunctionAddress)(
                tEvent.EventData.IntegerInfoCallbackData.SubFunction, tEvent.EventData.IntegerInfoCallbackData.ByteInfo,
                tEvent.EventData.IntegerInfoCallbackData.ShortInfo, tEvent.EventData.IntegerInfoCallbackData.LongInfo);
        break;

#if !defined(DO_NOT_NEED_SPEAK_EVENTS)
    case EVENT_SPEAKING_DONE:
        /*
         * Set data used for polling
         */
        sBDSpecialEventJustReceived = tEvent;
        sBDSpecialEventWasJustReceived = true;

        if (sSpeakingDoneCallback != nullptr) {
            sSpeakingDoneCallback(tEvent.EventData.UnsignedShortArray[0]);
        }
        break;
#endif

    case EVENT_REORIENTATION:
    case EVENT_REQUESTED_DATA_CANVAS_SIZE:
//    } else if (tEventType == EVENT_REORIENTATION || tEventType == EVENT_REQUESTED_DATA_CANVAS_SIZE) {
        /*
         * Got max display size for new orientation and local timestamp
         */
        copyDisplaySizeAndTimestampAndSetOrientation(&tEvent); // must be done before call of callback functions

        if (!BlueDisplay1.mBlueDisplayConnectionEstablished) {
            // if this is the first event, which sets mBlueDisplayConnectionEstablished to true, call connection callback too
            BlueDisplay1.mBlueDisplayConnectionEstablished = true;
            if (sConnectCallback != nullptr) {
                sConnectCallback();
            }
#if !defined(ONLY_CONNECT_EVENT_REQUIRED)
            // Since with simpleSerial we have only buffer for 1 event, only one event is sent and we must also call redraw here
            tEventType = EVENT_REDRAW; // This calls redraw callback
#endif
        }

#if !defined(ONLY_CONNECT_EVENT_REQUIRED)
        if (tEventType == EVENT_REORIENTATION) {
            if (sReorientationCallback != nullptr) {
                sReorientationCallback();
            }
            tEventType = EVENT_REDRAW;
        }
#endif

        break;

    case EVENT_CONNECTION_BUILD_UP:
//    } else if (tEventType == EVENT_CONNECTION_BUILD_UP) {
        /*
         * This is the event for initCommunication()
         * Got max display size for new orientation and local timestamp
         */
        copyDisplaySizeAndTimestampAndSetOrientation(&tEvent); // must be done before call of sConnectCallback()
        BlueDisplay1.mBlueDisplayConnectionEstablished = true;

        // first write a 40 bytes NOP command for synchronizing
        BlueDisplay1.sendSync();

        if (sConnectCallback != nullptr) {
            sConnectCallback();
        }
#if !defined(ONLY_CONNECT_EVENT_REQUIRED)
        tEventType = EVENT_REDRAW; // We also call redraw here, which in turn sets mCurrentDisplaySize and mHostUnixTimestamp
#endif

#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
        // do it after sConnectCallback() since the upper tends to send a reset all command
        LocalTouchButton::createAllLocalButtonsAtRemote();
        LocalTouchSlider::createAllLocalSlidersAtRemote();
#endif
        break;

    case EVENT_DISCONNECT:
//    } else if (tEventType == EVENT_DISCONNECT) {
        BlueDisplay1.mBlueDisplayConnectionEstablished = false;
        break;

    default:
        // check for sSensorChangeCallback != nullptr since we can still have a few events for sensors even if they are just disabled
        if (tEventType >= EVENT_FIRST_SENSOR_ACTION_CODE && tEventType <= EVENT_LAST_SENSOR_ACTION_CODE
                && sSensorChangeCallback != nullptr) {
            sSensorChangeCallback(tEventType - EVENT_FIRST_SENSOR_ACTION_CODE, &tEvent.EventData.SensorCallbackInfo);
        }

        break;
    }

#if !defined(ONLY_CONNECT_EVENT_REQUIRED)
    /*
     * This EventType is set in EVENT_REORIENTATION and EVENT_CONNECTION_BUILD_UP and therefore cannot be included in switch or else if.
     */
    if (tEventType == EVENT_REDRAW) {
        /*
         * Got current display size since host display size has changed (manually)
         * sets mCurrentDisplaySize and mHostUnixTimestamp
         */
        copyDisplaySizeAndTimestampAndSetOrientation(&tEvent);
        if (sRedrawCallback != nullptr) {
            sRedrawCallback();
        }
    }
#endif
    /*
     * End of individual event handling
     */

    sBDEventJustReceived = true;
#if defined(ARDUINO)
    sMillisOfLastReceivedBDEvent = millis(); // set time of (last) event
#else
    sMillisOfLastReceivedBDEvent = millis(); // set time of (last) event
#endif
}

void copyDisplaySizeAndTimestampAndSetOrientation(struct BluetoothEvent *aEvent) {
    if (aEvent->EventData.DisplaySize.XWidth > aEvent->EventData.DisplaySize.YHeight) {
        BlueDisplay1.mOrientationIsLandscape = true;
    } else {
        BlueDisplay1.mOrientationIsLandscape = false;
    }
    BlueDisplay1.mHostDisplaySize.XWidth = aEvent->EventData.DisplaySizeAndTimestamp.DisplaySize.XWidth;
    BlueDisplay1.mHostDisplaySize.YHeight = aEvent->EventData.DisplaySizeAndTimestamp.DisplaySize.YHeight;
    BlueDisplay1.mHostUnixTimestamp = aEvent->EventData.DisplaySizeAndTimestamp.UnixTimestamp;
}

#if !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS) && !defined(ESP32) // ESP32 treats warnings as errors :-(
/**
 * Flag for displaying event touchpanel X/Y data on screen
 */
void setDisplayXYValuesFlag(bool aEnableDisplay) {
    sDisplayXYValuesEnabled = aEnableDisplay;
}
bool isDisplayXYValuesEnabled() {
    return sDisplayXYValuesEnabled;
}
/**
 * show touchpanel data on screen
 */
void printEventTouchPositionData(int x, int y, color16_t aColor, color16_t aBackgroundColor) {
    char tStringBuffer[12];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
// We want it this way, but we get a warning '%03i' directive output may be truncated writing between 3 and 5 bytes into a region of size between 2 and 4
    snprintf(tStringBuffer, 12, "X:%03i Y:%03i", sCurrentPosition.TouchPosition.PositionX,
            sCurrentPosition.TouchPosition.PositionY);
#pragma GCC diagnostic pop
    BlueDisplay1.drawText(x, y, tStringBuffer, TEXT_SIZE_11, aColor, aBackgroundColor);
}
#endif // !defined(DO_NOT_NEED_BASIC_TOUCH_EVENTS)
#endif // _EVENTHANDLER_HPP
