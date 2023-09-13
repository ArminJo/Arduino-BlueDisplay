/*
 * BlueDisplayUtils.hpp
 *
 * C stub for the Android BlueDisplay app (and the local LCD screens with SSD1289 (on HY32D board) or HX8347 (on MI0283QT2 board) controller.
 *
 * It implements also a few display test functions.
 *
 *  SUMMARY
 *  Blue Display is an Open Source Android remote Display for Arduino etc.
 *  It receives basic draw requests from Arduino etc. over Bluetooth and renders it.
 *  It also implements basic GUI elements as buttons and sliders.
 *  GUI callback, touch and sensor events are sent back to Arduino.
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
#ifndef _BLUEDISPLAY_UTILS_HPP
#define _BLUEDISPLAY_UTILS_HPP

#include "BlueDisplayUtils.h"

#include "ADCUtils.hpp" // for getCPUTemperature() and getVCCVoltage()
/*
 * Show temperature and VCC voltage
 */
void printVCCAndTemperaturePeriodically(BlueDisplay &aBlueDisplay, uint16_t aXPos, uint16_t aYPos, uint16_t aFontSize, uint16_t aPeriodMillis) {
#if defined (AVR)
    static unsigned long sMillisOfLastVCCInfo = 0;
    uint32_t tMillis = millis();

    if ((tMillis - sMillisOfLastVCCInfo) >= aPeriodMillis) {
        sMillisOfLastVCCInfo = tMillis;

        char tDataBuffer[18];
        char tVCCString[6];
        char tTempString[6];

        float tTemp = getCPUTemperature();
        dtostrf(tTemp, 4, 1, tTempString);

        float tVCCvoltage = getVCCVoltage();
        dtostrf(tVCCvoltage, 4, 2, tVCCString);

        sprintf_P(tDataBuffer, PSTR("%s volt %s\xB0" "C"), tVCCString, tTempString); // \xB0 is degree character
        aBlueDisplay.drawText(aXPos, aYPos, tDataBuffer, aFontSize, COLOR16_BLACK, COLOR16_WHITE);
    }
#else
    // No dtostrf() and no getCPUTemperature() and getVCCVoltage()
    (void)aBlueDisplay;
    (void)aXPos;
    (void)aYPos;
    (void)aFontSize;
    (void)aPeriodMillis;
#endif
}

#endif // _BLUEDISPLAY_UTILS_HPP
