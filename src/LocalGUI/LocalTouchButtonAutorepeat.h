/*
 * LocalTouchButtonAutorepeat.h
 *
 * Extension of ToucButton
 * implements autorepeat feature for touch buttons
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

#ifndef _TOUCHBUTTON_AUTOREPEAT_H
#define _TOUCHBUTTON_AUTOREPEAT_H

#include "LocalGUI/LocalTouchButton.h"

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Button
 * @{
 */

class LocalTouchButtonAutorepeat: public LocalTouchButton {
public:

    LocalTouchButtonAutorepeat();

#if !defined(ARDUINO)
    ~LocalTouchButtonAutorepeat();
#endif

#if !defined(DISABLE_REMOTE_DISPLAY)
    LocalTouchButtonAutorepeat(BDButton *aBDButtonPtr);
#endif

    /*
     * Static functions
     */
    static int checkAllButtons(int aTouchPositionX, int aTouchPositionY, bool doCallback);
    static void autorepeatTouchHandler(LocalTouchButtonAutorepeat *aTheTouchedButton, int16_t aButtonValue);
    static void autorepeatButtonTimerHandler(int aTouchPositionX, int aTouchPositionY);
    void setButtonAutorepeatTiming(uint16_t aMillisFirstDelay, uint16_t aMillisFirstRate, uint16_t aFirstCount,
            uint16_t aMillisSecondRate);
    static void disableAutorepeatUntilEndOfTouch();

    /*
     * The autorepeat characteristic of this button
     */
    uint16_t mMillisFirstDelay;
    uint16_t mMillisFirstRate;
    uint16_t mFirstCount;
    uint16_t mMillisSecondRate;

    void (*mOriginalButtonOnTouchHandler)(LocalTouchButton*, int16_t);

    // Static values for currently touched/active button because only one touch to any autorepeat button possible
    static uint8_t sState;
    static uint16_t sCount;
    static unsigned long sCurrentCallbackDelayMillis; // The current period delay value. Can be mMillisFirstDelay, mMillisFirstRate or mMillisSecondRate
    static unsigned long sMillisOfLastCallOfCallback; // millis() value of last call to the buttons callback function

};
/** @} */
/** @} */
#endif // _TOUCHBUTTON_AUTOREPEAT_H
