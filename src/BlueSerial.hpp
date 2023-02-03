/*
 * BlueSerial.hpp
 *
 * Implements the "simpleSerial" low level serial functions for communication with the Android BlueDisplay app.
 * Includes also common serial functions.
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
 *
 */

#ifndef _BLUESERIAL_HPP
#define _BLUESERIAL_HPP

#include "BlueDisplay.h"

#if !defined(va_start)
#include <cstdarg> // for va_start, va_list etc.
#endif

#if defined(ESP32)
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;
#define SERIAL_REDEFINED
#define Serial SerialBT // use SerialBT object instead of Serial object throughout this file
#endif
/****************************************************************************
 *
 * Common functions which are independent of using simple or standard serial
 *
 ****************************************************************************/
/*
 * Receive buffer
 */
#define RECEIVE_TOUCH_OR_DISPLAY_DATA_SIZE 4
//Buffer for 12 bytes since no need for length and eventType and SYNC_TOKEN be stored
uint8_t sReceiveBuffer[RECEIVE_MAX_DATA_SIZE];
uint8_t sReceiveBufferIndex = 0; // Index of first free position in buffer
bool sReceiveBufferOutOfSync = false;

/**
 * Read message in buffer for one event.
 * After RECEIVE_BUFFER_SIZE bytes check if SYNC_TOKEN was sent.
 * If OK then interpret content and reset buffer.
 */
static uint8_t sReceivedEventType = EVENT_NO_EVENT; // Buffer for EventType until event data is complete
static uint8_t sReceivedDataSize;

bool usePairedPin = false; // Use pin of BT module to decide if BT is paired, this cannot be done by using software managed mBlueDisplayConnectionEstablished value
void setUsePairedPin(bool aUsePairedPin) {
#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY) && defined(ARDUINO)
    usePairedPin = aUsePairedPin;
    if (aUsePairedPin) {
        pinMode(PAIRED_PIN, INPUT);
    }
#else
    (void) aUsePairedPin;
#endif
}

/*
 * Checks if additional remote display is paired to avoid program slow down by UART sending to a not paired connection
 *
 * USART_isBluetoothPaired() is only required if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
 *   to disable sending by Bluetooth if BT is not connected.
 * It is reduced to return false if defined(DISABLE_REMOTE_DISPLAY)
 * It is reduced to return true if not defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
 */
bool USART_isBluetoothPaired(void) {
#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
#  if defined(STM32F303xC) || defined(STM32F103xB)
    return ((BLUETOOTH_PAIRED_DETECT_PORT->IDR & BLUETOOTH_PAIRED_DETECT_PIN) != 0);
#  else
    if (!usePairedPin) {
        return true;
    }
    // use tVal to produce optimal code with the compiler
    uint8_t tVal = digitalReadFast(PAIRED_PIN);
    if (tVal != 0) {
        return true;
    }
    return false;
#  endif
#elif defined(DISABLE_REMOTE_DISPLAY)
    return false; // No BT connected display available -> is not paired by default
#else
    return true; // Only BT connected display available -> is paired by default
#endif // defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
}

/**
 * send:
 * 1. Sync byte A5
 * 2. byte Function token
 * 3. Short length of parameters (here 5*2)
 * 4. Short n parameters
 */
// using this function saves 300 bytes for SimpleDSO
void sendUSART5Args(uint8_t aFunctionTag, uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor) {
    if (USART_isBluetoothPaired()) {
        uint16_t tParamBuffer[7];

        uint16_t *tBufferPointer = &tParamBuffer[0];
        *tBufferPointer++ = aFunctionTag << 8 | SYNC_TOKEN; // add sync token
        *tBufferPointer++ = 10; // parameter length
        *tBufferPointer++ = aStartX;
        *tBufferPointer++ = aStartY;
        *tBufferPointer++ = aEndX;
        *tBufferPointer++ = aEndY;
        *tBufferPointer++ = aColor;
        sendUSARTBufferNoSizeCheck((uint8_t*) &tParamBuffer[0], 14, NULL, 0);
    }
}

/**
 *
 * @param aFunctionTag
 * @param aNumberOfArgs currently not more than 12 args (SHORT) are supported
 */
void sendUSARTArgs(uint8_t aFunctionTag, uint_fast8_t aNumberOfArgs, ...) {
    if (USART_isBluetoothPaired()) {
        if (aNumberOfArgs > MAX_NUMBER_OF_ARGS_FOR_BD_FUNCTIONS) {
#if !defined(ARDUINO)
            assertParamMessage((aNumberOfArgs <= MAX_NUMBER_OF_ARGS_FOR_BD_FUNCTIONS), aNumberOfArgs, "only 12 params max");
#endif
            return;
        }

        uint16_t tParamBuffer[MAX_NUMBER_OF_ARGS_FOR_BD_FUNCTIONS + 2];
        va_list argp;
        uint16_t *tBufferPointer = &tParamBuffer[0];
        *tBufferPointer++ = aFunctionTag << 8 | SYNC_TOKEN; // add sync token
        va_start(argp, aNumberOfArgs);

        *tBufferPointer++ = aNumberOfArgs * 2;
        for (uint_fast8_t i = 0; i < aNumberOfArgs; ++i) {
            *tBufferPointer++ = va_arg(argp, int);
        }
        va_end(argp);
        sendUSARTBufferNoSizeCheck((uint8_t*) &tParamBuffer[0], aNumberOfArgs * 2 + 4, NULL, 0);
    }
}

/**
 *
 * @param aFunctionTag
 * @param aNumberOfArgs currently not more than 12 args (SHORT) are supported
 * Last two arguments are length of buffer and buffer pointer (..., size_t aDataLength, uint8_t *aDataBufferPtr)
 */
void sendUSARTArgsAndByteBuffer(uint8_t aFunctionTag, uint_fast8_t aNumberOfArgs, ...) {
    if (USART_isBluetoothPaired()) {
        if (aNumberOfArgs > MAX_NUMBER_OF_ARGS_FOR_BD_FUNCTIONS) {
            return;
        }

        uint16_t tParamBuffer[MAX_NUMBER_OF_ARGS_FOR_BD_FUNCTIONS + 4];
        va_list argp;
        uint16_t *tBufferPointer = &tParamBuffer[0];
        *tBufferPointer++ = aFunctionTag << 8 | SYNC_TOKEN; // add sync token
        va_start(argp, aNumberOfArgs);

        *tBufferPointer++ = aNumberOfArgs * 2;
        for (uint_fast8_t i = 0; i < aNumberOfArgs; ++i) {
            *tBufferPointer++ = va_arg(argp, int);
        }
// add data field header
        *tBufferPointer++ = DATAFIELD_TAG_BYTE << 8 | SYNC_TOKEN; // start new transmission block
        uint16_t tLength = va_arg(argp, int); // length in byte
        *tBufferPointer = tLength;
        uint8_t *aBufferPtr = (uint8_t*) va_arg(argp, int); // Buffer address
        va_end(argp);

        sendUSARTBufferNoSizeCheck((uint8_t*) &tParamBuffer[0], aNumberOfArgs * 2 + 8, aBufferPtr, tLength);
    }
}

/*********************************************
 * serialEvent() function for standard serial
 *********************************************/
#if !defined(USE_SIMPLE_SERIAL)
#  if defined(ARDUINO)
uint8_t getReceiveBufferByte() {
    return Serial.read();
}

size_t getReceiveBytesAvailable() {
    return Serial.available();
}
#  endif
/**
 * Check if a touch event has completely received by USART
 * Function is not synchronized because it should only be used by main thread
 * Will be called after each loop() (by Arduino Serial...) to process input data if available.
 * Fills in the remoteEvent structure with BD event data from serial
 * EventType is set if event is complete
 */
void serialEvent(void) {
    if (sReceiveBufferOutOfSync) {
// just blocking wait for next sync token
        while (getReceiveBytesAvailable() > 0) {
            if (getReceiveBufferByte() == SYNC_TOKEN) {
                sReceiveBufferOutOfSync = false;
                sReceivedEventType = EVENT_NO_EVENT;
                break;
            }
        }
    }
    if (!sReceiveBufferOutOfSync) {
        /*
         * regular operation here
         */
        auto tBytesAvailable = getReceiveBytesAvailable();
        /*
         * enough bytes available for next step?
         */
        if (sReceivedEventType == EVENT_NO_EVENT) {
            if (tBytesAvailable >= 2) {
                /*
                 * read message length and event tag first
                 */
                // First byte is raw length so subtract 3 for sync+eventType+length bytes
                sReceivedDataSize = getReceiveBufferByte() - 3;
                if (sReceivedDataSize > RECEIVE_MAX_DATA_SIZE) {
                    // invalid length
                    sReceiveBufferOutOfSync = true;
                    return;
                }
                sReceivedEventType = getReceiveBufferByte();
                tBytesAvailable -= 2;
            }
        }
        if (sReceivedEventType != EVENT_NO_EVENT) {
            if (tBytesAvailable > sReceivedDataSize) {
                // Event complete received, now read data and sync token
                // Using getReceiveBufferByte() here saves up to 150 bytes :-)
                unsigned char *tByteArrayPtr = remoteEvent.EventData.ByteArray;
                int i;
                for (i = 0; i < sReceivedDataSize; ++i) {
                    *tByteArrayPtr++ = getReceiveBufferByte();
                }
                if (getReceiveBufferByte() == SYNC_TOKEN) {
                    remoteEvent.EventType = sReceivedEventType;
                    sReceivedEventType = EVENT_NO_EVENT; // reset EventType buffer
                    handleEvent(&remoteEvent);
                } else {
                    sReceiveBufferOutOfSync = true;
                }
            }
        }
    }
}
#endif // !defined(USE_SIMPLE_SERIAL)

/*********************************************************************
 *
 * Functions which depends on using simple or standard serial on AVR
 *
 ********************************************************************/
#if defined(ARDUINO)

#if defined(ESP32)
void initSerial(String aBTClientName) {
    SerialBT.begin(aBTClientName, false);
}
/*
 * Use default name
 */
void initSerial() {
    SerialBT.begin("ESP-BD_Example", false);
}

#else

/*
 * Take BLUETOOTH_BAUD_RATE for initialization, otherwise use 9600
 */
void initSerial() {
#  if defined(USE_SIMPLE_SERIAL)
#    if defined BLUETOOTH_BAUD_RATE
    initSimpleSerial(BLUETOOTH_BAUD_RATE);
#    else
    initSimpleSerial(9600);
#    endif
#  else
#    if defined BLUETOOTH_BAUD_RATE
    Serial.begin(BLUETOOTH_BAUD_RATE);
#    else
    Serial.begin(9600);
#    endif
#  endif // defined(USE_SIMPLE_SERIAL)
}

/*
 * With explicit baud rate
 */
void initSerial(uint32_t aBaudRate) {
#  if defined(USE_SIMPLE_SERIAL)
    initSimpleSerial(aBaudRate);
#  else
    Serial.begin(aBaudRate);
#  endif // defined(USE_SIMPLE_SERIAL)
}
#endif // defined(ESP32)

/**
 * The central point for sending bytes
 */
void sendUSARTBufferNoSizeCheck(uint8_t *aParameterBufferPointer, uint8_t aParameterBufferLength, uint8_t *aDataBufferPointer,
        size_t aDataBufferLength) {
#if !defined(USE_SIMPLE_SERIAL) || (!defined(UCSR1A) && !defined(UCSR0A))
    Serial.write(aParameterBufferPointer, aParameterBufferLength);
    Serial.write(aDataBufferPointer, aDataBufferLength);
#else
    /*
     * Simple and reliable blocking version for Atmega328
     */
    while (aParameterBufferLength > 0) {
        // wait for USART send buffer to become empty
#  if (defined(UCSR1A) && ! defined(USE_USB_SERIAL)) || ! defined(UCSR0A) // Use TX1 on MEGA and on Leonardo, which has no TX0
        while (!((UCSR1A) & (1 << UDRE1))) {
            ;
        }
        UDR1 = *aParameterBufferPointer;
#  else
        while (!((UCSR0A) & (1 << UDRE0))) {
            ;
        }
        UDR0 = *aParameterBufferPointer;
#  endif // defined(UCSR1A)

        aParameterBufferPointer++;
        aParameterBufferLength--;
    }
    while (aDataBufferLength > 0) {
        // wait for USART send buffer to become empty
#  if (defined(UCSR1A) && ! defined(USE_USB_SERIAL)) || ! defined(UCSR0A) // Use TX1 on MEGA and on Leonardo, which has no TX0
        while (!((UCSR1A) & (1 << UDRE1))) {
            ;
        }
        UDR1 = *aDataBufferPointer;
#  else
        while (!((UCSR0A) & (1 << UDRE0))) {
            ;
        }
        UDR0 = *aDataBufferPointer;
#  endif // defined(UCSR1A)

        aDataBufferPointer++;
        aDataBufferLength--;
    }
#endif // USE_SIMPLE_SERIAL
}

#endif // defined(ARDUINO)

#if defined(AVR)

#  if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
#include "LocalDisplay/digitalWriteFast.h"
#  endif

#  if !defined(USE_SIMPLE_SERIAL) && defined(USE_SERIAL1)
#define Serial Serial1
#  endif

/**************************************************************
 *
 * Functions which depends on using simple or standard serial
 *
 **************************************************************/

/**
 * ultra simple blocking USART send routine - works 100%!
 * Only defined for USE_SIMPLE_SERIAL
 */
void sendUSART(char aChar) {
#if defined(USE_SIMPLE_SERIAL)
    // wait for buffer to become empty
#  if defined(UCSR1A)
    // Use TX1 on MEGA and on Leonardo, which has no TX0
    while (!((UCSR1A) & (1 << UDRE1))) {
        ;
    }
    UDR1 = aChar;
#  elif defined(UCSR0A)
    while (!((UCSR0A) & (1 << UDRE0))) {
        ;
    }
    UDR0 = aChar;
#  endif
#else
    Serial.write(aChar);
#endif // USE_SIMPLE_SERIAL
}

void sendUSART(const char *aString) {
    while (*aString != '0') {
        sendUSART(*aString);
        aString++;
    }
}

/*******************************************
 *
 * Functions only valid for simple serial
 *
 *******************************************/
#if defined(USE_SIMPLE_SERIAL)
void initSimpleSerial(uint32_t aBaudRate, bool aUsePairedPin) {
#  if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
    setUsePairedPin(aUsePairedPin);
#  else
    (void) aUsePairedPin; // unused parameter
#  endif
    initSimpleSerial(aBaudRate);
}

void initSimpleSerial(uint32_t aBaudRate) {
    uint16_t baud_setting;
#  if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__) || defined(ARDUINO_AVR_LEONARDO) || defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__)
    // Use TX1 on MEGA and on Leonardo, which has no TX0
    UCSR1A = 1 << U2X1;// Double Speed Mode
    // Exact value = 17,3611 (- 1) for 115200  2,1%
    // 8,68 (- 1) for 230400 8,5% for 8, 3.7% for 9
    // 4,34 (- 1) for 460800 8,5%
    // HC-05 Specified Max Total Error (%) for 8 bit= +3.90/-4.00
    baud_setting = (((F_CPU / 4) / aBaudRate) - 1) / 2;// /2 after -1 because of better rounding

    // assign the baud_setting, a.k.a. ubbr (USART Baud Rate Register)
    UBRR1H = baud_setting >> 8;
    UBRR1L = baud_setting;

    // enable: TX, RX, RX Complete Interrupt
    UCSR1B = (1 << RXEN1) | (1 << TXEN1) | (1 << RXCIE1);
#  else
    UCSR0A = 1 << U2X0; // Double Speed Mode
    // Exact value = 17,3611 (- 1) for 115200  2,1%
    // 8,68 (- 1) for 230400 8,5% for 8, 3.7% for 9
    // 4,34 (- 1) for 460800 8,5%
    // HC-05 Specified Max Total Error (%) for 8 bit= +3.90/-4.00
    baud_setting = (((F_CPU / 4) / aBaudRate) - 1) / 2;        // /2 after -1 because of better rounding

    // assign the baud_setting, a.k.a. ubbr (USART Baud Rate Register)
    UBRR0H = baud_setting >> 8;
    UBRR0L = baud_setting;

    // enable: TX, RX, RX Complete Interrupt
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
#  endif // defined(__AVR_ATmega1280__) || ...
    remoteEvent.EventType = EVENT_NO_EVENT;
    remoteTouchDownEvent.EventType = EVENT_NO_EVENT;
}

#  if defined(USART1_RX_vect)
// Use TX1 on MEGA and on Leonardo, which has no TX0
ISR(USART1_RX_vect) {
    uint8_t tByte = UDR1;
#  else
    ISR(USART_RX_vect) {
        uint8_t tByte = UDR0;
#  endif
        if (sReceiveBufferOutOfSync) {
            // just wait for next sync token and reset buffer
            if (tByte == SYNC_TOKEN) {
                sReceiveBufferOutOfSync = false;
                sReceivedEventType = EVENT_NO_EVENT;
                sReceiveBufferIndex = 0;
            }
        } else {
            if (sReceivedEventType == EVENT_NO_EVENT) {
                if (sReceiveBufferIndex == 0) {
                    // First byte is raw length so subtract 3 for sync+eventType+length bytes
                    tByte -= 3;
                    if (tByte > RECEIVE_MAX_DATA_SIZE) {
                        sReceiveBufferOutOfSync = true;
                    } else {
                        sReceiveBufferIndex++;
                        sReceivedDataSize = tByte;
                    }
                } else {
                    // Second byte is eventType
                    // setup for receiving plain message bytes
                    sReceivedEventType = tByte;
                    sReceiveBufferIndex = 0;
                }
            } else {
                if (sReceiveBufferIndex == sReceivedDataSize) {
                    // now we expect a sync token
                    if (tByte == SYNC_TOKEN) {
                        // event completely received
                        // we have one dedicated touch down event in order not to overwrite it with other events before processing it
                        // Yes it makes no sense if interrupts are allowed!
                        struct BluetoothEvent *tRemoteTouchEventPtr = &remoteEvent;
#  if !defined(DO_NOT_NEED_BASIC_TOUCH)
                        if (sReceivedEventType == EVENT_TOUCH_ACTION_DOWN
                                || (remoteTouchDownEvent.EventType == EVENT_NO_EVENT && remoteEvent.EventType == EVENT_NO_EVENT)) {
                            tRemoteTouchEventPtr = &remoteTouchDownEvent; // Use remoteTouchDownEvent now
                        }
#  endif
                        tRemoteTouchEventPtr->EventType = sReceivedEventType;
                        // copy buffer to structure
                        memcpy(tRemoteTouchEventPtr->EventData.ByteArray, sReceiveBuffer, sReceivedDataSize);
                        sReceiveBufferIndex = 0;
                        sReceivedEventType = EVENT_NO_EVENT;

                    } else {
                        // reset buffer since we had an overflow or glitch
                        sReceiveBufferOutOfSync = true;
                        sReceiveBufferIndex = 0;
                    }
                } else {
                    // plain message byte
                    sReceiveBuffer[sReceiveBufferIndex++] = tByte;
                }
            }
        }
    }
#endif // USE_SIMPLE_SERIAL

#elif !defined(ARDUINO) && (defined(STM32F303xC) || defined(STM32F103xB)) // defined(AVR)

#include "BlueSerial.h"
#include "BlueDisplay.h"

#include "timing.h"
#include "stm32fx0xPeripherals.h" // For Watchdog_reload()

#include <string.h> // for memcpy
#include <stdarg.h>  // for varargs

//#define USE_SIMPLE_SERIAL

DMA_HandleTypeDef DMA_UART_BD_TXHandle;
DMA_HandleTypeDef DMA_UART_BD_RXHandle;
UART_HandleTypeDef UART_BD_Handle;

/**
 * UART receive is done via continuous DMA transfer to a circular receive buffer.
 * At this time only touch events of 6 byte are transferred.
 * The thread process checks the receive buffer via checkTouchReceived
 * and marks processed events by just clearing the data.
 * Buffer overrun is detected by using a Buffer size of (n*6)-1 so the sync token
 * in case of overrun is on another position than the one expected.
 *
 * UART sending is done by writing data to a circular send buffer and then starting the DMA for this data.
 * During transmission further data can be written into the buffer until it is full.
 * The next write then waits for the ongoing transmission(s) to end (blocking wait) until enough free space is available.
 * If an transmission ends, the buffer space used for this transmission gets available for next send data.
 * If there is more data in the buffer to send, then the next DMA transfer for the remaining data is started immediately.
 */

/*
 * UART constants
 */
// send buffer
#define UART_SEND_BUFFER_SIZE 1024
uint8_t *sUSARTSendBufferPointerIn; // only set by thread - point to first byte of free buffer space
volatile uint8_t *sUSARTSendBufferPointerOut; // only set by ISR - point to first byte not yet transfered
uint8_t USARTSendBuffer[UART_SEND_BUFFER_SIZE] __attribute__ ((aligned(4)));
uint8_t *sUSARTSendBufferPointerOutTmp; // value of sUSARTSendBufferPointerOut after transfer complete
volatile bool sDMATransferOngoing = false;  // synchronizing flag for ISR <-> thread

// Circular receive buffer
#define USART_RECEIVE_BUFFER_SIZE (TOUCH_COMMAND_MAX_DATA_SIZE * 10 -1) // not a multiple of TOUCH_COMMAND_SIZE_BYTE in order to discover overruns
uint8_t USARTReceiveBuffer[USART_RECEIVE_BUFFER_SIZE] __attribute__ ((aligned(4)));
uint8_t *sUSARTReceiveBufferPointer; // point to first byte not yet processed (of next received message)
size_t sLastRXDMACount;

/**
 * Init the input for Bluetooth HC-05 state pin
 * Initialization of clock and the RX and TX pins
 * Initialization of the RX and TX DMA channels and buffer pointer.
 * Starting the RX channel.
 */
void HAL_UART_MspInit(UART_HandleTypeDef *aUARTHandle) {
    if (aUARTHandle == &UART_BD_Handle) {
        UART_BD_CLOCK_ENABLE();
        UART_BD_IO_CLOCK_ENABLE();

        GPIO_InitTypeDef GPIO_InitStructure;
        // Bluetooth state pin
        GPIO_InitStructure.Pin = BLUETOOTH_PAIRED_DETECT_PIN;
        GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
        GPIO_InitStructure.Pull = GPIO_PULLDOWN; // Input is active high
        HAL_GPIO_Init(BLUETOOTH_PAIRED_DETECT_PORT, &GPIO_InitStructure);

#ifdef STM32F30X
        // Configure UART pins:   Tx (10)  and Rx (11) on Port C
        // Since the AF codes are the same, we can do it in one call
        GPIO_InitStructure.Pin = UART_BD_TX_PIN | UART_BD_RX_PIN;
        GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
        GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStructure.Pull = GPIO_PULLUP;
        GPIO_InitStructure.Alternate = GPIO_AF7_USART3;
#else
        // need 2 calls
        GPIO_InitStructure.Pin = UART_BD_TX_PIN;
        GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStructure.Speed = GPIO_SPEED_LOW;
        HAL_GPIO_Init(UART_BD_PORT, &GPIO_InitStructure);

        GPIO_InitStructure.Pin = UART_BD_RX_PIN;
        GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
        GPIO_InitStructure.Pull = GPIO_NOPULL;
#endif
        HAL_GPIO_Init(UART_BD_PORT, &GPIO_InitStructure);

        UART_BD_DMA_CLOCK_ENABLE();
        DMA_UART_BD_TXHandle.Instance = UART_BD_DMA_TX_CHANNEL;

        /*
         * init TX channel and buffer pointer
         */
        sUSARTSendBufferPointerIn = &USARTSendBuffer[0];
        sUSARTSendBufferPointerOut = &USARTSendBuffer[0];
        DMA_UART_BD_TXHandle.Init.Direction = DMA_MEMORY_TO_PERIPH;
        DMA_UART_BD_TXHandle.Init.PeriphInc = DMA_PINC_DISABLE;
        DMA_UART_BD_TXHandle.Init.MemInc = DMA_MINC_ENABLE;
        DMA_UART_BD_TXHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        DMA_UART_BD_TXHandle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        DMA_UART_BD_TXHandle.Init.Mode = DMA_NORMAL;
        DMA_UART_BD_TXHandle.Init.Priority = DMA_PRIORITY_LOW;
        HAL_DMA_Init(&DMA_UART_BD_TXHandle);

        // Link to UART otherwise HAL_USART_Transmit_DMA crashes with nullpointer
        __HAL_LINKDMA(&UART_BD_Handle, hdmatx, DMA_UART_BD_TXHandle);

#ifdef STM32F30X
        DMA_UART_BD_TXHandle.Instance->CPAR = (uint32_t) &UART_BD_Handle.Instance->TDR;
#else
        DMA_UART_BD_TXHandle.Instance->CPAR = (uint32_t) &UART_BD_Handle.Instance->DR;
#endif
        UART_BD_Handle.Instance->CR3 |= USART_CR3_DMAT; // enable DMA transmit

        /*
         * init RX channel and buffer pointer
         */
        DMA_UART_BD_RXHandle.Instance = UART_BD_DMA_RX_CHANNEL;

        sUSARTReceiveBufferPointer = &USARTReceiveBuffer[0];
        sLastRXDMACount = USART_RECEIVE_BUFFER_SIZE;

        DMA_UART_BD_RXHandle.Init.Direction = DMA_PERIPH_TO_MEMORY;
        DMA_UART_BD_RXHandle.Init.PeriphInc = DMA_PINC_DISABLE;
        DMA_UART_BD_RXHandle.Init.MemInc = DMA_MINC_ENABLE;
        DMA_UART_BD_RXHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        DMA_UART_BD_RXHandle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        DMA_UART_BD_RXHandle.Init.Mode = DMA_CIRCULAR;
        DMA_UART_BD_RXHandle.Init.Priority = DMA_PRIORITY_LOW;
        HAL_DMA_Init(&DMA_UART_BD_RXHandle);

        __HAL_LINKDMA(&UART_BD_Handle, hdmarx, DMA_UART_BD_RXHandle);

        // set register manually
#ifdef STM32F30X
        DMA_UART_BD_RXHandle.Instance->CPAR = (uint32_t) &UART_BD_Handle.Instance->RDR;
#else
        DMA_UART_BD_RXHandle.Instance->CPAR = (uint32_t) &UART_BD_Handle.Instance->DR;
#endif
        DMA_UART_BD_RXHandle.Instance->CMAR = (uint32_t) sUSARTReceiveBufferPointer;
        // Write to DMA Channel CNDTR
        DMA_UART_BD_RXHandle.Instance->CNDTR = USART_RECEIVE_BUFFER_SIZE;

        UART_BD_Handle.Instance->CR3 |= USART_CR3_DMAR; // enable DMA receive
        DMA_UART_BD_RXHandle.Instance->CCR |= DMA_CCR_EN; // Channel enable - no interrupts!
    }
}

/**
 * Initialization of the USART itself and the NVIC for the interrupts.
 */
void UART_BD_initialize(uint32_t aBaudRate) {

    UART_BD_Handle.Instance = UART_BD;

    UART_BD_Handle.Init.BaudRate = aBaudRate;
    UART_BD_Handle.Init.WordLength = UART_WORDLENGTH_8B;
    UART_BD_Handle.Init.StopBits = UART_STOPBITS_1;
    UART_BD_Handle.Init.Parity = UART_PARITY_NONE;
    UART_BD_Handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    UART_BD_Handle.Init.Mode = UART_MODE_TX_RX; // receiver and transmitter enable
    HAL_UART_Init(&UART_BD_Handle);

//    USART_ITConfig(UART_BD_Handle.Instance, USART_IT_RXNE, ENABLE);   // enable Receive and overrun Interrupt
//    USART_ITConfig(UART_BD_Handle.Instance, USART_IT_TC, ENABLE); // enable the UART transfer_complete interrupt

    /* Enable USART IRQ to lowest prio*/
    NVIC_SetPriority((IRQn_Type) (UART_BD_IRQ), 3);
    HAL_NVIC_EnableIRQ((IRQn_Type) (UART_BD_IRQ));
}

void setUART_BD_BaudRate(uint32_t aBaudRate) {
    UART_BD_Handle.Init.BaudRate = aBaudRate;
    UART_BD_Handle.Instance->BRR = (uint16_t) (HAL_RCC_GetPCLK1Freq() / UART_BD_Handle.Init.BaudRate);
}

uint32_t getUSART_BD_BaudRate(void) {
    return UART_BD_Handle.Init.BaudRate;
}

/**
 * Not used yet
 * Reset RX_DMA count and start address to initial values.
 * Used by buffer error/overrun handling
 */
void UART_BD_DMA_RX_reset(void) {
// Disable DMA1 channel2 - is really needed here!
    UART_BD_Handle.hdmarx->Instance->CCR &= ~DMA_CCR_EN;

// Write to DMA1 CMAR
    UART_BD_Handle.hdmarx->Instance->CMAR = (uint32_t) &USARTReceiveBuffer[0];
    sUSARTReceiveBufferPointer = &USARTReceiveBuffer[0];
    // clear receive buffer
    memset(&USARTReceiveBuffer[0], 0, USART_RECEIVE_BUFFER_SIZE);

    // Write to DMA1 CNDTR
    UART_BD_Handle.hdmarx->Instance->CNDTR = USART_RECEIVE_BUFFER_SIZE;
    UART_BD_Handle.hdmarx->Instance->CCR |= DMA_CCR_EN; // No interrupts!
}

/**
 * Starts a new DMA to USART transfer with the given parameters.
 * Assert that USART is ready for new transfer.
 * No further parameter check is done here!
 */
void UART_BD_DMA_TX_start(uint32_t aMemoryBaseAddr, uint32_t aBufferSize) {
    if (sDMATransferOngoing) {
        return; // not allowed to start a new transfer, because DMA is busy
    }

    // assertion if no Transfer ongoing, but USART TX Buffer not empty
    assert_param(__HAL_UART_GET_FLAG(&UART_BD_Handle, UART_FLAG_TXE ) != RESET);

    sDMATransferOngoing = true;

    // Compute next buffer out pointer
    uint8_t *tUSARTSendBufferPointerOutTmp = (uint8_t*) (aMemoryBaseAddr + aBufferSize);
    // check for buffer wrap around
    if (tUSARTSendBufferPointerOutTmp >= &USARTSendBuffer[UART_SEND_BUFFER_SIZE]) {
        tUSARTSendBufferPointerOutTmp = &USARTSendBuffer[0];
    }
    sUSARTSendBufferPointerOutTmp = tUSARTSendBufferPointerOutTmp;

    if (aBufferSize == 1) {
        // no DMA needed just put data to TDR register
#ifdef STM32F30X
        UART_BD_Handle.Instance->TDR = *(uint8_t*) aMemoryBaseAddr;
#else
        UART_BD_Handle.Instance->DR = *(uint8_t*) aMemoryBaseAddr;
#endif
    } else {
        // Disable DMA channel - it is really needed here!
        UART_BD_Handle.hdmatx->Instance->CCR &= ~DMA_CCR_EN;

        // Write to DMA Channel CMAR
        UART_BD_Handle.hdmatx->Instance->CMAR = aMemoryBaseAddr;
        // Write to DMA Channel CNDTR
        UART_BD_Handle.hdmatx->Instance->CNDTR = aBufferSize;
        //USART_ClearFlag(UART_BD_Handle.Instance, USART_FLAG_TC);
#ifdef STM32F30X
        __HAL_UART_CLEAR_IT(&UART_BD_Handle, UART_CLEAR_TCF);
#else
        __HAL_UART_CLEAR_FLAG(&UART_BD_Handle, UART_FLAG_TC);
#endif
        // enable DMA channel
        UART_BD_Handle.hdmatx->Instance->CCR |= DMA_CCR_EN;
        //DMA_Cmd(USART_DMA_TX_CHANNEL, ENABLE); // No DMA interrupts!
    }
    // Enable USART TC interrupt
    __HAL_UART_ENABLE_IT(&UART_BD_Handle, UART_IT_TC);
}

/**
 * We must wait for USART transfer complete before starting next DMA,
 * otherwise the last byte of the transfer will be corrupted!!!
 * Therefore we must use USART and not the DMA TC interrupt!
 */
extern "C" void UART_BD_IRQHANDLER(void) {
    //if (USART_GetITStatus(UART_BD_Handle.Instance, USART_IT_TC) != RESET) {
    if (__HAL_UART_GET_FLAG(&UART_BD_Handle, UART_FLAG_TC) != RESET) {

        sUSARTSendBufferPointerOut = sUSARTSendBufferPointerOutTmp;
        sDMATransferOngoing = false;
        if (sUSARTSendBufferPointerOut == sUSARTSendBufferPointerIn) {
            // transfer complete and no new data arrived in buffer
            /*
             * !! USART_ClearFlag(UART_BD_Handle.Instance, USART_FLAG_TC) has no effect on the TC Flag !!!! => next interrupt will happen after return from ISR
             * Must disable interrupt here otherwise it will interrupt forever (STM bug???)
             */
            __HAL_UART_DISABLE_IT(&UART_BD_Handle, UART_IT_TC);
            //USART_ITConfig(UART_BD_Handle.Instance, USART_IT_TC, DISABLE);
            //USART_ClearFlag(UART_BD_Handle.Instance, USART_FLAG_TC );
        } else {
            uint8_t *tUSARTSendBufferPointerIn = sUSARTSendBufferPointerIn;
            if (sUSARTSendBufferPointerOut < tUSARTSendBufferPointerIn) {
                // new data in buffer -> start new transfer
                UART_BD_DMA_TX_start((uint32_t) sUSARTSendBufferPointerOut,
                        (uint32_t) (tUSARTSendBufferPointerIn - sUSARTSendBufferPointerOut));
            } else {
                // new data, but buffer wrap around occurred - send tail of buffer
                UART_BD_DMA_TX_start((uint32_t) sUSARTSendBufferPointerOut,
                        &USARTSendBuffer[UART_SEND_BUFFER_SIZE] - sUSARTSendBufferPointerOut);
            }
        }
    }
}

/*
 * Buffer handling
 */
/**
 * put data byte and check for wrap around
 */
uint8_t* putSendBuffer(uint8_t *aUSARTSendBufferPointerIn, uint8_t aData) {
    *aUSARTSendBufferPointerIn++ = aData;
    // check for buffer wrap around
    if (aUSARTSendBufferPointerIn >= &USARTSendBuffer[UART_SEND_BUFFER_SIZE]) {
        aUSARTSendBufferPointerIn = &USARTSendBuffer[0];
    }
    return aUSARTSendBufferPointerIn;
}

int getSendBufferFreeSpace(void) {
    if (sUSARTSendBufferPointerOut == sUSARTSendBufferPointerIn && !sDMATransferOngoing) {
        // buffer empty
        return UART_SEND_BUFFER_SIZE;
    }
    if (sUSARTSendBufferPointerOut < sUSARTSendBufferPointerIn) {
        return (UART_SEND_BUFFER_SIZE - (sUSARTSendBufferPointerIn - sUSARTSendBufferPointerOut));
    }
    // buffer is completely filled up with data or buffer wrap around
    return (sUSARTSendBufferPointerOut - sUSARTSendBufferPointerIn);
}

/**
 * Copy content of both buffers to send buffer, check for buffer wrap around and call USART_BD_DMA_TX_start() with right parameters.
 * Do blocking wait if not enough space left in buffer
 */
void sendUSARTBufferNoSizeCheck(uint8_t *aParameterBufferPointer, uint8_t aParameterBufferLength, uint8_t *aDataBufferPointer,
        size_t aDataBufferLength) {
#ifdef USE_SIMPLE_SERIAL
    sendUSARTBufferSimple(aParameterBufferPointer, aParameterBufferLength, aDataBufferPointer, aDataBufferLength);
    return;
#else

    if (!sDMATransferOngoing) {
        // safe to reset buffer pointers since no transmit pending
        sUSARTSendBufferPointerOut = &USARTSendBuffer[0];
        sUSARTSendBufferPointerIn = &USARTSendBuffer[0];
    }
    uint8_t *tUSARTSendBufferPointerIn = sUSARTSendBufferPointerIn;
    int tSize = aParameterBufferLength + aDataBufferLength;
    /*
     * check (and wait) for enough free space
     */
    if (getSendBufferFreeSpace() < tSize) {
        // not enough space left - wait for transfer (chain) to complete or for size
        // get interrupt level
        uint32_t tISPR = (__get_IPSR() & 0xFF);
        setTimeoutMillis(300); // enough for 256 bytes at 9600
        while (sDMATransferOngoing) {
            // is needed here, because early watchdog ISR sends also data
#ifdef HAL_WWDG_MODULE_ENABLED
            Watchdog_reload();
#endif
            if (tISPR > 0) {
                // here in ISR, check manually for TransferComplete interrupt flag
                if (__HAL_UART_GET_FLAG(&UART_BD_Handle, UART_FLAG_TC) != RESET) {
                    // call ISR Handler manually
                    UART_BD_IRQHANDLER();
                    // Assertion
                    assert_param(__HAL_UART_GET_FLAG(&UART_BD_Handle, UART_FLAG_TC) == RESET);
                }
            }

            if (getSendBufferFreeSpace() >= tSize) {
                break;
            }
            if (isTimeoutSimple()) {
                // skip transfer, don't overwrite
                return;
            }
        }
    }

    /*
     * enough space here
     */
    uint8_t *tStartBufferPointer = tUSARTSendBufferPointerIn;

    int tBufferSizeToEndOfBuffer = (&USARTSendBuffer[UART_SEND_BUFFER_SIZE] - tUSARTSendBufferPointerIn);
    if (tBufferSizeToEndOfBuffer < tSize) {
        //transfer possible, but must be done in 2 chunks since DMA cannot handle buffer wrap around during a transfer
        tSize = tBufferSizeToEndOfBuffer; // set size for 1. chunk
        // copy parameter the hard way
        while (aParameterBufferLength > 0) {
            tUSARTSendBufferPointerIn = putSendBuffer(tUSARTSendBufferPointerIn, *aParameterBufferPointer++);
            aParameterBufferLength--;
        }
        //copy data
        while (aDataBufferLength > 0) {
            tUSARTSendBufferPointerIn = putSendBuffer(tUSARTSendBufferPointerIn, *aDataBufferPointer++);
            aDataBufferLength--;
        }
    } else {
        // copy parameter and data with memcpy
        memcpy((uint8_t*) tUSARTSendBufferPointerIn, aParameterBufferPointer, aParameterBufferLength);
        tUSARTSendBufferPointerIn += aParameterBufferLength;

        if (aDataBufferLength > 0) {
            memcpy((uint8_t*) tUSARTSendBufferPointerIn, aDataBufferPointer, aDataBufferLength);
            tUSARTSendBufferPointerIn += aDataBufferLength;
        }
        // check for buffer wrap around - happens if tBufferSizeToEndOfBuffer == tSize
        if (tUSARTSendBufferPointerIn >= &USARTSendBuffer[UART_SEND_BUFFER_SIZE]) {
            tUSARTSendBufferPointerIn = &USARTSendBuffer[0];
        }
    }
// the only statement which writes the variable sUSARTSendBufferPointerIn
    sUSARTSendBufferPointerIn = tUSARTSendBufferPointerIn;

// start DMA if not already running
    UART_BD_DMA_TX_start((uint32_t) tStartBufferPointer, tSize);
#endif
}

#include <stdlib.h> // for abs()
/**
 * used if databuffer can be greater than USART_SEND_BUFFER_SIZE
 */
void sendUSARTBuffer(uint8_t *aParameterBufferPointer, size_t aParameterBufferLength, uint8_t *aDataBufferPointer,
        size_t aDataBufferLength) {
#ifdef USE_SIMPLE_SERIAL
    sendUSARTBufferSimple(aParameterBufferPointer, aParameterBufferLength, aDataBufferPointer, aDataBufferLength);
    return;
#else
    if ((aParameterBufferLength + aDataBufferLength) > UART_SEND_BUFFER_SIZE) {
        // first send command
        sendUSARTBufferNoSizeCheck(aParameterBufferPointer, aParameterBufferLength, NULL, 0);
        // then send data in USART_SEND_BUFFER_SIZE chunks
        int tSize = aDataBufferLength;
        while (tSize > 0) {
            int tSendSize = UART_SEND_BUFFER_SIZE;
            if (tSize < UART_SEND_BUFFER_SIZE) {
                tSendSize = tSize;
            }
            sendUSARTBufferNoSizeCheck(aDataBufferPointer, tSendSize, NULL, 0);
            aDataBufferPointer += UART_SEND_BUFFER_SIZE;
            tSize -= UART_SEND_BUFFER_SIZE;
        }
    } else {
        sendUSARTBufferNoSizeCheck(aParameterBufferPointer, aParameterBufferLength, aDataBufferPointer, aDataBufferLength);
    }
#endif
}

#ifdef USE_SIMPLE_SERIAL
/**
 * very simple blocking USART send routine - works 100%!
 */
void sendUSARTBufferSimple(uint8_t * aParameterBufferPointer, size_t aParameterBufferLength, uint8_t * aDataBufferPointer,
        size_t aDataBufferLength) {
    while (aParameterBufferLength > 0) {
// wait for USART send buffer to become empty
        while (__HAL_UART_GET_FLAG(&UART_BD_Handle, UART_FLAG_TXE) == RESET) {
            ;
        }

//USART_SendData(UART_BD_Handle.Instance, *aCharPtr);
#ifdef STM32F30X
        UART_BD_Handle.Instance->TDR = (*aParameterBufferPointer & (uint16_t) 0x00FF);
#else
        UART_BD_Handle.Instance->DR = (*aParameterBufferPointer & (uint16_t) 0x00FF);
#endif
        aParameterBufferPointer++;
        aParameterBufferLength--;
    }
    while (aDataBufferLength > 0) {
// wait for USART send buffer to become empty
        while (__HAL_UART_GET_FLAG(&UART_BD_Handle, UART_FLAG_TXE) == RESET) {
            ;
        }
//USART_SendData(UART_BD_Handle.Instance, *aCharPtr);
#ifdef STM32F30X
        UART_BD_Handle.Instance->TDR = (*aDataBufferPointer & (uint16_t) 0x00FF);
#else
        UART_BD_Handle.Instance->DR = (*aDataBufferPointer & (uint16_t) 0x00FF);
#endif
        aDataBufferPointer++;
        aDataBufferLength--;
    }
}
#endif

/**
 * Get a short from receive buffer, clear it in buffer and handle buffer wrap around
 */
uint16_t getReceiveBufferWord(void) {
    uint8_t *tUSARTReceiveBufferPointer = sUSARTReceiveBufferPointer;
    if (tUSARTReceiveBufferPointer >= &USARTReceiveBuffer[USART_RECEIVE_BUFFER_SIZE]) {
        // - since pointer may be far behind buffer end
        tUSARTReceiveBufferPointer -= USART_RECEIVE_BUFFER_SIZE;
    }
    uint16_t tResult = *tUSARTReceiveBufferPointer;
    *tUSARTReceiveBufferPointer = 0x00;
    tUSARTReceiveBufferPointer++;
    if (tUSARTReceiveBufferPointer >= &USARTReceiveBuffer[USART_RECEIVE_BUFFER_SIZE]) {
        tUSARTReceiveBufferPointer = &USARTReceiveBuffer[0];
    }
    tResult |= (*tUSARTReceiveBufferPointer) << 8;
    *tUSARTReceiveBufferPointer = 0x00;
    tUSARTReceiveBufferPointer++;
    if (tUSARTReceiveBufferPointer >= &USARTReceiveBuffer[USART_RECEIVE_BUFFER_SIZE]) {
        tUSARTReceiveBufferPointer = &USARTReceiveBuffer[0];
    }
    sLastRXDMACount -= 2;
    if (sLastRXDMACount <= 0) {
        sLastRXDMACount += USART_RECEIVE_BUFFER_SIZE;
    }
    sUSARTReceiveBufferPointer = tUSARTReceiveBufferPointer;
    return tResult;
}

/**
 * Get a byte from receive buffer, clear it in buffer and handle buffer wrap around
 */
uint8_t getReceiveBufferByte(void) {
    uint8_t *tUSARTReceiveBufferPointer = sUSARTReceiveBufferPointer;
    uint8_t tResult = *tUSARTReceiveBufferPointer;
    *tUSARTReceiveBufferPointer = 0x00;
    tUSARTReceiveBufferPointer++;
    if (tUSARTReceiveBufferPointer >= &USARTReceiveBuffer[USART_RECEIVE_BUFFER_SIZE]) {
        tUSARTReceiveBufferPointer = &USARTReceiveBuffer[0];
    }
    sLastRXDMACount--;
    if (sLastRXDMACount <= 0) {
        sLastRXDMACount += USART_RECEIVE_BUFFER_SIZE;
    }
    sUSARTReceiveBufferPointer = tUSARTReceiveBufferPointer;
    return tResult;
}

/*
 * computes received bytes since LastRXDMACount
 */
size_t getReceiveBytesAvailable(void) {
    size_t tCount = UART_BD_Handle.hdmarx->Instance->CNDTR;
    if (tCount <= sLastRXDMACount) {
        return sLastRXDMACount - tCount;
    } else {
        // DMA wrap around
        return sLastRXDMACount + (USART_RECEIVE_BUFFER_SIZE - tCount);
    }
}

/*
 * NOT USED YET - maybe useful for Error Interrupt IT_TE2
 */
// starting a new DMA just after DMA TC interrupt corrupts the last byte of the transfer still ongoing.
extern "C" void DMA1_Channel2_IRQHandler(void) {
// Test on DMA Transfer Complete interrupt
    if (__HAL_DMA_GET_FLAG(UART_BD_Handle.hdmatx, DMA_FLAG_TC2)) {
        /* Clear DMA  Transfer Complete interrupt pending bit */
        __HAL_DMA_CLEAR_FLAG(UART_BD_Handle.hdmatx, DMA_FLAG_TC2);
        //DMA_ClearITPendingBit(DMA1_IT_TC2);
//sUSARTDmaReady = true;
    }
// Test on DMA Transfer Error interrupt
    if (__HAL_DMA_GET_FLAG(UART_BD_Handle.hdmatx, DMA_FLAG_TE2)) {
        failParamMessage(UART_BD_Handle.hdmatx->Instance->CPAR, "DMA Error");
        __HAL_DMA_CLEAR_FLAG(UART_BD_Handle.hdmatx, DMA_FLAG_TE2);
        //DMA_ClearITPendingBit(DMA1_IT_TE2);
    }
// Test on DMA Transfer Half interrupt
    if (__HAL_DMA_GET_FLAG(UART_BD_Handle.hdmatx, DMA_FLAG_HT2)) {
        __HAL_DMA_CLEAR_FLAG(UART_BD_Handle.hdmatx, DMA_FLAG_HT2);
        //DMA_ClearITPendingBit(DMA1_IT_HT2);
    }
}
#endif // defined(STM32F303xC) || defined(STM32F103xB)

#if defined(SERIAL_REDEFINED)
#undef Serial
#endif
#endif // _BLUESERIAL_HPP
