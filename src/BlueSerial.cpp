/*
 * BlueSerial.cpp
 *
 * Implements the "simpleSerial" low level serial functions for communication with the Android BlueDisplay app.
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#include <Arduino.h>
#include "BlueDisplay.h"

#if !defined(va_start)
#include <cstdarg> // for va_start, va_list etc.
#endif

// definitions from <wiring_private.h>
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

#undef USART_isBluetoothPaired

#if defined(LOCAL_DISPLAY_EXISTS) && defined(REMOTE_DISPLAY_SUPPORTED)
bool usePairedPin = false;

void setUsePairedPin(bool aUsePairedPin) {
    usePairedPin = aUsePairedPin;
}

bool USART_isBluetoothPaired(void) {
    if (!usePairedPin) {
        return true;
    }
    // use tVal to produce optimal code with the compiler
    uint8_t tVal = digitalReadFast(PAIRED_PIN);
    if (tVal != 0) {
        return true;
    }
    return false;
}
#endif // defined(LOCAL_DISPLAY_EXISTS) && defined(REMOTE_DISPLAY_SUPPORTED)

#if ! defined(USE_SIMPLE_SERIAL) && defined(USE_SERIAL1)
#define Serial Serial1
#endif

#if defined(ESP32)
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;
#define Serial SerialBT
#endif

/*
 * Wrapper for calling initSimpleSerial or Serial[0,1].begin
 */
#if defined(ESP32)
void initSerial(String aBTClientName) {
    Serial.begin(aBTClientName, false);
}
#else
void initSerial(uint32_t aBaudRate) {
#  ifdef USE_SIMPLE_SERIAL
    initSimpleSerial(aBaudRate);
#  else
    Serial.begin(aBaudRate);
#  endif
}
#endif

#ifdef USE_SIMPLE_SERIAL
#  ifdef LOCAL_DISPLAY_EXISTS
void initSimpleSerial(uint32_t aBaudRate, bool aUsePairedPin) {
    if (aUsePairedPin) {
        pinMode(PAIRED_PIN, INPUT);
    }
#  else
void initSimpleSerial(uint32_t aBaudRate) {
#  endif // LOCAL_DISPLAY_EXISTS
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

/**
 * ultra simple blocking USART send routine - works 100%!
 */
void sendUSART(char aChar) {
    // wait for buffer to become empty
#  if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__) || defined(ARDUINO_AVR_LEONARDO) || defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__)
    // Use TX1 on MEGA and on Leonardo, which has no TX0
    while (!((UCSR1A) & (1 << UDRE1))) {
        ;
    }
    UDR1 = aChar;
#  else
    while (!((UCSR0A) & (1 << UDRE0))) {
        ;
    }
    UDR0 = aChar;
#  endif // Atmega...
}

//void USART_send(char aChar) {
//    sendUSART(aChar);
//}

void sendUSART(const char * aString) {
    while (*aString != '0') {
        sendUSART(*aString);
        aString++;
    }

}
#endif // USE_SIMPLE_SERIAL

/**
 * On Atmega328
 * TX of USART0 is port D1
 * RX is port D0
 */
/*
 * RECEIVE BUFFER
 */
#define RECEIVE_TOUCH_OR_DISPLAY_DATA_SIZE 4
//Buffer for 12 bytes since no need for length and eventType and SYNC_TOKEN be stored
uint8_t sReceiveBuffer[RECEIVE_MAX_DATA_SIZE];
uint8_t sReceiveBufferIndex = 0; // Index of first free position in buffer
bool sReceiveBufferOutOfSync = false;

/**
 * The central point for sending bytes
 */
void sendUSARTBufferNoSizeCheck(uint8_t * aParameterBufferPointer, int aParameterBufferLength, uint8_t * aDataBufferPointer,
        int16_t aDataBufferLength) {
#if ! defined(USE_SIMPLE_SERIAL)
    Serial.write(aParameterBufferPointer, aParameterBufferLength);
    Serial.write(aDataBufferPointer, aDataBufferLength);
#else
/*
 * Simple and reliable blocking version for Atmega328
 */
    while (aParameterBufferLength > 0) {
        // wait for USART send buffer to become empty
#  if (defined(UCSR1A) && ! defined(USE_USB_SERIAL)) || ! defined (UCSR0A) // Use TX1 on MEGA and on Leonardo, which has no TX0
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
#  if (defined(UCSR1A) && ! defined(USE_USB_SERIAL)) || ! defined (UCSR0A) // Use TX1 on MEGA and on Leonardo, which has no TX0
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

/**
 * send:
 * 1. Sync Byte A5
 * 2. Byte Function token
 * 3. Short length of parameters (here 5*2)
 * 4. Short n parameters
 */
// using this function saves 300 bytes for SimpleDSO
void sendUSART5Args(uint8_t aFunctionTag, uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd, uint16_t aColor) {
    uint16_t tParamBuffer[MAX_NUMBER_OF_ARGS_FOR_BD_FUNCTIONS];

    uint16_t * tBufferPointer = &tParamBuffer[0];
    *tBufferPointer++ = aFunctionTag << 8 | SYNC_TOKEN; // add sync token
    *tBufferPointer++ = 10; // parameter length
    *tBufferPointer++ = aXStart;
    *tBufferPointer++ = aYStart;
    *tBufferPointer++ = aXEnd;
    *tBufferPointer++ = aYEnd;
    *tBufferPointer++ = aColor;
    sendUSARTBufferNoSizeCheck((uint8_t*) &tParamBuffer[0], 14, NULL, 0);
}

/**
 *
 * @param aFunctionTag
 * @param aNumberOfArgs currently not more than 12 args (SHORT) are supported
 */
void sendUSARTArgs(uint8_t aFunctionTag, int aNumberOfArgs, ...) {
    if (aNumberOfArgs > MAX_NUMBER_OF_ARGS_FOR_BD_FUNCTIONS) {
        return;
    }

    uint16_t tParamBuffer[MAX_NUMBER_OF_ARGS_FOR_BD_FUNCTIONS + 2];
    va_list argp;
    uint16_t * tBufferPointer = &tParamBuffer[0];
    *tBufferPointer++ = aFunctionTag << 8 | SYNC_TOKEN; // add sync token
    va_start(argp, aNumberOfArgs);

    *tBufferPointer++ = aNumberOfArgs * 2;
    for (uint8_t i = 0; i < aNumberOfArgs; ++i) {
        *tBufferPointer++ = va_arg(argp, int);
    }
    va_end(argp);
    sendUSARTBufferNoSizeCheck((uint8_t*) &tParamBuffer[0], aNumberOfArgs * 2 + 4, NULL, 0);
}

/**
 *
 * @param aFunctionTag
 * @param aNumberOfArgs currently not more than 12 args (SHORT) are supported
 * Last two arguments are length of buffer and buffer pointer (..., size_t aDataLength, uint8_t * aDataBufferPtr)
 */
void sendUSARTArgsAndByteBuffer(uint8_t aFunctionTag, int aNumberOfArgs, ...) {
    if (aNumberOfArgs > MAX_NUMBER_OF_ARGS_FOR_BD_FUNCTIONS) {
        return;
    }

    uint16_t tParamBuffer[MAX_NUMBER_OF_ARGS_FOR_BD_FUNCTIONS + 4];
    va_list argp;
    uint16_t * tBufferPointer = &tParamBuffer[0];
    *tBufferPointer++ = aFunctionTag << 8 | SYNC_TOKEN; // add sync token
    va_start(argp, aNumberOfArgs);

    *tBufferPointer++ = aNumberOfArgs * 2;
    for (uint8_t i = 0; i < aNumberOfArgs; ++i) {
        *tBufferPointer++ = va_arg(argp, int);
    }
    // add data field header
    *tBufferPointer++ = DATAFIELD_TAG_BYTE << 8 | SYNC_TOKEN; // start new transmission block
    uint16_t tLength = va_arg(argp, int); // length in byte
    *tBufferPointer++ = tLength;
    uint8_t * aBufferPtr = (uint8_t *) va_arg(argp, int); // Buffer address
    va_end(argp);

    sendUSARTBufferNoSizeCheck((uint8_t*) &tParamBuffer[0], aNumberOfArgs * 2 + 8, aBufferPtr, tLength);
}

/**
 * Read message in buffer for one event.
 * After RECEIVE_BUFFER_SIZE bytes check if SYNC_TOKEN was sent.
 * If OK then interpret content and reset buffer.
 */
static uint8_t sReceivedEventType = EVENT_NO_EVENT;
static uint8_t sReceivedDataSize;

#ifdef USE_SIMPLE_SERIAL
bool allowTouchInterrupts = false; // !!do not enable it, if event handling may take more time than receiving a byte (which results in buffer overflow)!!!

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
                        struct BluetoothEvent * tRemoteTouchEventPtr = &remoteEvent;
#  ifndef DO_NOT_NEED_BASIC_TOUCH
                        if (sReceivedEventType == EVENT_TOUCH_ACTION_DOWN
                                || (remoteTouchDownEvent.EventType == EVENT_NO_EVENT && remoteEvent.EventType == EVENT_NO_EVENT)) {
                            tRemoteTouchEventPtr = &remoteTouchDownEvent;
                        }
#  endif
                        tRemoteTouchEventPtr->EventType = sReceivedEventType;
                        // copy buffer to structure
                        memcpy(tRemoteTouchEventPtr->EventData.ByteArray, sReceiveBuffer, sReceivedDataSize);
                        sReceiveBufferIndex = 0;
                        sReceivedEventType = EVENT_NO_EVENT;

                        if (allowTouchInterrupts) {
                            // Dangerous, it blocks receive event as long as event handling goes on!!!
                            handleEvent(tRemoteTouchEventPtr);
                        }
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
#else // USE_SIMPLE_SERIAL

/*
 * Will be called after each loop() (by Arduino Serial...) to process input data if available.
 */
void serialEvent(void) {
    if (sReceiveBufferOutOfSync) {
// just wait for next sync token
        while (Serial.available() > 0) {
            if (Serial.read() == SYNC_TOKEN) {
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
        uint8_t tBytesAvailable = Serial.available();
        /*
         * enough bytes available for next step?
         */
        if (sReceivedEventType == EVENT_NO_EVENT) {
            if (tBytesAvailable >= 2) {
                /*
                 * read message length and event tag first
                 */
                Serial.readBytes((char *) sReceiveBuffer, 2);
                // First byte is raw length so subtract 3 for sync+eventType+length bytes
                sReceivedDataSize = sReceiveBuffer[0] - 3;
                if (sReceivedDataSize > RECEIVE_MAX_DATA_SIZE) {
                    // invalid length
                    sReceiveBufferOutOfSync = true;
                    return;
                }
                sReceivedEventType = sReceiveBuffer[1];
                tBytesAvailable -= 2;
            }
        }
        if (sReceivedEventType != EVENT_NO_EVENT) {
            if (tBytesAvailable > sReceivedDataSize) {
                // Event complete received, now read data and sync token
                Serial.readBytes((char *) sReceiveBuffer, sReceivedDataSize);
                if (Serial.read() == SYNC_TOKEN) {
                    remoteEvent.EventType = sReceivedEventType;
                    // copy buffer to structure
                    memcpy(remoteEvent.EventData.ByteArray, sReceiveBuffer, sReceivedDataSize);
                    sReceivedEventType = EVENT_NO_EVENT;
                    handleEvent(&remoteEvent);
                } else {
                    sReceiveBufferOutOfSync = true;
                }
            }
        }
    }
}
#endif // USE_SIMPLE_SERIAL
