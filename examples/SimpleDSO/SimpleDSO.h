/*
 * SimpleDSO.h
 *
 *  Copyright (C) 2015-2023  Armin Joachimsmeyer
 *  Email: armin.joachimsmeyer@gmail.com
 *
 *  This file is part of Arduino-Simple-DSO https://github.com/ArminJo/Arduino-Simple-DSO.
 *
 *  Arduino-Simple-DSO is free software: you can redistribute it and/or modify
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

#ifndef _SIMPLE_TOUCHSCREEN_DSO_H
#define _SIMPLE_TOUCHSCREEN_DSO_H

#include "TouchDSOCommon.h"

// Internal version
#define VERSION_DSO "3.2"
/*
 * Version 3.2 - 11/2019
 * - Clear data buffer at start and at switching inputs.
 * - Multiline button caption.
 *
 * Version 3.1
 * - Stop response improved for fast mode.
 * - Value computation for ultra fast modes fixed.
 * - millis() timer compensation formula fixed.
 * - AC/DC button and info line handling improved.
 */

/****************************************************************************
 * Change this if you have reprogrammed the hc05 module for other baud rate
 ***************************************************************************/
#if !defined(BLUETOOTH_BAUD_RATE)
//#define BLUETOOTH_BAUD_RATE BAUD_115200
#define BLUETOOTH_BAUD_RATE BAUD_9600
#endif

#define MILLIS_BETWEEN_INFO_OUTPUT 1000

#define THOUSANDS_SEPARATOR '.'

/*
 * Pins on port B
 */
#define OUTPUT_MASK_PORTB   0X2C
#define ATTENUATOR_DETECT_PIN_0 8 // PortB0 in INPUT_PULLUP mode
#define ATTENUATOR_DETECT_PIN_1 9 // PortB1 in INPUT_PULLUP mode
#define TIMER_1_OUTPUT_PIN 10 // Frequency generation OC1B TIMER1
#define VEE_PIN 11 // // PortB3 OC2A TIMER2 Square wave for VEE (-5 volt) generation
#define DEBUG_PIN 13 // PortB5 PIN 13

/*
 * Pins on port B
 */
#define OUTPUT_MASK_PORTC   0X20 // PC5 ADC5
#define AC_DC_BIAS_PIN A5 // PC5 ADC5

/*
 * Pins on port D
 * AC/DC, attenuator control and external trigger input
 * !!! Pin layout only for Atmega328 !!!
 */
#define CONTROL_PORT PORTD
#define CONTROL_DDR  DDRD
#define EXTERN_TRIGGER_INPUT_PIN 2// PD2

// Control
#define ATTENUATOR_SHIFT 4 // Bit 4+5
#define ATTENUATOR_MASK 0x30 // Bit 4+5
#define OUTPUT_MASK_PORTD    0xF0 // Bit 4-7
#define ATTENUATOR_0_PIN 4 // PD4
#define ATTENUATOR_1_PIN 5 // PD5
#define AC_DC_RELAY_PIN 6 // PD6

// GUI element colors

#define COLOR_SLIDER RGB(0xD0,0xD0,0xD0)

// No trigger wait timeout for modes != TRIGGER_DELAY_NONE
#define TRIGGER_DELAY_NONE 0
#define TRIGGER_DELAY_MICROS 1
#define TRIGGER_DELAY_MILLIS 2

#define TRIGGER_DELAY_MICROS_POLLING_ADJUST_COUNT 1 // estimated value to be subtracted from value because of fast mode initial delay
#define TRIGGER_DELAY_MICROS_ISR_ADJUST_COUNT 4 // estimated value to be subtracted from value because of ISR initial delay

// States of tTriggerStatus
#define TRIGGER_STATUS_START 0 // No trigger condition met
#define TRIGGER_STATUS_AFTER_HYSTERESIS 1 // slope and hysteresis condition met, wait to go beyond trigger level without hysteresis.
#define TRIGGER_STATUS_FOUND 2 // Trigger condition met - Used for shorten ISR handling
#define TRIGGER_STATUS_FOUND_AND_WAIT_FOR_DELAY 3 // Trigger condition met and waiting for ms delay

/*
 * External attenuator values
 */
#define ATTENUATOR_TYPE_NO_ATTENUATOR       0 // No attenuator at all. Start with aRef = VCC -> see ATTENUATOR_DETECT_PIN_0
#define ATTENUATOR_TYPE_FIXED_ATTENUATOR    1 // Fixed attenuator at Channel0, 1, 2
#define ATTENUATOR_TYPE_ACTIVE_ATTENUATOR   2 // to be developed
#define NUMBER_OF_CHANNEL_WITH_ACTIVE_ATTENUATOR 2

struct MeasurementControlStruct {
    // State
    bool isRunning;
    bool StopRequested;
    // Used to disable trigger timeout and to specify full buffer read with stop after first read.
    bool isSingleShotMode;

    float VCC; // Volt of VCC
    uint8_t ADCReferenceShifted; // DEFAULT = 1 =VCC   INTERNAL = 3 = 1.1 volt

    // Input select
    uint8_t ADMUXChannel;
    uint8_t AttenuatorType; //ATTENUATOR_TYPE_NO_ATTENUATOR, ATTENUATOR_TYPE_SIMPLE_ATTENUATOR, ATTENUATOR_TYPE_ACTIVE_ATTENUATOR
    bool ChannelHasActiveAttenuator;

    // AC / DC Switch
    bool ChannelHasAC_DCSwitch; // has AC / DC switch - only for channels with active or passive attenuators. Is at least false for TEMP and REF channels
    bool ChannelIsACMode;   // AC Mode for actual channel
    bool isACMode; // storage for user AC mode setting used to restore AC mode when switching back from channel without AC capabilities.
    uint16_t RawDSOReadingACZero;

    // Trigger
    bool TriggerSlopeRising;
    uint16_t RawTriggerLevel;
    uint16_t RawTriggerLevelHysteresis; // The RawTriggerLevel +/- hysteresis depending on slope (- for TriggerSlopeRising) - Used for computeMicrosPerPeriod()
    uint16_t RawHysteresis;             // quarter of peak to peak value
    uint16_t ValueBeforeTrigger;

    uint32_t TriggerDelayMillisEnd; // value of millis() at end of milliseconds trigger delay
    uint16_t TriggerDelayMillisOrMicros;
    uint8_t TriggerDelayMode; //  TRIGGER_DELAY_NONE 0, TRIGGER_DELAY_MICROS 1, TRIGGER_DELAY_MILLIS 2. Threshold is  UINT16_MAX

    // Using type TriggerMode instead of uint8_t increases program size by 76 bytes
    uint8_t TriggerMode; // adjust values automatically
    uint8_t OffsetMode; //OFFSET_MODE_0_VOLT, OFFSET_MODE_AUTOMATIC, OFFSET_MODE_MANUAL
    uint8_t TriggerStatus; //TRIGGER_STATUS_START 0, TRIGGER_STATUS_BEFORE_THRESHOLD 1, TRIGGER_STATUS_OK 2
    uint8_t TriggerSampleCountPrecaler; // for dividing sample count by 256 - to avoid 32bit variables in ISR
    uint16_t TriggerSampleCountDividedBy256; // for trigger timeout
    uint16_t TriggerTimeoutSampleCount; // ISR max samples before trigger timeout. Used only for trigger modes with timeout.

    // Statistics (for info and auto trigger)
    uint16_t RawValueMin;
    uint16_t RawValueMax;
    uint16_t ValueMinForISR;
    uint16_t ValueMaxForISR;
    uint16_t ValueAverage;
    uint32_t IntegrateValueForAverage;
    uint32_t PeriodMicros;
    uint32_t PeriodFirst; // Length of first pulse or pause
    uint32_t PeriodSecond; // Length of second pulse or pause
    uint32_t FrequencyHertz;

    // Timebase
    bool AcquisitionFastMode;
    uint8_t TimebaseIndex;
    uint8_t TimebaseHWValue;

    bool RangeAutomatic; // RANGE_MODE_AUTOMATIC, MANUAL

    // Shift and scale
    uint16_t OffsetValue;
    uint8_t AttenuatorValue; // 0 for direct input or channels without attenuator, 1 -> factor 10, 2 -> factor 100, 3 -> input shortcut
    uint8_t ShiftValue; // shift (division) value  (0-2) for different voltage ranges
    uint16_t HorizontalGridSizeShift8; // depends on shift  for 5 volt reference 0,02 -> 41 other -> 51.2
    float HorizontalGridVoltage; // voltage per grid for offset etc.
    int8_t OffsetGridCount; // number of bottom line for offset != 0 volt.
    uint32_t TimestampLastRangeChange;
};

extern struct MeasurementControlStruct MeasurementControl;

// values for DisplayPage
// using enums increases code size by 120 bytes for Arduino
#define DSO_PAGE_START      0    // Start GUI
#define DSO_PAGE_CHART      1    // Chart in analyze and running mode
#define DSO_PAGE_SETTINGS   2
#define DSO_PAGE_FREQUENCY  3
#ifndef AVR
#define DSO_PAGE_MORE_SETTINGS 4
#define DSO_PAGE_SYST_INFO  5
#endif

#define DSO_SUB_PAGE_MAIN   0
#define DSO_SUB_PAGE_FFT    1

// modes for showInfoMode
#define INFO_MODE_NO_INFO 0
#define INFO_MODE_SHORT_INFO 1
#define INFO_MODE_LONG_INFO 2
struct DisplayControlStruct {
    uint8_t TriggerLevelDisplayValue; // For clearing old line of manual trigger level setting
    int8_t XScale; // Factor for X Data expansion(>0)  0 = no scale, 2->display 1 value 2 times etc.

    uint8_t DisplayPage; // START, CHART, SETTINGS, MORE_SETTINGS
    uint8_t DisplaySubPage; // DSO_SUB_PAGE_FFT

    bool DrawWhileAcquire;
    uint8_t showInfoMode;

    bool showHistory;
    uint16_t EraseColor;
};
extern DisplayControlStruct DisplayControl;

/*
 * Data buffer
 */
struct DataBufferStruct {
    uint8_t DisplayBuffer[DISPLAY_WIDTH];
    uint8_t * DataBufferNextInPointer;
    uint8_t * DataBufferNextDrawPointer; // pointer to DataBuffer - for draw-while-acquire mode
    uint16_t DataBufferNextDrawIndex; // index in DisplayBuffer - for draw-while-acquire mode
    // to detect end of acquisition in interrupt service routine
    uint8_t * DataBufferEndPointer;
    // Used to synchronize ISR with main loop
    bool DataBufferFull;
    // AcqusitionSize is DISPLAY_WIDTH except on last acquisition before stop then it is DATABUFFER_SIZE
    uint16_t AcquisitionSize;
    // Pointer for horizontal scrolling
    uint8_t * DataBufferDisplayStart;
    uint8_t DataBuffer[DATABUFFER_SIZE]; // contains also display values i.e. (DISPLAY_VALUE_FOR_ZERO - 8BitValue)
};
extern DataBufferStruct DataBufferControl;

// Utility section
uint16_t getInputRawFromDisplayValue(uint8_t aDisplayValue);
float getFloatFromDisplayValue(uint8_t aDisplayValue);
void printSingleshotMarker();
void clearSingleshotMarker();
extern "C" void INT0_vect();

// for printf etc.
#if defined(AVR)
#define SIZEOF_STRINGBUFFER 50
#else
#define SIZEOF_STRINGBUFFER 240
#endif
extern char sStringBuffer[SIZEOF_STRINGBUFFER];

extern BDButton TouchButtonBack;
// global flag for page control. Is evaluated by calling loop or page and set by buttonBack handler
extern bool sBackButtonPressed;

#endif // _SIMPLE_TOUCHSCREEN_DSO_H
