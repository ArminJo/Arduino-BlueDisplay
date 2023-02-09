/**
 * LocalEventHelper.hpp
 *
 *  Support for event generation / handling of local touch events
 *
 *
 *  Copyright (C) 2023  Armin Joachimsmeyer
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
 */

#ifndef _LOCAL_EVENT_HELPER_HPP
#define _LOCAL_EVENT_HELPER_HPP

#include "ADS7846.h"
#include "EventHandler.h"

uint8_t sTouchObjectTouched; // On touch down, this changes from NO_TOUCH to one of BUTTON_TOUCHED, SLIDER_TOUCHED, PANEL_TOUCHED

#if defined(SUPPORT_LOCAL_LONG_TOUCH_DOWN_DETECTION)  && !defined(USE_TIMER_FOR_PERIODIC_LOCAL_TOUCH_CHECKS)
uint32_t sLastTouchDownMillis; // time of last touch down for detecting long touch down
bool sTouchUpCallbackEnabledOnce; // Required to call only once at a longer touch
#endif

#define TOUCH_SWIPE_RESOLUTION_MILLIS 20

#include "LocalGUI/LocalTouchButton.h"
#include "LocalGUI/LocalTouchSlider.h"

/**
 * To be called by main loop
 * Reads touch panel data and handles down and up events by calling checkAllButtons and checkAllSliders
 */
void checkAndHandleTouchPanelEvents() {
    TouchPanel.readData();
    handleTouchPanelEvents();
}

#if !defined(USE_TIMER_FOR_PERIODIC_LOCAL_TOUCH_CHECKS)
/**
 * Handles down events by calling checkAllButtons and checkAllSliders
 * No suppression of micro moves using mTouchLastPosition here!
 */
void handleTouchPanelEvents() {
    if (!TouchPanel.ADS7846TouchActive) {
        /**
         * No touch here, reset flags
         */
        sTouchObjectTouched = NO_TOUCH;

    } else {
        auto tPositionX = TouchPanel.getCurrentX();
        auto tPositionY = TouchPanel.getCurrentY();

#  if defined(SUPPORT_LOCAL_LONG_TOUCH_DOWN_DETECTION)  && !defined(USE_TIMER_FOR_PERIODIC_LOCAL_TOUCH_CHECKS)
        if(sTouchObjectTouched == NO_TOUCH) {
            /*
             * Here we have the touch down event
             */
            sLastTouchDownMillis = millis();
            sTouchUpCallbackEnabledOnce = true;
        }

        /*
         * Periodically check for long touch down
         */
        if(sTouchUpCallbackEnabledOnce && sLongTouchDownCallback != NULL && sTouchObjectTouched != SLIDER_TOUCHED) {
            if( millis() - sLongTouchDownTimeoutMillis > sLastTouchDownMillis) {
                sTouchUpCallbackEnabledOnce = false;

                struct TouchEvent tLongTouchDownEvent;
                // long touch timeout detected -> call callback once.
                tLongTouchDownEvent.TouchPosition = TouchPanel.mCurrentTouchPosition; // do not support TouchPointerIndex here
                sLongTouchDownCallback(&tLongTouchDownEvent);
            }
        }
#  endif

        /*
         * Check if button or slider is touched.
         * Check button first in order to give priority to buttons which are overlapped by sliders.
         * Remember which is pressed first and "stay" there.
         */
        // Check button only once at a new touch, check autorepeat buttons always to create autorepeat timing
        if (sTouchObjectTouched == NO_TOUCH || sTouchObjectTouched == BUTTON_TOUCHED) {
            if (LocalTouchButton::checkAllButtons(tPositionX, tPositionY, sTouchObjectTouched == BUTTON_TOUCHED)) {
                sTouchObjectTouched = BUTTON_TOUCHED;
            }
        }
        // Check slider only once at a new touch, or always if initially touched
        if (sTouchObjectTouched == NO_TOUCH || sTouchObjectTouched == SLIDER_TOUCHED) {
            if (LocalTouchSlider::checkAllSliders(tPositionX, tPositionY)) {
                sTouchObjectTouched = SLIDER_TOUCHED;
            }
        }
        if (sTouchObjectTouched == NO_TOUCH) {
            // initially no object was touched
            sTouchObjectTouched = PANEL_TOUCHED;
        }
    }
}

#else //  !defined(USE_TIMER_FOR_PERIODIC_LOCAL_TOUCH_CHECKS)

void checkForMovesAndSwipes(); // local forward declaration
extern struct BluetoothEvent localTouchEvent; // helps the eclipse indexer :-(

/*
 * To be called by ADS7846 interrupt on touch down and touch up
 * This in turn starts the periodic timer with changeDelayCallback() to check for moves, long touch down and swipes.
 */
void handleTouchPanelEvents(void) {
    bool tLevel = ADS7846_getInteruptLineLevel();
    BSP_LED_Toggle (LED_GREEN_2); // GREEN RIGHT
    if (!tLevel) {
        /*
         * touchDown event - line input is low
         * Fill in the local event structure, which is read by checkAndHandleEvents()
         */
        TouchPanel.readData(ADS7846_READ_OVERSAMPLING_DEFAULT); // this disables interrupt for additional TOUCH_DELAY_AFTER_READ_MILLIS
        TouchPanel.mTouchDownPosition= TouchPanel.mCurrentTouchPosition;
        TouchPanel.mLastTouchPosition = TouchPanel.mCurrentTouchPosition;

        /*
         * Check if button or slider is touched
         * Check button first in order to give priority to buttons which are overlapped by sliders
         * Remember which is pressed first and "stay" there
         * !!! We are calling the slider and button callback here in ISR context !!!
         */
        LocalTouchButton *tTouchedButton = LocalTouchButton::find(TouchPanel.mCurrentTouchPosition.PositionX,
                TouchPanel.mCurrentTouchPosition.PositionY, false);
        if (tTouchedButton != NULL) {
            if (tTouchedButton->mFlags & FLAG_BUTTON_TYPE_AUTOREPEAT) {
                // Local autorepeat button callback, which is autorepeatTouchHandler() can not be called by event, so we must call it directly here
                tTouchedButton->mOnTouchHandler(tTouchedButton, 0);
            } else {
                //Red/Green button handling
                if (tTouchedButton->mFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
                    // Toggle value for Red/Green button, because we called findButton() and not checkButton() which would do the handling for us.
                    tTouchedButton->mValue = !tTouchedButton->mValue;
                    if (!(tTouchedButton->mFlags & FLAG_BUTTON_TYPE_MANUAL_REFRESH)) {
#  if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
                        tTouchedButton->mBDButtonPtr->setValueAndDraw(tTouchedButton->mValue); // Update also the remote button
                        // local button refresh is done by event handler
#  endif
                    }
                }
                /*
                 * Create button event for processing by main loop in order to return from ISR
                 */
                localTouchEvent.EventType = EVENT_BUTTON_CALLBACK;
                localTouchEvent.EventData.GuiCallbackInfo.CallbackFunctionAddress = (void*) tTouchedButton->mOnTouchHandler;
                localTouchEvent.EventData.GuiCallbackInfo.ValueForGUICallback.uint16Values[0] = tTouchedButton->mValue; // we support only 16 bit values for buttons
#  if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
                localTouchEvent.EventData.GuiCallbackInfo.ObjectIndex = tTouchedButton->mBDButtonPtr->mButtonHandle;
#  endif
            }
            sTouchObjectTouched = BUTTON_TOUCHED;

        } else if (LocalTouchSlider::checkAllSliders(TouchPanel.mCurrentTouchPosition.PositionX,
                TouchPanel.mCurrentTouchPosition.PositionY)) {
            sTouchObjectTouched = SLIDER_TOUCHED;

        } else {
            // no button or slider touched -> plain touch down event
            sTouchObjectTouched = PANEL_TOUCHED;

            localTouchEvent.EventData.TouchEventInfo.TouchPosition = TouchPanel.mCurrentTouchPosition;
            localTouchEvent.EventData.TouchEventInfo.TouchPointerIndex = 0;
            localTouchEvent.EventType = EVENT_TOUCH_ACTION_DOWN;
        }

        if (TouchPanel.ADS7846TouchActive) {
            TouchPanel.ADS7846TouchStart = true;
            /*
             * Enable move recognition by periodically reading actual position.
             */
            changeDelayCallback(&checkForMovesAndSwipes, TOUCH_SWIPE_RESOLUTION_MILLIS);

            /*
             * Enable long touch down detection if touch is not on a slider
             */
            if (sTouchObjectTouched != SLIDER_TOUCHED && sLongTouchDownCallback != NULL) {
                changeDelayCallback(&callbackHandlerForLongTouchDownTimeout, sLongTouchDownTimeoutMillis); // enable timeout
            }
        } else {
            // line was active but values could not be read correctly (e.g. because of delay by higher prio interrupts)
            BSP_LED_Toggle (LED_BLUE); // BLUE Back
        }
    } else {
        /**
         * Touch released here
         */
        changeDelayCallback(&checkForMovesAndSwipes, DISABLE_TIMER_DELAY_VALUE); // disable periodic interrupts which can call handleTouchRelease
        if (TouchPanel.ADS7846TouchActive) {
            TouchPanel.ADS7846TouchActive = false;
            handleLocalTouchUp();
            sTouchObjectTouched = NO_TOUCH;
        }
    }
    resetBacklightTimeout();
// Clear the EXTI line pending bit and enable new interrupt on other edge
    ADS7846_ClearITPendingBit();
}

/**
 * Callback routine for SysTick handler
 * Enabled at touch down at a rate of TOUCH_SWIPE_RESOLUTION_MILLIS (20ms)
 */
void checkForMovesAndSwipes(void) {
    int tLevel = ADS7846_getInteruptLineLevel();
    if (!tLevel) {
        // pressed - line input is low, touch still active
        TouchPanel.readData(ADS7846_READ_OVERSAMPLING_DEFAULT);
        localTouchEvent.EventData.TouchEventInfo.TouchPosition = TouchPanel.mCurrentTouchPosition;
        localTouchEvent.EventData.TouchEventInfo.TouchPointerIndex = 0;
        if (!TouchPanel.ADS7846TouchActive) {
            // went inactive just while readRawData()
            if (sTouchObjectTouched == PANEL_TOUCHED) {
                // Generate touch up event if no button or slider was touched
                handleLocalTouchUp();
            }
        } else {
            /*
             * Check if autorepeat button or slider is still touched.
             * Check button first in order to give priority to buttons which are overlapped by sliders.
             * Remember which is pressed first and "stay" there.
             * !!! we are calling the autorepeat button and slider callbacks here in ISR context !!!
             * Calling autorepeat button callback with event is too complicated,
             * because the event is yet not aware of the class LocalTouchButtonAutorepeat required by
             * the autorepeatTouchHandler(), which generates the timing, and therefore the callback crashes.
             */
            if (sTouchObjectTouched == BUTTON_TOUCHED) {
                // Check autorepeat buttons only if slider was not touched before
                LocalTouchButton::checkAllButtons(TouchPanel.mCurrentTouchPosition.PositionX,
                        TouchPanel.mCurrentTouchPosition.PositionY, true);
            } else if (sTouchObjectTouched == SLIDER_TOUCHED) {
                LocalTouchSlider::checkAllSliders(TouchPanel.mCurrentTouchPosition.PositionX,
                        TouchPanel.mCurrentTouchPosition.PositionY);
            } else {
                /*
                 * Do not accept pseudo or micro moves as an event.
                 * In the BlueDisplay app the threshold is set to mCurrentViewWidth / 100, which is 3 pixel here.
                 */
                if (abs(TouchPanel.mLastTouchPosition.PositionX - TouchPanel.mCurrentTouchPosition.PositionX) >= 3
                        || abs(TouchPanel.mLastTouchPosition.PositionY - TouchPanel.mCurrentTouchPosition.PositionY) >= 3) {
                    TouchPanel.mLastTouchPosition = TouchPanel.mCurrentTouchPosition;

                    // Significant move here

                    // avoid overwriting other (e.g long touch down) events
                    if (localTouchEvent.EventType == EVENT_NO_EVENT) {
                        localTouchEvent.EventType = EVENT_TOUCH_ACTION_MOVE;
                    }
                }
            }
            // restart timer
            changeDelayCallback(&checkForMovesAndSwipes, TOUCH_SWIPE_RESOLUTION_MILLIS);
        }
    } else {
        handleLocalTouchUp();
        sTouchObjectTouched = NO_TOUCH;
    }
}

/**
 * This handler is called on both edge of touch interrupt signal.
 * Actually the ADS7846 IRQ signal bounces on rising edge (going inactive).
 * This can happen up to 8 milliseconds after initial transition.
 */
extern "C" void EXTI1_IRQHandler(void) {
// wait for stable reading
    delay(TOUCH_DEBOUNCE_DELAY_MILLIS);
    handleTouchPanelEvents();
}

/*
 * Timer functions to support periodic local touch check
 */
/**
 * set CallbackPeriod
 */
void setPeriodicTouchCallbackPeriod(uint32_t aCallbackPeriod) {
    sPeriodicCallbackPeriodMillis = aCallbackPeriod;
}

/**
 * Callback routine for SysTick handler, must create an event structure, because it is called in ISR context!
 * Creates event if no slider was touched and no swipe gesture was started
 */
void callbackHandlerForLongTouchDownTimeout() {
#  if !defined(ARDUINO)
    assert_param(sLongTouchDownCallback != NULL);
#  endif
// No long touch if slider touched or swipe is made
    /*
     * Check if a swipe is intended (position has moved over threshold).
     * If not, call long touch callback
     */
    if(sTouchObjectTouched != SLIDER_TOUCHED){
        if (abs(TouchPanel.mTouchDownPosition.PositionX - TouchPanel.mCurrentTouchPosition.PositionX) < TOUCH_SWIPE_THRESHOLD
                && abs(TouchPanel.mTouchDownPosition.PositionY - TouchPanel.mCurrentTouchPosition.PositionY) < TOUCH_SWIPE_THRESHOLD) {
            // fill up event
            localTouchEvent.EventData.TouchEventInfo.TouchPosition = TouchPanel.mLastTouchPosition;
            localTouchEvent.EventType = EVENT_LONG_TOUCH_DOWN_CALLBACK;
            /*
             * Disable next touch up handling, since we already have a valid event
             */
            sDisableTouchUpOnce = true;
        }
    }
}
#endif // defined(USE_TIMER_FOR_PERIODIC_LOCAL_TOUCH_CHECKS)

#if defined(LOCAL_DISPLAY_GENERATES_BD_EVENTS)

/**
 * If touch down was not on buttons or slider, performs swipe recognition and swipe or up BlueDisplay event generation.
 */
void handleLocalTouchUp() {
    /*
     * First, disable long touch down detection
     */
    changeDelayCallback(&callbackHandlerForLongTouchDownTimeout, DISABLE_TIMER_DELAY_VALUE); // enable timeout

    if(!sDisableTouchUpOnce){
        if (sTouchObjectTouched == PANEL_TOUCHED) {
            uint16_t tTouchDeltaX = TouchPanel.mLastTouchPosition.PositionX - TouchPanel.mTouchDownPosition.PositionX;
            uint16_t tTouchDeltaXAbs = abs(tTouchDeltaX);
            uint16_t tTouchDeltaY = TouchPanel.mLastTouchPosition.PositionY - TouchPanel.mTouchDownPosition.PositionY;
            uint16_t tTouchDeltaYAbs = abs(tTouchDeltaY);

            if (sSwipeEndCallbackEnabled && (tTouchDeltaXAbs >= TOUCH_SWIPE_THRESHOLD || tTouchDeltaYAbs >= TOUCH_SWIPE_THRESHOLD)) {
                /*
                 * Swipe recognized here
                 * compute SWIPE data and generate local BlueDisplay swipe event
                 */
                localTouchEvent.EventData.SwipeInfo.TouchStartX = TouchPanel.mLastTouchPosition.PositionX;
                localTouchEvent.EventData.SwipeInfo.TouchStartY = TouchPanel.mLastTouchPosition.PositionY;
                localTouchEvent.EventData.SwipeInfo.TouchDeltaX = tTouchDeltaX;
                localTouchEvent.EventData.SwipeInfo.TouchDeltaY = tTouchDeltaY;
                if (tTouchDeltaXAbs >= tTouchDeltaYAbs) {
                    // X direction
                    localTouchEvent.EventData.SwipeInfo.SwipeMainDirectionIsX = true;
                } else {
                    localTouchEvent.EventData.SwipeInfo.SwipeMainDirectionIsX = false;
                }
                localTouchEvent.EventType = EVENT_SWIPE_CALLBACK;
            } else {
                // Generate touch up event if no button or slider was touched
                localTouchEvent.EventData.TouchEventInfo.TouchPosition = TouchPanel.mLastTouchPosition; // Current position is already invalid here
                localTouchEvent.EventData.TouchEventInfo.TouchPointerIndex = 0;
                localTouchEvent.EventType = EVENT_TOUCH_ACTION_UP;
            }
        }
    }
    sDisableTouchUpOnce = false;
}
#endif // defined(LOCAL_DISPLAY_GENERATES_BD_EVENTS)
#endif //_LOCAL_EVENT_HELPER_HPP
