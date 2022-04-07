/*
 * BlueSerial.h
 *
 *  SUMMARY
 *  Blue Display is an Open Source Android remote Display for Arduino etc.
 *  It receives basic draw requests from Arduino etc. over Bluetooth and renders it.
 *  It also implements basic GUI elements as buttons and sliders.
 *  GUI callback, touch and sensor events are sent back to Arduino.
 *
 *  Copyright (C) 2014-2020  Armin Joachimsmeyer
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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#ifndef _BLUESERIAL_H
#define _BLUESERIAL_H

#if defined(ARDUINO)
#include <Arduino.h> // for UBRR1H + BOARD_HAVE_USART1 + SERIAL_USB
#else
#include <stddef.h>
#endif

#if defined(__AVR__)
/*
 * Simple serial is a simple blocking serial version without receive buffer and other overhead.
 * Simple serial on the MEGA2560 uses USART1
 */
//#define USE_SIMPLE_SERIAL // Do not use the Serial object. Saves up to 1250 bytes program memory and 185 bytes RAM, if Serial is not used otherwise
#endif

#if defined(SERIAL_PORT_HARDWARE1) // is defined for Arduino Due
#define BOARD_HAVE_USART2 // they start counting with 0
#endif

// If Serial1 is available, but you want to use direct connection by USB to your smartphone / tablet,
// then you have to activate the next line
//#define USE_USB_SERIAL

/*
 * Determine which serial to use.
 * - Use standard Serial if USE_USB_SERIAL is requested.
 * - Prefer the use of second USART, to have the standard Serial available for application (debug) use except for ATmega328P and PB.
 * - Use Serial1 on stm32 if SERIAL_USB and USART1 is existent. If no SERIAL_USB existent, it requires USART2 to have Serial1 available.
 * - Use Serial1 on AVR if second USART is existent, as on the ATmega Boards.
 */
// In some cores for ATmega328PB only ATmega328P is defined
#if ! defined(USE_USB_SERIAL) && ! defined(__AVR_ATmega328P__) && ! defined(__AVR_ATmega328PB__) && ((defined(BOARD_HAVE_USART1) && defined(SERIAL_USB)) \
    || (defined(BOARD_HAVE_USART2) && ! defined(SERIAL_USB)) \
    || defined(UBRR1H))
#define USE_SERIAL1
#endif

#define BAUD_STRING_4800 "4800"
#define BAUD_STRING_9600 "9600"
#define BAUD_STRING_19200 "19200"
#define BAUD_STRING_38400 "38400"
#define BAUD_STRING_57600 "57600"
#define BAUD_STRING_115200 "115200"
#define BAUD_STRING_230400 "230400"
#define BAUD_STRING_460800 "460800"
#define BAUD_STRING_921600 " 921600"
#define BAUD_STRING_1382400 "1382400"

#define BAUD_4800 (4800)
#define BAUD_9600 (9600)
#define BAUD_19200 (19200)
#define BAUD_38400 (38400)
#define BAUD_57600 (57600)
#define BAUD_115200 (115200)
#define BAUD_230400 (230400)
#define BAUD_460800 (460800)
#define BAUD_921600 ( 921600)
#define BAUD_1382400 (1382400)

/*
 * common functions
 */
void sendUSARTArgs(uint8_t aFunctionTag, uint_fast8_t aNumberOfArgs, ...);
void sendUSARTArgsAndByteBuffer(uint8_t aFunctionTag, uint_fast8_t aNumberOfArgs, ...);
void sendUSART5Args(uint8_t aFunctionTag, uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor);
void sendUSART5ArgsAndByteBuffer(uint8_t aFunctionTag, uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd,
        uint16_t aColor, uint8_t *aBufferPtr, size_t aBufferLength);
// used internal by the above functions
void sendUSARTBufferNoSizeCheck(uint8_t *aParameterBufferPointer, uint8_t aParameterBufferLength, uint8_t *aDataBufferPointer,
        int16_t aDataBufferLength);

#define PAIRED_PIN 5
bool USART_isBluetoothPaired(void); // is reduced to return true; if not defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)

#  if defined(SUPPORT_LOCAL_DISPLAY) && !defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
void initSimpleSerial(uint32_t aBaudRate, bool aUsePairedPin);
#  else
#    if defined(ESP32)
void initSerial(String aBTClientName);
#    else
void initSerial(uint32_t aBaudRate);
void initSerial();
void initSimpleSerial(uint32_t aBaudRate);
#   endif
#  endif // defined(SUPPORT_LOCAL_DISPLAY)

extern bool allowTouchInterrupts;
void sendUSART(char aChar);
void sendUSART(const char *aChar);
//void USART_send(char aChar);

void serialEvent();

#endif // _BLUESERIAL_H
#pragma once
