/*
 * LocalTouchButtonAutorepeat.hpp
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
#define AUTOREPEAT_BUTTON_STATE_DISABLED_UNTIL_END_OF_TOUCH 4

uint8_t LocalTouchButtonAutorepeat::sState;
uint16_t LocalTouchButtonAutorepeat::sCount;

unsigned long LocalTouchButtonAutorepeat::sCurrentCallbackDelayMillis;
unsigned long LocalTouchButtonAutorepeat::sMillisOfLastCallOfCallback;
extern bool isFirstTouchOfButtonOrSlider(); // must be provided by main program

LocalTouchButtonAutorepeat::LocalTouchButtonAutorepeat() { // @suppress("Class members should be properly initialized")
    // List handling is done in the base class
}

#if !defined(ARDUINO)
LocalTouchButtonAutorepeat::~LocalTouchButtonAutorepeat() {
    // List handling is done in the base class
}
#endif

#if !defined(DISABLE_REMOTE_DISPLAY)
/*
 * Required for creating a local autorepeat button for an existing aBDButton at BDButton::init
 */
LocalTouchButtonAutorepeat::LocalTouchButtonAutorepeat(BDButton *aBDButtonPtr) : // @suppress("Class members should be properly initialized")
        LocalTouchButton(aBDButtonPtr) {
}
#endif
/**
 * after aMillisFirstDelay milliseconds a callback is done every aMillisFirstRate milliseconds for aFirstCount times
 * after this a callback is done every aMillisSecondRate milliseconds
 */
void LocalTouchButtonAutorepeat::setButtonAutorepeatTiming(uint16_t aMillisFirstDelay, uint16_t aMillisFirstRate,
        uint16_t aFirstCount, uint16_t aMillisSecondRate) {
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
    mOnTouchHandler = ((void (*)(LocalTouchButton*, int16_t)) &autorepeatTouchHandler);
}

void LocalTouchButtonAutorepeat::disableAutorepeatUntilEndOfTouch() {
    sState = AUTOREPEAT_BUTTON_STATE_DISABLED_UNTIL_END_OF_TOUCH;
}

/**
 * Touch handler for button object called at touch down, which in turn implements the autorepeat functionality
 * by enabling the autorepeatButtonTimerHandler to be called with the initial delay
 */
void LocalTouchButtonAutorepeat::autorepeatTouchHandler(LocalTouchButtonAutorepeat *aTheTouchedButton, int16_t aButtonValue) {
    (void)aButtonValue; // not required here
    /*
     * Autorepeat is implemented by periodic calls of checkAllButtons() by main loop, which ends up here
     * We know that touch is still in the button area
     */
    bool tDoCallback = false;
#  if defined(TRACE)
    Serial.print(F("First="));
    Serial.print(isFirstTouchOfButtonOrSlider());
    Serial.print(F(" State="));
    Serial.println(sState);
#  endif
    /*
     * Check if we are called initially on touch down
     * sTouchPanelButtonOrSliderTouched is set to true just after the successful call of checkAllButtons()
     * which means that it is false at the time of first call of checkAllButtons() which in turn calls autorepeatTouchHandler().
     */
    if (sTouchObjectTouched == NO_TOUCH) {
        sState = AUTOREPEAT_BUTTON_STATE_AFTER_FIRST_DELAY;
        sCurrentCallbackDelayMillis = aTheTouchedButton->mMillisFirstDelay;
        sCount = aTheTouchedButton->mFirstCount;
        tDoCallback = true;

    } else if (sState != AUTOREPEAT_BUTTON_STATE_DISABLED_UNTIL_END_OF_TOUCH
            && (millis() - sMillisOfLastCallOfCallback > sCurrentCallbackDelayMillis)) {
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
        if (aTheTouchedButton->mFlags & FLAG_BUTTON_DO_BEEP_ON_TOUCH) {
            playFeedbackTone();
        }

#if defined(DISABLE_REMOTE_DISPLAY)
        aTheTouchedButton->mOriginalButtonOnTouchHandler(aTheTouchedButton, aTheTouchedButton->mValue);
#else // SUPPORT_REMOTE_AND_LOCAL_DISPLAY is defined here
        aTheTouchedButton->mOriginalButtonOnTouchHandler((LocalTouchButton*) aTheTouchedButton->mBDButtonPtr, aTheTouchedButton->mValue);
#endif
        sMillisOfLastCallOfCallback = millis();
    }
}

/** @} */
/** @} */
#endif // _TOUCHBUTTON_AUTOREPEAT_HPP
