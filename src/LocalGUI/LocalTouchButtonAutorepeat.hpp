/*
 * TouchButtonAutorepeat.hpp
 *
 * Extension of TouchButton
 * implements autorepeat feature for touch buttons
 *
 * 52 Byte per autorepeat button on 32Bit ARM
 *
 *  Copyright (C) 2012-2023  Armin Joachimsmeyer
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

#ifndef _TOUCHBUTTON_AUTOREPEAT_HPP
#define _TOUCHBUTTON_AUTOREPEAT_HPP
#include "LocalGUI/LocalTouchButtonAutorepeat.h"

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Button
 * @{
 */
#define AUTOREPEAT_BUTTON_STATE_AFTER_FIRST_DELAY   0
#define AUTOREPEAT_BUTTON_STATE_FIRST_PERIOD        1
#define AUTOREPEAT_BUTTON_STATE_SECOND_PERIOD       3

uint8_t TouchButtonAutorepeat::sState;
uint16_t TouchButtonAutorepeat::sCount;

#if defined(AUTOREPEAT_BY_USING_LOCAL_EVENT)
TouchButtonAutorepeat *TouchButtonAutorepeat::sLastAutorepeatButtonTouched = NULL; // Pointer to currently touched/active button for timer callback
#else
unsigned long TouchButtonAutorepeat::sCurrentCallbackDelayMillis;
unsigned long TouchButtonAutorepeat::sMillisOfLastCallOfCallback;
extern bool isFirstTouch(); // must be provided by main program
#endif

TouchButtonAutorepeat::TouchButtonAutorepeat() { // @suppress("Class members should be properly initialized")
    // List handling is done in the base class
}

#if !defined(ARDUINO)
TouchButtonAutorepeat::~TouchButtonAutorepeat() {
    // List handling is done in the base class
}
#endif

#if !defined(DISABLE_REMOTE_DISPLAY)
/*
 * Required for creating a local autorepeat button for an existing aBDButton at BDButton::init
 */
TouchButtonAutorepeat::TouchButtonAutorepeat(BDButton *aBDButtonPtr) : // @suppress("Class members should be properly initialized")
        TouchButton(aBDButtonPtr) {
}
#endif
/**
 * after aMillisFirstDelay milliseconds a callback is done every aMillisFirstRate milliseconds for aFirstCount times
 * after this a callback is done every aMillisSecondRate milliseconds
 */
void TouchButtonAutorepeat::setButtonAutorepeatTiming(uint16_t aMillisFirstDelay, uint16_t aMillisFirstRate, uint16_t aFirstCount,
        uint16_t aMillisSecondRate) {
    mMillisFirstDelay = aMillisFirstDelay;
    mMillisFirstRate = aMillisFirstRate;
    if (aFirstCount < 1) {
        aFirstCount = 1;
    }
    mFirstCount = aFirstCount;
    mMillisSecondRate = aMillisSecondRate;
    // do it here since flags are initialized by init()
    mFlags |= FLAG_BUTTON_TYPE_AUTOREPEAT;

    // Replace standard button handler with autorepeatTouchHandler
    mOriginalButtonOnTouchHandler = mOnTouchHandler;
    mOnTouchHandler = ((void (*)(TouchButton*, int16_t)) &autorepeatTouchHandler);
}

/**
 * Touch handler for button object called at touch down, which in turn implements the autorepeat functionality
 * by enabling the autorepeatButtonTimerHandler to be called with the initial delay
 */
void TouchButtonAutorepeat::autorepeatTouchHandler(TouchButtonAutorepeat *aTheTouchedButton, int16_t aButtonValue) {
#if !defined(AUTOREPEAT_BY_USING_LOCAL_EVENT)
    /*
     * Autorepeat is implemented by periodic calls of checkAllButtons() by main loop, which ends up here
     * We know that touch is still in the button area
     */
    bool tDoCallback = false;
#if defined(TRACE)
    Serial.print(F("First="));
    Serial.print(isFirstTouch());
    Serial.print(F(" State="));
    Serial.println(sState);
#endif
    /*
     * Check if we are called initially on touch down
     */
    if(isFirstTouch()){
        tDoCallback = true;
        sState = AUTOREPEAT_BUTTON_STATE_AFTER_FIRST_DELAY;
        sCurrentCallbackDelayMillis = aTheTouchedButton->mMillisFirstDelay;
        sCount =aTheTouchedButton-> mFirstCount;

    } else if (millis() - sMillisOfLastCallOfCallback  > sCurrentCallbackDelayMillis) {
        /*
         * Period has expired
         */
        switch (sState) {
        case AUTOREPEAT_BUTTON_STATE_AFTER_FIRST_DELAY:
            // register delay for first autorepeat rate
            sCurrentCallbackDelayMillis = aTheTouchedButton->mMillisFirstRate;
            sState = AUTOREPEAT_BUTTON_STATE_FIRST_PERIOD;
            sCount--;
            break;
        case AUTOREPEAT_BUTTON_STATE_FIRST_PERIOD:
            if (sCount == 0) {
                // register  delay for second and last autorepeat rate
                sCurrentCallbackDelayMillis = aTheTouchedButton->mMillisSecondRate;
                sState = AUTOREPEAT_BUTTON_STATE_SECOND_PERIOD;
            } else {
                sCount--;
            }
            break;
        case AUTOREPEAT_BUTTON_STATE_SECOND_PERIOD:
            // here we stay as long as button is pressed
            sCount++; // just for fun
            break;
        }
        tDoCallback = true;
    }

    if (tDoCallback) {
#if defined(GUI_FEEDBACK_TONE_PIN)
        if (mFlags & FLAG_BUTTON_DO_BEEP_ON_TOUCH) {
            tone(GUI_FEEDBACK_TONE_PIN, 3000, 50);
        }
#endif

#  if defined(DISABLE_REMOTE_DISPLAY)
        aTheTouchedButton->mOriginalButtonOnTouchHandler(aTheTouchedButton, aButtonValue);
#  else // SUPPORT_REMOTE_AND_LOCAL_DISPLAY is defined here
        if (aTheTouchedButton->mFlags & LOCAL_BUTTON_FLAG_USE_BDBUTTON_FOR_CALLBACK) {
            aTheTouchedButton->mOriginalButtonOnTouchHandler((TouchButton*) aTheTouchedButton->mBDButtonPtr, aButtonValue);
        } else {
            aTheTouchedButton->mOriginalButtonOnTouchHandler(aTheTouchedButton, aButtonValue);
        }
#  endif
        sMillisOfLastCallOfCallback = millis();
    }
#else
    /*
     * Autorepeat is implemented by timer call
     */
    // Touch down goes here, to enable autorepeat, we register delay callback
    sLastAutorepeatButtonTouched = aTheTouchedButton;
    registerPeriodicTouchCallback(&TouchButtonAutorepeat::autorepeatButtonTimerHandler, aTheTouchedButton->mMillisFirstDelay);

#if defined(GUI_FEEDBACK_TONE_PIN)
    if (mFlags & FLAG_BUTTON_DO_BEEP_ON_TOUCH) {
        tone(GUI_FEEDBACK_TONE_PIN, 3000, 50);
    }
#endif

#  if defined(DISABLE_REMOTE_DISPLAY)
    aTheTouchedButton->mOriginalButtonOnTouchHandler(aTheTouchedButton, aButtonValue);
#  else
    if (aTheTouchedButton->mFlags & LOCAL_BUTTON_FLAG_USE_BDBUTTON_FOR_CALLBACK) {
        aTheTouchedButton->mOriginalButtonOnTouchHandler((TouchButton*) aTheTouchedButton->mBDButtonPtr, aButtonValue);
    } else {
        aTheTouchedButton->mOriginalButtonOnTouchHandler(aTheTouchedButton, aButtonValue);
    }
#  endif
    sState = AUTOREPEAT_BUTTON_STATE_AFTER_FIRST_DELAY;
    sCount = aTheTouchedButton->mFirstCount;
#endif

}

#if defined(AUTOREPEAT_BY_USING_LOCAL_EVENT)

/**
 * Callback function to be called by EventHandler after the first delay if touch is still active
 */
void TouchButtonAutorepeat::autorepeatButtonTimerHandler(int aTouchPositionX, int aTouchPositionY) {
    TouchButtonAutorepeat *tTouchedButton = sLastAutorepeatButtonTouched;
    /*
     * Check if touch still in button area
     */
    if (tTouchedButton == NULL || !tTouchedButton->checkButtonInArea(aTouchPositionX, aTouchPositionY)) {
        return;
    }
    switch (sState) {
    case AUTOREPEAT_BUTTON_STATE_AFTER_FIRST_DELAY:
        // register delay for first autorepeat rate
        setPeriodicTouchCallbackPeriod(tTouchedButton->mMillisFirstRate);
        sState = AUTOREPEAT_BUTTON_STATE_FIRST_PERIOD;
        sCount--;
        break;
    case AUTOREPEAT_BUTTON_STATE_FIRST_PERIOD:
        if (sCount == 0) {
            // register  delay for second and last autorepeat rate
            setPeriodicTouchCallbackPeriod(tTouchedButton->mMillisSecondRate);
            sState = AUTOREPEAT_BUTTON_STATE_SECOND_PERIOD;
        } else {
            sCount--;
        }
        break;
    case AUTOREPEAT_BUTTON_STATE_SECOND_PERIOD:
        // here we stay as long as button is pressed
        sCount++; // just for fun
        break;
    }

#  if defined(DISABLE_REMOTE_DISPLAY)
        tTouchedButton->mOriginalButtonOnTouchHandler(tTouchedButton, tTouchedButton->mValue);
#  else
    if (tTouchedButton->mFlags & LOCAL_BUTTON_FLAG_USE_BDBUTTON_FOR_CALLBACK) {
        tTouchedButton->mOriginalButtonOnTouchHandler((TouchButton*) tTouchedButton->mBDButtonPtr, tTouchedButton->mValue);
    } else {
        tTouchedButton->mOriginalButtonOnTouchHandler(tTouchedButton, tTouchedButton->mValue);
    }
#  endif
}
#endif // defined(AUTOREPEAT_BY_USING_LOCAL_EVENT)

/** @} */
/** @} */
#endif // _TOUCHBUTTON_AUTOREPEAT_HPP
