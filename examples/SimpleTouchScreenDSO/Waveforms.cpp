/*
 * Waveforms.cpp
 *
 * Code uses 16 bit AVR Timer1 and generates a 62.5 kHz PWM signal with 8 Bit resolution.
 * After every PWM cycle an interrupt handler sets a new PWM value, resulting in a sine, triangle or sawtooth output.
 * New value is taken by a rolling index from a table for sine, or directly computed from that index for triangle and sawtooth waveforms.
 *
 * Maximum values:                                                          Minimum values:
 * SINE: clip to minimum 8 samples per period => 128 us / 7812.5 Hz           7,421 mHz
 * SAWTOOTH: clip to minimum 16 samples per period => 256 us / 3906.25 Hz     3.725 mHz
 * TRIANGLE: clip to minimum 32 samples per period => 512 us / 1953.125 Hz    1.866 mHz
 * By using a "floating point" index increment, every frequency lower than these maximum values can be generated.
 *
 * In CTC Mode Timer1 generates square wave from 0.119 Hz up to 8 MHz (full range of Timer1).
 * Timer1 is used by Arduino for Servo Library. For 8 bit resolution it may also be possible to use Timer2 which is used for Arduino tone().
 *
 * Output is at PIN 10
 *
 * PWM RC-Filter suggestions
 * Simple: 2k2 Ohm and 100 nF
 * 2nd order (good for sine and triangle): 1 kOhm and 100 nF -> 4k7 Ohm and 22 nF
 * 2nd order (better for sawtooth):        1 kOhm and 22 nF  -> 4k7 Ohm and 4.7 nF
 *
 *  Copyright (C) 2017  Armin Joachimsmeyer
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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#include <Arduino.h>
#include "Waveforms.h"

#define TIMER_PRESCALER_MASK 0x07

struct FrequencyInfoStruct sFrequencyInfo;

/*
 * Sine table from 0 to 90 degree including 0 AND 90 degree therefore we have an odd number
 * Contains values from 128 to 255 (or if inverted: 1 to 128)
 */
#define SIZE_OF_SINE_TABLE_QUARTER 32
const uint8_t sSineTableQuarter128[SIZE_OF_SINE_TABLE_QUARTER + 1] PROGMEM = { 128, 135, 141, 147, 153, 159, 165, 171, 177, 182,
        188, 193, 199, 204, 209, 213, 218, 222, 226, 230, 234, 237, 240, 243, 245, 248, 250, 251, 253, 254, 254, 255, 255 };
// Base period, for which exact one next value from table/computation is taken at every interrupt
// 8 Bit PWM resolution gives 488.28125 Hz sine base frequency: 1/16 us * 256 * 128 = 16*128 = 2048 us = 488.28125 Hz
#define BASE_PERIOD_MICROS_FOR_SINE_TABLE 2048UL // ((1/F_CPU) * PWM_RESOLUTION) * (SIZE_OF_SINE_TABLE_QUARTER * 4)
#define BASE_PERIOD_MICROS_FOR_TRIANGLE 8176UL // (1/F_CPU) * PWM_RESOLUTION * (256+255) Values -> 122.3092 Hz
#define BASE_PERIOD_MICROS_FOR_SAWTOOTH 4096UL // (1/F_CPU) * PWM_RESOLUTION * 256 Values -> 244.140625 Hz

const char FrequencyFactorChars[4] = { 'm', ' ', 'k', 'M' };

/*
 * 8-bit PWM Output at PIN 10
 * Overflow interrupt is generated every cycle -> this is used to generate the waveforms
 */
void initTimer1For8BitPWM() {
    DDRB |= _BV(DDB2); // set pin OC1B = PortB2 -> PIN 10 to output direction

    TCCR1A = _BV(COM1B1) | _BV(WGM10); // Clear OC1B on Compare Match.  With WGM12 Waveform Generation Mode 5 - Fast PWM 8-bit,
    //    TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11); // With WGM12 Waveform Generation Mode 6 - Fast PWM, 9-bit
    //    TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11) | _BV(WGM10); // With WGM12 Waveform Generation Mode 7 - Fast PWM, 10-bit
    TCCR1B = _BV(WGM12); // set OC1A/OC1B at BOTTOM (non-inverting mode) - no clock (prescaler) -> timer disabled now

    OCR1A = UINT8_MAX;   // output DC - HIGH
    OCR1B = UINT8_MAX;   // output DC - HIGH
    TCNT1 = 0;      // init counter
    TIMSK1 = _BV(TOIE1); // Enable Overflow Interrupt
}

/*
 * CTC output at PIN 10
 */
void initTimer1ForCTC(void) {
    DDRB |= _BV(DDB2); // set pin OC1B = PortB2 -> PIN 10 to output direction

    TIMSK1 = 0; // no interrupts

    TCCR1A = _BV(COM1B0); // Toggle OC1B on compare match / CTC mode
    TCCR1B = _BV(WGM12); // CTC with OCR1A - no clock->timer disabled
    OCR1A = 125 - 1; // set compare match register for 1 kHz
    TCNT1 = 0; // init counter
}

void setWaveformMode(uint8_t aNewMode) {
    aNewMode &= WAVEFORM_MASK;
    sFrequencyInfo.Waveform = aNewMode;
    if (aNewMode == WAVEFORM_SQUARE) {
        initTimer1ForCTC();
    } else {
        initTimer1For8BitPWM();
    }
    // start timer if not already done
    startWaveform();
    // recompute values
    setWaveformFrequency();
}

void cycleWaveformMode() {
    setWaveformMode(sFrequencyInfo.Waveform + 1);
}

const char * cycleWaveformModePGMString() {
    cycleWaveformMode();
    return getWaveformModePGMString();
}

const char * getWaveformModePGMString() {
    const char *tResultString;
    tResultString = PSTR("Square");
    if (sFrequencyInfo.Waveform == WAVEFORM_SINE) {
        tResultString = PSTR("Sine");
    } else if (sFrequencyInfo.Waveform == WAVEFORM_TRIANGLE) {
        tResultString = PSTR("Triangle");
    } else if (sFrequencyInfo.Waveform == WAVEFORM_SAWTOOTH) {
        tResultString = PSTR("Sawtooth");
    }
    return tResultString;
}

float getPeriodMicros() {
    // output period use float, since we have 1/8 us for square wave
    float tPeriodMicros;
    if (sFrequencyInfo.Waveform == WAVEFORM_SQUARE) {
        // use better resolution here
        tPeriodMicros = sFrequencyInfo.ControlValue.DividerInt;
        tPeriodMicros /= 8;
    } else {
        tPeriodMicros = sFrequencyInfo.PeriodMicros;
    }
    return tPeriodMicros;
}

void setNormalizedFrequencyFactor(int aIndexValue) {
    sFrequencyInfo.FrequencyFactorIndex = aIndexValue;
    uint32_t tFactor = 1;
    while (aIndexValue >= 1) {
        tFactor *= 1000;
        aIndexValue--;
    }
    sFrequencyInfo.FrequencyFactorTimes1000 = tFactor;
}

/*
 * Set display values sFrequencyNormalized and sFrequencyFactorIndex
 *
 * Problem is set e.g. value of 1 Hz as 1000 mHz or 1 Hz?
 * so we just try to keep the existing range.
 * First put value of 1000 to next range,
 * then undo if value < 1.00001 and existing range is one lower
 */
void setNormalizedFrequencyAndFactor(float aValue) {
    uint8_t tFrequencyFactorIndex = 1;
    // normalize Frequency to 1 - 1000 and compute FrequencyFactorIndex
    if (aValue < 1) {
        tFrequencyFactorIndex = 0; //mHz
        aValue *= 1000;
    } else {
        // 1000.1 to avoid switching to next range because of resolution issues
        while (aValue >= 1000) {
            aValue /= 1000;
            tFrequencyFactorIndex++;
        }
    }

    // check if tFrequencyFactorIndex - 1 fits better
    if (aValue < 1.00001 && sFrequencyInfo.FrequencyFactorIndex == (tFrequencyFactorIndex - 1)) {
        aValue *= 1000;
        tFrequencyFactorIndex--;
    }

    setNormalizedFrequencyFactor(tFrequencyFactorIndex);
    sFrequencyInfo.FrequencyNormalized = aValue;
}

bool setWaveformFrequency() {
    return setWaveformFrequency((sFrequencyInfo.FrequencyNormalized * sFrequencyInfo.FrequencyFactorTimes1000) / 1000);
}
/*
 * SINE: clip to minimum 8 samples per period => 128 us / 7812.5 Hz
 * SAWTOOTH: clip to minimum 16 samples per period => 256 us / 3906.25 Hz
 * Triangle: clip to minimum 32 samples per period => 512 us / 1953.125 Hz
 * return true if clipping occurs
 */
bool setWaveformFrequency(float aFrequency) {
    bool hasError = false;
    if (sFrequencyInfo.Waveform == WAVEFORM_SQUARE) {
        // need initialized sFrequencyInfo structure
        hasError = setSquareWaveFrequency(aFrequency);
    } else {
        // use shift 16 to increase resolution but avoid truncation
        long tBasePeriodShift16 = (BASE_PERIOD_MICROS_FOR_SINE_TABLE << 16);
        if (sFrequencyInfo.Waveform == WAVEFORM_TRIANGLE) {
            tBasePeriodShift16 = (BASE_PERIOD_MICROS_FOR_TRIANGLE << 16);
        } else if (sFrequencyInfo.Waveform == WAVEFORM_SAWTOOTH) {
            tBasePeriodShift16 = (BASE_PERIOD_MICROS_FOR_SAWTOOTH << 16);
        }
        uint32_t tPeriodMicros = 1000000UL / aFrequency;
        uint32_t tBaseFrequencyFactorShift16 = tBasePeriodShift16 / tPeriodMicros;
        if (tBaseFrequencyFactorShift16 > (16L << 16)) {
            // Clip at factor 16 (taking every 16th value) and recompute values
            tBaseFrequencyFactorShift16 = (16L << 16);
            tPeriodMicros = (tBasePeriodShift16 >> 16) / 16;
            hasError = true;
        } else if (tBaseFrequencyFactorShift16 < 1) {
            tBaseFrequencyFactorShift16 = 1;
            tPeriodMicros = tBasePeriodShift16;
            hasError = true;
        }
        // recompute values
        sFrequencyInfo.Frequency = 1000000.0 / tPeriodMicros;
        sFrequencyInfo.PeriodMicros = tPeriodMicros;
        sFrequencyInfo.ControlValue.BaseFrequencyFactorShift16 = tBaseFrequencyFactorShift16;

        sFrequencyInfo.PrescalerRegisterValueBackup = 1;
        if (sFrequencyInfo.isOutputEnabled) {
            // start Timer1 for PWM generation
            TCCR1B &= ~TIMER_PRESCALER_MASK;
            TCCR1B |= _BV(CS10); // set prescaler to 1 -> gives 16 us / 62.5 kHz PWM
        }
    }
    setNormalizedFrequencyAndFactor(sFrequencyInfo.Frequency);
    return hasError;
}

bool setSquareWaveFrequency(float aFrequency) {
    bool hasError = false;
    float tFrequency = aFrequency;
    /*
     * Timer runs in toggle mode and has 8 MHz / 0.125 us maximum frequency
     * Divider = (F_CPU/2) / sFrequency
     * Divider= 1, prescaler= 1 => 8 MHz
     * Divider= 16348 * prescaler= 1024 = 0x200000000 => 8,388,608 us => 0.119209 Hz
     */
    uint32_t tDividerInteger = (F_CPU / 2) / tFrequency;
    if (tDividerInteger == 0) {
        if (tFrequency < 1) {
            // for very small frequencies (F_CPU / 2) / tFrequency gives NaN which results in 0
            tDividerInteger = 0x10000 * 1024; // maximum divider
        } else {
            // 8 MHz / 0.125 us is maximum
            hasError = true;
            tDividerInteger = 1;
            tFrequency = 8;
        }
    }

    /*
     * Determine prescaler and PrescalerRegisterValue from tDividerInteger value,
     * in order to get an tDividerInteger value <= 0x10000 (register value is tDividerInteger-1)
     */
    uint16_t tPrescaler = 1; // direct clock
    uint8_t tPrescalerRegisterValue = 1;
    if (tDividerInteger > 0x10000) {
        tDividerInteger >>= 3;
        if (tDividerInteger <= 0x10000) {
            tPrescaler = 8;
            tPrescalerRegisterValue = 2;
        } else {
            tDividerInteger >>= 3;
            if (tDividerInteger <= 0x10000) {
                tPrescaler = 64;
                tPrescalerRegisterValue = 3;
            } else {
                tDividerInteger >>= 2;
                if (tDividerInteger <= 0x10000) {
                    tPrescaler = 256;
                    tPrescalerRegisterValue = 4;
                } else {
                    tDividerInteger >>= 2;
                    tPrescaler = 1024;
                    tPrescalerRegisterValue = 5;
                    if (tDividerInteger > 0x10000) {
                        // clip to 16 bit value
                        tDividerInteger = 0x10000;
                    }
                }
            }
        }
    }
    sFrequencyInfo.PrescalerRegisterValueBackup = tPrescalerRegisterValue;
    if (sFrequencyInfo.isOutputEnabled) {
        // set values to timer register
        TCCR1B &= ~TIMER_PRESCALER_MASK;
        TCCR1B |= tPrescalerRegisterValue;
    }
    OCR1A = tDividerInteger - 1; // set compare match register

    /*
     * recompute exact period and frequency for eventually changed 16 bit period
     * Frequency = (F_CPU/2) / (DividerInt * Prescaler)
     */
    tDividerInteger *= tPrescaler;

    tFrequency = ((float) (F_CPU / 2)) / tDividerInteger;
    /*
     * Save values
     */
    sFrequencyInfo.Frequency = tFrequency;
    sFrequencyInfo.ControlValue.DividerInt = tDividerInteger;
    sFrequencyInfo.PeriodMicros = tDividerInteger / 8;
    return hasError;
}

void stopWaveform() {
// set prescaler choice to 0 -> timer stops
    TCCR1B &= ~TIMER_PRESCALER_MASK;
}

void startWaveform() {
    TCCR1B &= ~TIMER_PRESCALER_MASK;
    TCCR1B |= sFrequencyInfo.PrescalerRegisterValueBackup;
}

//Timer1 overflow interrupt vector handler
ISR(TIMER1_OVF_vect) {

    static int8_t sSineTableIndex = 0;
    static uint8_t sNumberOfQuadrant = 0;
    static uint8_t sNextOcrbValue = 0;

// output value at start of ISR to avoid jitter
    OCR1B = sNextOcrbValue;
    /*
     * Increase index by sBaseFrequencyFactor.
     * In order to avoid floating point arithmetic in ISR, use sBaseFrequencyFactorShift16 and handle resulting residual.
     */
    int8_t tIndexDelta = sFrequencyInfo.ControlValue.BaseFrequencyFactorShift16 >> 16;
// handle fraction of frequency factor
    sFrequencyInfo.BaseFrequencyFactorAccumulator += sFrequencyInfo.ControlValue.BaseFrequencyFactorShift16 & 0xFFFF;
    if (sFrequencyInfo.BaseFrequencyFactorAccumulator > 0x8000) {
        /*
         * Accumulated fraction is bigger than "half" so increase index
         */
        tIndexDelta++;
        sFrequencyInfo.BaseFrequencyFactorAccumulator -= 0x10000;
    }
    if (tIndexDelta > 0) {
        uint8_t tNumberOfQuadrant = sNumberOfQuadrant;
        if (sFrequencyInfo.Waveform == WAVEFORM_SINE) {
            uint8_t tQuadrantIncrease = 0;
            switch (tNumberOfQuadrant) {
            case 0: // [0,90) Degree, including 0, not including 90 Degree
            case 2: // [180,270) Degree
                sSineTableIndex += tIndexDelta;
                if (sSineTableIndex >= SIZE_OF_SINE_TABLE_QUARTER) {
                    sSineTableIndex = SIZE_OF_SINE_TABLE_QUARTER - (sSineTableIndex - SIZE_OF_SINE_TABLE_QUARTER);
                    tQuadrantIncrease = 1;
                }
                break;
            case 1: // [90,180) Degree
            case 3: // [270,360) Degree
                sSineTableIndex -= tIndexDelta;
                if (sSineTableIndex <= 0) {
                    sSineTableIndex = -sSineTableIndex;
                    tQuadrantIncrease = 1;
                }
                break;
            }
            if (tNumberOfQuadrant & 0x02) {
                // case 2 and 3   -128 -> 128 ; -255 -> 1
                sNextOcrbValue = -(pgm_read_byte(&sSineTableQuarter128[sSineTableIndex]));
            } else {
                sNextOcrbValue = pgm_read_byte(&sSineTableQuarter128[sSineTableIndex]);
            }

            tNumberOfQuadrant = (tNumberOfQuadrant + tQuadrantIncrease) & 0x03;

            /*
             * the same as loop with variable delay
             */
            //    // [0,90)
            //    for (int i = 0; i < SIZE_OF_SINE_TABLE_QUARTER; ++i) {
            //        OCR1B = sSineTableQuarter128[i];
            //        delayMicroseconds(sDelay);
            //    }
            //    // [90,180)
            //    for (int i = SIZE_OF_SINE_TABLE_QUARTER; i > 0; i--) {
            //        OCR1B = sSineTableQuarter128[i];
            //        delayMicroseconds(sDelay);
            //    }
            //    // [180,270)
            //    for (int i = 0; i < SIZE_OF_SINE_TABLE_QUARTER; ++i) {
            //        OCR1B = -(sSineTableQuarter128[i]);
            //        delayMicroseconds(sDelay);
            //    }
            //    // [270,360)
            //    for (int i = SIZE_OF_SINE_TABLE_QUARTER; i > 0; i--) {
            //        OCR1B = -(sSineTableQuarter128[i]);
            //        delayMicroseconds(sDelay);
            //    }
        } else if (sFrequencyInfo.Waveform == WAVEFORM_TRIANGLE) {
            /*
             * Values 0 and FF are half as often as other values, so special treatment required
             * One period from 0 to 0 consists of 256 + 255 values!
             */
            uint8_t tOldOcrbValue = sNextOcrbValue;
            if (tNumberOfQuadrant == 0) {
                // Value from 1 to FF
                // increasing value
                sNextOcrbValue += tIndexDelta;
                // detect overflow (value > UINT8_MAX)
                if (sNextOcrbValue < tOldOcrbValue) {
                    tNumberOfQuadrant = 1;
                    // 0->FE, 1->FD
                    sNextOcrbValue = (~sNextOcrbValue) - 1;
                }
            } else {
                // decreasing value Value from FE to 0
                sNextOcrbValue -= tIndexDelta;
                // detect underflow
                if (sNextOcrbValue > tOldOcrbValue) {
                    tNumberOfQuadrant = 0;
                    // FF -> 1, FE -> 2
                    sNextOcrbValue = -sNextOcrbValue;
                }
            }

        } else if (sFrequencyInfo.Waveform == WAVEFORM_SAWTOOTH) {
            sNextOcrbValue += tIndexDelta;
        }
        sNumberOfQuadrant = tNumberOfQuadrant;

    }
}

/*
 * Use it if you need a different size of table e.g. to generate different frequencies or increase accuracy for low frequencies
 */
void computeSineTableValues(uint8_t aSineTable[], unsigned int aNumber) {
//
    float tRadianDelta = (M_PI * 2) / aNumber;
    float tRadian = 0.0;
// (i <= aNumber) in order to include value for 360 degree
    for (unsigned int i = 0; i < aNumber; ++i) {
        float tSineFloat = (sin(tRadian) * 127) + 128;
        aSineTable[i] = (tSineFloat + 0.5);
        tRadian += tRadianDelta;
    }
}
