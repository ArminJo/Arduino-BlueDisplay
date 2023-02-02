/*
 * LocalGUI.hpp
 *
 * Helper for Arduino library detection phase to avoid to have an include of
 * type LocalGUI/LocalTouchButton.hpp in the sketch, which will not be found in this phase.
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
#ifndef _LOCAL_GUI_HPP
#define _LOCAL_GUI_HPP

#if !defined(DISABLE_REMOTE_DISPLAY)
#error Remote display is not disabled by #define DISABLE_REMOTE_DISPLAY and #include "LocalGUI.hpp" is used. This is most likely an error. You have to use #include "BlueDisplay.hpp" instead, it in turn includes the local GUI!
#endif

#include "GUIHelper.hpp"        // Must be included before LocalGUI/*. For TEXT_SIZE_11, getLocalTextSize() etc.
#include "LocalGUI/LocalTouchButton.hpp"
#include "LocalGUI/LocalTouchSlider.hpp"
#include "LocalGUI/LocalTouchButtonAutorepeat.hpp"
#include "LocalGUI/LocalTinyPrint.hpp"
#include "LocalGUI/ThickLine.hpp"

#endif // _LOCAL_GUI_HPP
