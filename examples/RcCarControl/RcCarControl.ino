/*
 *  RcCarControl.cpp
 *  Demo of using the BlueDisplay library for HC-05 on Arduino

 *  Copyright (C) 2015  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay.
 *  BlueDisplay is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#include <Arduino.h>

#include "RcCarControlBD.h"

// Pin 13 has an LED connected on most Arduino boards.
const int LED_PIN = 13;

/*******************************************************************************************
 * Program code starts here
 *******************************************************************************************/

void setup() {
    BDsetup();
}

void loop() {
    BDloop();
}
