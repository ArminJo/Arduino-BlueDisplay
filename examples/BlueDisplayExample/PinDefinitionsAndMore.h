/*
 *  PinDefinitionsAndMore.h
 *
 *  Contains SERVOX_PIN definitions for ServoEasing examples for various platforms
 *  as well as includes and definitions for LED_BUILTIN
 *
 *  Copyright (C) 2020  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay https://github.com/ArminJo/ServoEasing.
 *
 *  IRMP is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

/*
 * Pin mapping table for different platforms
 *
 * Platform     TX          RX
 * -----------------------------
 * AVR           0           1
 * ESP8266       1 // TX     3 // RX
 * DUE          18 // TX1   19 // RX1
 * BluePill     A9         A10
 */

#if defined(ESP8266)
#define TONE_PIN         14 // labeled D5
#define ANALOG_INPUT_PIN 0
#define ECHO_IN_PIN      13 // labeled D7
#define TRIGGER_OUT_PIN  15 // labeled D8

#elif defined(ESP32)
#define tone(a,b,c) void() // no tone() available on ESP32
#define noTone(a) void()
#define ANALOG_INPUT_PIN A0 // 36/VP
#define ECHO_IN_PIN      26
#define TRIGGER_OUT_PIN  27

#elif defined(ARDUINO_ARCH_SAM)
#define tone(a,b,c) void() // no tone() available on ESP32
#define noTone(a) void()
#define ANALOG_INPUT_PIN A0
#define ECHO_IN_PIN      4
#define TRIGGER_OUT_PIN  5

#elif defined(STM32F1xx) || defined(__STM32F1__)
// BluePill in 2 flavors
// STM32F1xx is for "Generic STM32F1 series" from STM32 Boards from STM32 cores of Arduino Board manager
// __STM32F1__is for "Generic STM32F103C series" from STM32F1 Boards (STM32duino.com) of Arduino Board manager
#define TONE_PIN         2
#define ANALOG_INPUT_PIN PA0
#define ECHO_IN_PIN      PB0
#define TRIGGER_OUT_PIN  PB1

#else
#define ECHO_IN_PIN      4
#define TRIGGER_OUT_PIN  5
#define TONE_PIN         3 // must be 3 to be compatible with talkie
#define ANALOG_INPUT_PIN A0
#endif

// for ESP32 LED_BUILTIN is defined as: static const uint8_t LED_BUILTIN 2
#if !defined(LED_BUILTIN) && !defined(ESP32)
#define LED_BUILTIN PB1
#endif
// On the Zero and others we switch explicitly to SerialUSB
#if defined(ARDUINO_ARCH_SAMD)
#define Serial SerialUSB
// The Chinese SAMD21 M0-Mini clone has no led connected, if you connect it, it is on pin 24 like on the original board.
// Attention! D2 and D4 are reversed on these boards
//#undef LED_BUILTIN
//#define LED_BUILTIN 25 // Or choose pin 25, it is the RX pin, but active low.
#endif
