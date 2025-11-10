/*
 *  CO2LoggerAndChart.h
 *
 *
 *  Copyright (C) 2024-2025  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of Arduino-BlueDisplay https://github.com/ArminJo/Arduino-BlueDisplay.
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

 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#ifndef _CO2_LOGGER_AND_CHART_H
#define _CO2_LOGGER_AND_CHART_H

#include <stdint.h>
#include "TimeLib.h" // for useful macros like SECS_PER_MIN
#include "BlueDisplay.h"

#define CHART_BACKGROUND_COLOR  COLOR16_WHITE
#define CHART_DATA_COLOR        COLOR16_RED
#define CHART_AXES_COLOR        COLOR16_BLUE
#define CHART_GRID_COLOR        COLOR16_GREEN
#define CHART_DATA_COLOR        COLOR16_RED
#define CHART_TEXT_COLOR        COLOR16_BLACK

#define CO2_BASE_VALUE          400L
#define CO2_COMPRESSION_FACTOR    5L // value 1 -> 405, 2 -> 410 etc.
/*
 * Even with initDisplay() called from main loop we only have 1140 bytes available for application
 */
#define CO2_ARRAY_SIZE  1152L // 0x480 1152-> 4 days, 1440->5 days at 5 minutes / sample
#define NUMBER_OF_DAYS_IN_BUFFER    4

#define TIME_ADJUSTMENT 0
//#define TIME_ADJUSTMENT SECS_PER_HOUR // To be subtracted from received timestamp
#define MILLIS_IN_ONE_SECOND 1000L
// sNextStorageMillis is required by CO2LoggerAndChart.hpp for toast at connection startup
extern uint32_t sNextStorageMillis; // If not connected first storage in 1 after boot.

#define BRIGHTNESS_LOW      2
#define BRIGHTNESS_MIDDLE   1
#define BRIGHTNESS_HIGH     0
#define START_BRIGHTNESS    BRIGHTNESS_HIGH
extern uint8_t sCurrentBrightness;

/*
 * Is intended to be called by setup
 */
void InitCo2LoggerAndChart();

/*
 * Is intended to be called by main loop
 */
bool storeCO2ValuePeriodically(uint16_t aCO2Value, const uint32_t aStoragePeriodMillis);

/*
 * Use this instead of delay(), this checks the BlueDisplay communication and makes the program reactive
 */
void delayMillisWithHandleEventAndFlags(unsigned long aDelayMillis);
/*
 * End prematurely, if event received
 */
bool delayMillisWithCheckForEventAndFlags(unsigned long aDelayMillis);
/*
 * Checks for BlueDisplay events and returns true if event happened,
 * which may introduce an unknown delay the program might not be able to handle
 */
bool handleEventAndFlags();

/***********************
 * Internal functions
 **********************/
/*
 * Array is in section .noinit
 * Therefore check checksum before initializing it after reboot
 */
void initializeCO2Array();

/*
 * aCO2Value is (CO2[ppm] - 400) / 5
 */
void writeToCO2Array(uint8_t aCO2Value);

/*
 * This function is not called by event callback, it is called from main loop
 * signalInitDisplay() is called by event callback, which only sets a flag for the main loop.
 * This helps reducing stack usage,
 * !!!but BlueDisplay1.isConnectionEstablished() cannot be used as indicator for initialized BD data, without further handling!!!
 */
void initDisplay(void);
void drawDisplay();
void initCO2Chart();

/*
 * Print difference between next storage time and now(), which is set by caller :-)
 * Serial output is displayed as an Android toast
 */
void printTimeToNextStorage();
void changeBrightness();
void drawCO2Chart();
void printCO2Value();
void printTime();
#endif // _CO2_LOGGER_AND_CHART_H
