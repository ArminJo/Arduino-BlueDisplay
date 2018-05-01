/*
 * Waveforms.cpp
 *
 * Code uses 16 bit AVR Timer1 and generates a 62.5 kHz PWM signal with 8 Bit resolution.
 * After every PWM every cycle an interrupt handler sets a new PWM value, resulting in a sine, triangle or sawtooth output.
 * New value is taken by a rolling index from a table, or directly computed from that index.
 * By using a "floating point" index increment, every frequency can be generated.
 *
 * In CTC Mode Timer1 generates square wave from 0.119 Hz up to 8 MHz (full range for Timer1).
 * Timer1 is used by Arduino for Servo Library. For 8 bit resolution you could also use Timer2 which is used for Arduino tone().
 *
 * Output is at PIN 10
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
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
// Base period, if exact one next value of table is taken at every interrupt
// 8 Bit PWM resolution gives 488.28125Hz sine base frequency: 1/16 us * 256 * 128 = 16*128 = 2048us = 488.28125Hz
#define BASE_PERIOD_FOR_SINE_TABLE 2048UL // ((1/F_CPU) * PWM_RESOLUTION) * (SIZE_OF_SINE_TABLE_QUARTER * 4)

#define BASE_PERIOD_FOR_TRIANGLE 8176UL // (1/F_CPU) * PWM_RESOLUTION * (256+255) Values -> 122.3092Hz
#define BASE_PERIOD_FOR_SAWTOOTH 4096UL // (1/F_CPU) * PWM_RESOLUTION * 256 Values -> 244.140625Hz

int8_t sSineTableIndex = 0;
uint8_t sNumberOfQuadrant = 0;
uint8_t sNextOcrbValue = 0;

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

    OCR1A = 0xFF;   // output DC - HIGH
    OCR1B = 0xFF;   // output DC - HIGH
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
    OCR1A = 125 - 1; // set compare match register for 1kHz
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
    const char * tResultString;
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

void setFrequency(float aValue) {
    uint8_t tIndex = 1;
    if (sFrequencyInfo.Waveform == WAVEFORM_SQUARE) {
        // normalize Frequency to 1 - 1000 and compute SquareWaveFrequencyFactor
        if (aValue < 1) {
            tIndex = 0; //mHz
            aValue *= 1000;
        } else {
            while (aValue > 1000) {
                aValue /= 1000;
                tIndex++;
            }
        }
    }
    /*
     * FrequencyFactor is not needed for PWM.
     * Set to one for PWM since it might be used for display of Frequency
     */
    setFrequencyFactor(tIndex);
    sFrequencyInfo.Frequency = aValue;
    setWaveformFrequency();
}

/*
 * SINE: clip to minimum 8 samples per period => 128us / 7812.5Hz
 * SAWTOOTH: clip to minimum 16 samples per period => 256us / 3906.25Hz
 * Triangle: clip to minimum 32 samples per period => 512us / 1953.125Hz
 */
bool setWaveformFrequency() {
    bool hasError = false;
    if (sFrequencyInfo.Waveform == WAVEFORM_SQUARE) {
        // need initialized sFrequencyInfo structure
        hasError = setSquareWaveFrequency();
    } else {
        // use shift 16 to increase resolution but avoid truncation
        long tBasePeriodShift16 = (BASE_PERIOD_FOR_SINE_TABLE << 16);
        if (sFrequencyInfo.Waveform == WAVEFORM_TRIANGLE) {
            tBasePeriodShift16 = (BASE_PERIOD_FOR_TRIANGLE << 16);
        } else if (sFrequencyInfo.Waveform == WAVEFORM_SAWTOOTH) {
            tBasePeriodShift16 = (BASE_PERIOD_FOR_SAWTOOTH << 16);
        }
        sFrequencyInfo.PeriodMicros = 1000000UL / sFrequencyInfo.Frequency;
        uint32_t tBaseFrequencyFactorShift16 = tBasePeriodShift16 / sFrequencyInfo.PeriodMicros;
        if (tBaseFrequencyFactorShift16 > (16L << 16)) {
            // Clip at factor 16 and recompute values
            tBaseFrequencyFactorShift16 = (16L << 16);
            sFrequencyInfo.PeriodMicros = (tBasePeriodShift16 >> 16) / 16;
            sFrequencyInfo.Frequency = 1000000UL / sFrequencyInfo.PeriodMicros;
            hasError = true;
        }
        sFrequencyInfo.ControlValue.sBaseFrequencyFactorShift16 = tBaseFrequencyFactorShift16;

        sFrequencyInfo.PrescalerRegisterValue = 1;
        if (sFrequencyInfo.isOutputEnabled) {
            // start Timer1 for PWM generation
            TCCR1B &= ~TIMER_PRESCALER_MASK;
            TCCR1B |= _BV(CS10); // set prescaler to 1 -> gives 16us / 62.5kHz PWM
        }
    }
    return hasError;
}

bool setSquareWaveFrequency() {
    bool hasError = false;
    float tFrequency = sFrequencyInfo.Frequency;
    /*
     * Timer runs in toggle mode and has 8 MHz maximum frequency
     * Divider = (F_CPU/2) / (sFrequency * (sFrequencyFactorTimes1000 / 1000)) = (F_CPU * 500) / (sFrequencyFactorTimes1000 * sFrequency)
     * Divider= 1, prescaler= 1 => 8 MHz
     * Divider= 16348 * prescaler= 1024 = 0x200000000 => 8,388,608us => 0.119209Hz
     *
     * But F_CPU * 500 does not fit in a 32 bit integer so use half of it which fits and compensate later
     */
    bool tFreqWasCompensated = false;
    uint32_t tDividerInt = ((F_CPU * 250) / sFrequencyInfo.FrequencyFactorTimes1000);
    if (tDividerInt > 0x7FFFFFFF) { // equivalent to if(FrequencyFactorTimes1000 == 1) but more than 10 bytes less program space
        // compensate frequency since divider is too big to compensate
        // in milliHertz range here
        if (tFrequency > 2) {
            /*
             * Values less than 1 are not correctly processed below. Values below 100 milliHertz makes no sense for AVR timer
             * and are corrected anyway by ComputePeriodAndSetTimer()
             */
            tFrequency /= 2;
        }
        tFreqWasCompensated = true;
    } else {
        // compensate divider to correct value
        tDividerInt *= 2;
    }
    uint32_t tSavedValue = tDividerInt;
    tDividerInt /= tFrequency;

    if (tDividerInt == 0) {
        // 8 Mhz / 0.125 us is Max
        hasError = true;
        tDividerInt = 1;
        tFrequency = 8;
    }

    /*
     * Determine prescaler and PrescalerRegisterValue from tDividerInt value,
     * in order to get an tDividerInt value <= 0x10000 (register value is tDividerInt-1)
     */
    uint16_t tPrescaler = 1; // direct clock
    uint8_t tPrescalerRegisterValue = 1;
    if (tDividerInt > 0x10000) {
        tDividerInt >>= 3;
        if (tDividerInt <= 0x10000) {
            tPrescaler = 8;
            tPrescalerRegisterValue = 2;
        } else {
            tDividerInt >>= 3;
            if (tDividerInt <= 0x10000) {
                tPrescaler = 64;
                tPrescalerRegisterValue = 3;
            } else {
                tDividerInt >>= 2;
                if (tDividerInt <= 0x10000) {
                    tPrescaler = 256;
                    tPrescalerRegisterValue = 4;
                } else {
                    tDividerInt >>= 2;
                    tPrescaler = 1024;
                    tPrescalerRegisterValue = 5;
                    if (tDividerInt > 0x10000) {
                        // clip to 16 bit value
                        tDividerInt = 0x10000;
                    }
                }
            }
        }
    }
    sFrequencyInfo.PrescalerRegisterValue = tPrescalerRegisterValue;
    if (sFrequencyInfo.isOutputEnabled) {
        // set values to timer register
        TCCR1B &= ~TIMER_PRESCALER_MASK;
        TCCR1B |= tPrescalerRegisterValue;
    }
    OCR1A = tDividerInt - 1; // set compare match register

    /*
     * recompute exact period and frequency for eventually changed 16 bit period
     * Frequency = ((F_CPU/2) / (DividerInt * Prescaler)) / (sFrequencyFactorTimes1000 / 1000)
     *           = (FCPU * 500) / (sFrequencyFactorTimes1000 * (DividerInt * Prescaler)
     */
    tDividerInt *= tPrescaler;

    // reuse (FCPU * 500) / sFrequencyFactorTimes1000 from above
    tFrequency = tSavedValue;
    tFrequency /= tDividerInt;
    if (tFreqWasCompensated) {
        // undo compensation from above
        tFrequency *= 2;
    }
    /*
     * Save values
     */
    sFrequencyInfo.Frequency = tFrequency;
    sFrequencyInfo.ControlValue.DividerInt = tDividerInt;
    return hasError;
}

void setFrequencyFactor(int aIndexValue) {
    sFrequencyInfo.FrequencyFactorIndex = aIndexValue;
    uint32_t tFactor = 1;
    while (aIndexValue > 0) {
        tFactor *= 1000;
        aIndexValue--;
    }
    sFrequencyInfo.FrequencyFactorTimes1000 = tFactor;
}

void stopWaveform() {
    // set prescaler choice to 0 -> timer stops
    TCCR1B &= ~TIMER_PRESCALER_MASK;
}

void startWaveform() {
    TCCR1B &= ~TIMER_PRESCALER_MASK;
    TCCR1B |= sFrequencyInfo.PrescalerRegisterValue;
}

//Timer1 overflow interrupt vector handler
ISR(TIMER1_OVF_vect) {
    static int32_t sBaseFrequencyFactorAccumulator = 0; // used to handle fractions of factor above

    // output value at start of ISR to avoid jitter
    OCR1B = sNextOcrbValue;
    /*
     * Increase index by sBaseFrequencyFactor.
     * In order to avoid floating point arithmetic use sBaseFrequencyFactorShift16 and handle resulting residual.
     */
    int8_t tIndexDelta = sFrequencyInfo.ControlValue.sBaseFrequencyFactorShift16 >> 16;
    // handle fraction of frequency Factor
    sBaseFrequencyFactorAccumulator += sFrequencyInfo.ControlValue.sBaseFrequencyFactorShift16 & 0xFFFF;
    if (sBaseFrequencyFactorAccumulator > 0x8000) {
        /*
         * Accumulated fraction is bigger than "half" so increase index
         */
        tIndexDelta++;
        sBaseFrequencyFactorAccumulator -= 0x10000;
    }
    if (tIndexDelta > 0) {
        if (sFrequencyInfo.Waveform == WAVEFORM_SINE) {
            uint8_t tQuadrantIncrease = 0;
            switch (sNumberOfQuadrant) {
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
            if (sNumberOfQuadrant & 0x02) {
                // case 2 and 3   -128 -> 128 ; -255 -> 1
                sNextOcrbValue = -(pgm_read_byte(&sSineTableQuarter128[sSineTableIndex]));
            } else {
                sNextOcrbValue = pgm_read_byte(&sSineTableQuarter128[sSineTableIndex]);
            }

            sNumberOfQuadrant = (sNumberOfQuadrant + tQuadrantIncrease) & 0x03;

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
            if (sNumberOfQuadrant == 0) {
                // Value from 1 to FF
                // increasing value
                sNextOcrbValue += tIndexDelta;
                // detect overflow (value > 0xFF)
                if (sNextOcrbValue < tOldOcrbValue) {
                    sNumberOfQuadrant = 1;
                    // 0->FE, 1->FD
                    sNextOcrbValue = (~sNextOcrbValue) - 1;
                }
            } else {
                // decreasing value Value from FE to 0
                sNextOcrbValue -= tIndexDelta;
                // detect underflow
                if (sNextOcrbValue > tOldOcrbValue) {
                    sNumberOfQuadrant = 0;
                    // FF -> 1, FE -> 2
                    sNextOcrbValue = -sNextOcrbValue;
                }
            }

        } else if (sFrequencyInfo.Waveform == WAVEFORM_SAWTOOTH) {
            sNextOcrbValue += tIndexDelta;
        }
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
