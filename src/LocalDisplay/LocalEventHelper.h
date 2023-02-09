/*
 * LocalEventHelper.h
 *
 * Support for event generation / handling of local touch events
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
 *
 */

#ifndef _LOCAL_EVENT_HELPER_H
#define _LOCAL_EVENT_HELPER_H

#include <stdint.h>

/*
 * Local touch generates a local touch down or touch up event for BlueDisplay event handler, if not on button or slider.
 * I.e. localTouchEvent is then filled with data.
 */
//#define LOCAL_DISPLAY_GENERATES_BD_EVENTS
/*
 * For basic touch detection
 */
#define NO_TOUCH                    0
#define BUTTON_TOUCHED              1
#define SLIDER_TOUCHED              2
#define PANEL_TOUCHED               3 // We have a touch down, but not on a touch object
extern uint8_t sTouchObjectTouched; // On touch down, this changes from NO_TOUCH to one of BUTTON_TOUCHED, SLIDER_TOUCHED, PANEL_TOUCHED

/*
 * Local long touch down detection
 */
#if defined(SUPPORT_LOCAL_LONG_TOUCH_DOWN_DETECTION)  && !defined(USE_TIMER_FOR_PERIODIC_LOCAL_TOUCH_CHECKS)
extern uint32_t sLastTouchDownMillis; // time of last touch down for detecting long touch down
#endif

// Local swipe detection
#define TOUCH_SWIPE_THRESHOLD 10  // threshold for swipe detection to suppress long touch handler calling
#define TOUCH_SWIPE_RESOLUTION_MILLIS 20
void handleLocalTouchUp(void);

/*
 * The event handler void handleTouchPanelEvents() can be called by main loop
 * or called by an ADS7846 interrupt and a periodic timer for detecting moves, long touch down and swipes.
 */
void checkAndHandleTouchPanelEvents();
void handleTouchPanelEvents();

#if defined(USE_TIMER_FOR_PERIODIC_LOCAL_TOUCH_CHECKS)

// for local autorepeat button or color picker
void registerPeriodicTouchCallback(bool (*aPeriodicTouchCallback)(int, int), uint32_t aCallbackPeriodMillis);
void setPeriodicTouchCallbackPeriod(uint32_t aCallbackPeriod);

void (*sPeriodicTouchCallback)(int, int) = NULL; // return parameter not yet used
void callbackHandlerForLongTouchDownTimeout(void);

uint32_t sPeriodicCallbackPeriodMillis;
#endif

#endif //_LOCAL_EVENT_HELPER_H
