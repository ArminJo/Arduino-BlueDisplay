/*
 * BlueSerial.h
 *
 *  SUMMARY
 *  Blue Display is an Open Source Android remote Display for Arduino etc.
 *  It receives basic draw requests from Arduino etc. over Bluetooth and renders it.
 *  It also implements basic GUI elements as buttons and sliders.
 *  GUI callback, touch and sensor events are sent back to Arduino.
 *
 *  Copyright (C) 2014  Armin Joachimsmeyer
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#ifndef BLUESERIAL_H_
#define BLUESERIAL_H_

#if defined(ARDUINO)
#include <Arduino.h> // for UBRR1H + BOARD_HAVE_USART1 + SERIAL_USB
#else
#include <stddef.h>
#endif

//#define USE_STANDARD_SERIAL
#if !defined(USE_STANDARD_SERIAL) && defined (AVR)
// Simple serial is a simple blocking serial version without receive buffer and other overhead.
// Using it saves up to 1250 byte FLASH and 185 byte RAM since USART is used directly
// Simple serial on the MEGA2560 uses USART1
#define USE_SIMPLE_SERIAL
#endif

// If Serial1 is available, but you want to use direct connection by USB to your smartphone / tablet, then you have to comment out the next line
//#define USE_USB_SERIAL

/*
 * Determine which serial to use
 * Standard Serial if USE_USB_SERIAL is defined use
 * Serial1 on stm32 if SERIAL_USB and USART1 is existent. If no SERIAL_USB it needs USART2 to have Serial1.
 * Serial1 on AVR if second USART is existent
 */
#if ! defined(USE_USB_SERIAL) && ((defined(BOARD_HAVE_USART1) && defined(SERIAL_USB)) \
    || (defined(BOARD_HAVE_USART2) && ! defined(SERIAL_USB)) \
    || defined(UBRR1H))
#define USE_SERIAL1
#else
#define USE_SERIAL0
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
void sendUSARTArgs(uint8_t aFunctionTag, int aNumberOfArgs, ...);
void sendUSARTArgsAndByteBuffer(uint8_t aFunctionTag, int aNumberOfArgs, ...);
void sendUSART5Args(uint8_t aFunctionTag, uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor);
void sendUSART5ArgsAndByteBuffer(uint8_t aFunctionTag, uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd,
		uint16_t aColor, uint8_t * aBufferPtr, size_t aBufferLength);

#define PAIRED_PIN 5

#if defined(LOCAL_DISPLAY_EXISTS) && defined(REMOTE_DISPLAY_SUPPORTED)
bool USART_isBluetoothPaired(void);
#else
#if defined(LOCAL_DISPLAY_EXISTS)
void initSimpleSerial(uint32_t aBaudRate, bool aUsePairedPin);
#define USART_isBluetoothPaired() (false)
#else
void initSimpleSerial(uint32_t aBaudRate);
#define USART_isBluetoothPaired() (true)
#endif
#endif

extern bool allowTouchInterrupts;
void sendUSART(char aChar);
void sendUSART(const char * aChar);
//void USART_send(char aChar);

void serialEvent();

#endif /* BLUESERIAL_H_ */
