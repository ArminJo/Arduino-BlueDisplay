/*
 * Waveforms.h
 *
 *  Copyright (C) 2017-2023  Armin Joachimsmeyer
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

#ifndef _WAVEFORMS_H
#define _WAVEFORMS_H

#define WAVEFORM_SQUARE     0
#define WAVEFORM_SINE       1
#define WAVEFORM_TRIANGLE   2
#define WAVEFORM_SAWTOOTH   3
#define WAVEFORM_MASK       0x03

#define FREQUENCY_RANGE_INDEX_MILLI_HERTZ   0
#define FREQUENCY_RANGE_INDEX_HERTZ         1
#define FREQUENCY_RANGE_INDEX_KILO_HERTZ    2
#define FREQUENCY_RANGE_INDEX_MEGA_HERTZ    3

struct FrequencyInfoStruct {
    union {
        uint32_t DividerInt; // Only for square wave and for info - may be (divider * prescaler) - resolution is 1/8 us
        uint32_t BaseFrequencyFactorShift16; // Value used by ISR - only for NON square wave
    } ControlValue;
    uint32_t PeriodMicros; // only for display purposes
    float Frequency; // use float, since we have mHz.

    uint8_t Waveform;  // 0 to WAVEFORM_MAX
    bool isOutputEnabled;

    /*
     * Normalized frequency variables for display
     * The effective frequency is FrequencyNormalizedTo_1_to_1000 * (FrequencyNormalizedFactorTimes1000 / 1000)
     */
    float FrequencyNormalizedTo_1_to_1000; // Frequency values from 1 to 1000 for slider
    uint32_t FrequencyNormalizedFactorTimes1000; // factor for mHz/Hz/kHz/MHz - times 1000 because of mHz handling - 1 -> 1 mHz, 1000 -> 1 Hz, 1000000 -> 1 kHz
    uint8_t FrequencyRangeIndex; // index for FrequencyRangeChars[]. 0->mHz, 1->Hz, 2->kHz, 3->MHz

    /*
     * Internal (private) values
     */
    int32_t BaseFrequencyFactorAccumulator; //  Value used by ISR - used to handle fractions of BaseFrequencyFactorShift16

    uint8_t PrescalerRegisterValueBackup; // backup of old value for start/stop of square wave
};
extern struct FrequencyInfoStruct sFrequencyInfo;

extern const char FrequencyRangeChars[4]; // see FrequencyRangeIndex above

void setWaveformMode(uint8_t aNewMode);
void cycleWaveformMode();
const __FlashStringHelper* cycleWaveformModePGMString();
const __FlashStringHelper* getWaveformModePGMString();
float getPeriodMicros();

void setNormalizedFrequencyAndFactor(float aFrequency);
void setNormalizedFrequencyFactorFromRangeIndex(uint8_t aFrequencyRangeIndex);

void initTimer1For8BitPWM();
bool setWaveformFrequencyFromNormalizedValues();
bool setWaveformFrequency(float aFrequency);
bool setSquareWaveFrequency(float aFrequency);

void stopWaveform();
void startWaveform();

// utility Function
void computeSineTableValues(uint8_t aSineTable[], unsigned int aNumber);

#endif // _WAVEFORMS_H
