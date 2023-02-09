/*
 * LocalDisplay.hpp
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
 *
 */
#ifndef _LOCAL_DISPLAY_HPP
#define _LOCAL_DISPLAY_HPP

#define USE_HX8347D
#include "LocalDisplay/fonts.hpp"
#include "LocalDisplay/LocalDisplayInterface.hpp" // The implementation of the local display must be included first since it defines LOCAL_DISPLAY_HEIGHT etc.
#include "LocalDisplay/ADS7846.hpp"     // Must be after the local display implementation since it uses e.g. LOCAL_DISPLAY_HEIGHT
#include "LocalDisplay/LocalEventHelper.hpp"

#if defined(SUPPORT_LOCAL_LONG_TOUCH_DOWN_DETECTION) || defined(LOCAL_DISPLAY_GENERATES_BD_EVENTS)
#include "EventHandler.hpp"
#endif

#endif // _LOCAL_DISPLAY_HPP
