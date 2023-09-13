/*
 * BlueDisplayUtils.h
 *
 *  Convenience functions using BlueDisplay
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

#ifndef _BLUEDISPLAY_UTILS_H
#define _BLUEDISPLAY_UTILS_H

#include "BlueDisplay.h"

void printVCCAndTemperaturePeriodically(BlueDisplay &aBlueDisplay, uint16_t aXPos, uint16_t aYPos, uint16_t aFontSize, uint16_t aPeriodMillis);

#endif // _BLUEDISPLAY_UTILS_H
