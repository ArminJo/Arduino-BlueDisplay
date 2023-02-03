/*
 * BlueSerial.h
 *
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
 */

#ifndef _BLUESERIAL_H
#define _BLUESERIAL_H

#if defined(ARDUINO)
#include <Arduino.h> // for UBRR1H + BOARD_HAVE_USART1 + SERIAL_USB
#elif defined(STM32F10X)
#include <stm32f1xx.h>
#include <stddef.h>
#elif defined(STM32F30X)
#include <stm32f3xx.h>
#include "stm32f3xx_hal_conf.h" // for UART_HandleTypeDef
#include <stddef.h>
#endif

/*
 * Simple serial is a simple blocking serial version without receive buffer and other overhead.
 * Simple serial on the MEGA2560 uses USART1
 */
//#define USE_SIMPLE_SERIAL // Do not use the Serial object. Saves up to 1250 bytes program memory and 185 bytes RAM, if Serial is not used otherwise

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
 * Common functions which are independent of using simple or standard serial
 */
/*
 * Checks if additional remote display is paired to avoid program slow down by UART sending to a not paired connection
 *
 * USART_isBluetoothPaired() is only required if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
 *   to disable sending by Bluetooth if BT is not connected.
 * It is reduced to return false if defined(DISABLE_REMOTE_DISPLAY)
 * It is reduced to return true if not defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
 */
bool USART_isBluetoothPaired(void);
#if !defined(PAIRED_PIN) && defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
#define PAIRED_PIN 5
#endif
void setUsePairedPin(bool aUsePairedPin);

void sendUSARTArgs(uint8_t aFunctionTag, uint_fast8_t aNumberOfArgs, ...);
void sendUSARTArgsAndByteBuffer(uint8_t aFunctionTag, uint_fast8_t aNumberOfArgs, ...);
void sendUSART5Args(uint8_t aFunctionTag, uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor);
// used internal by the above functions
void sendUSARTBufferNoSizeCheck(uint8_t *aParameterBufferPointer, uint8_t aParameterBufferLength, uint8_t *aDataBufferPointer,
        size_t aDataBufferLength);

/*
 * Functions only valid for standard serial
 */
#if !defined(USE_SIMPLE_SERIAL)
uint8_t getReceiveBufferByte(void);
size_t getReceiveBytesAvailable(void);
void serialEvent(void); // Is called by Arduino runtime
#endif

#if defined(ARDUINO)
/*
 * Functions which depends on using simple or standard serial on AVR
 */
void initSerial();

#if defined(ESP32)
void initSerial(String aBTClientName);
#else
void initSerial(uint32_t aBaudRate);
#endif

void sendUSART(char aChar);
void sendUSART(const char *aString);
#endif

/*
 * Functions only valid for simple serial
 */
#if defined(USE_SIMPLE_SERIAL)
void initSimpleSerial(uint32_t aBaudRate);
void initSimpleSerial(uint32_t aBaudRate, bool aUsePairedPin);
#endif

#if defined(STM32F303xC) || defined(STM32F103xB)
/*
 * Stuff for STM
 */
#ifdef STM32F303xC
#define UART_BD                     USART3
#define UART_BD_TX_PIN              GPIO_PIN_10
#define UART_BD_RX_PIN              GPIO_PIN_11
#define UART_BD_PORT                GPIOC
#define UART_BD_IO_CLOCK_ENABLE()   __GPIOC_CLK_ENABLE()
#define UART_BD_CLOCK_ENABLE()      __USART3_CLK_ENABLE()
#define UART_BD_IRQ                 USART3_IRQn
#define UART_BD_IRQHANDLER          USART3_IRQHandler

#define UART_BD_DMA_TX_CHANNEL      DMA1_Channel2
#define UART_BD_DMA_RX_CHANNEL      DMA1_Channel3
#define UART_BD_DMA_CLOCK_ENABLE()  __DMA1_CLK_ENABLE()

#define BLUETOOTH_PAIRED_DETECT_PIN     GPIO_PIN_13
#define BLUETOOTH_PAIRED_DETECT_PORT    GPIOC
#endif

#ifdef STM32F103xB
#define UART_BD                     USART1
#define UART_BD_TX_PIN              GPIO_PIN_9
#define UART_BD_RX_PIN              GPIO_PIN_10
#define UART_BD_PORT                GPIOA
#define UART_BD_IO_CLOCK_ENABLE()   __GPIOA_CLK_ENABLE()
#define UART_BD_CLOCK_ENABLE()      __USART1_CLK_ENABLE()
#define UART_BD_IRQ                 USART1_IRQn
#define UART_BD_IRQHANDLER          USART1_IRQHandler

#define UART_BD_DMA_TX_CHANNEL      DMA1_Channel4
#define UART_BD_DMA_RX_CHANNEL      DMA1_Channel5
#define UART_BD_DMA_CLOCK_ENABLE()  __DMA1_CLK_ENABLE()

#define BLUETOOTH_PAIRED_DETECT_PIN     GPIO_PIN_7
#define BLUETOOTH_PAIRED_DETECT_PORT    GPIOA
#endif

// The UART used by BlueDisplay
extern UART_HandleTypeDef UART_BD_Handle;
#ifdef __cplusplus
extern "C" {
#endif
void UART_BD_IRQHANDLER(void);
#ifdef __cplusplus
}
#endif

// Send functions using buffer and DMA
int getSendBufferFreeSpace(void);

void UART_BD_initialize(uint32_t aBaudRate);
void HAL_UART_MspInit(UART_HandleTypeDef* aUARTHandle);

uint32_t getUSART_BD_BaudRate(void);
void setUART_BD_BaudRate(uint32_t aBaudRate);

// Simple blocking serial version without overhead
void sendUSARTBufferSimple(uint8_t * aParameterBufferPointer, size_t aParameterBufferLength,
        uint8_t * aDataBufferPointer, size_t aDataBufferLength);

#endif // defined(AVR)
#endif // _BLUESERIAL_H
