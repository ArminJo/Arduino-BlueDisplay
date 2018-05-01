/*
 * Waveforms.h
 *
 *  Copyright (C) 2017  Armin Joachimsmeyer
 *  Email: armin.joachimsmeyer@gmail.com
 *  License: GPL v3 (http://www.gnu.org/licenses/gpl.html)
 *
 */

#ifndef WAVEFORMS_H_
#define WAVEFORMS_H_

#define WAVEFORM_SQUARE 0
#define WAVEFORM_SINE 1
#define WAVEFORM_TRIANGLE 2
#define WAVEFORM_SAWTOOTH 3
#define WAVEFORM_MAX WAVEFORM_MODE_SAWTOOTH
#define WAVEFORM_MASK 0x03

struct FrequencyInfoStruct {
    union {
        uint32_t DividerInt; // Value used by hardware - may be (divider * prescaler)
        uint32_t sBaseFrequencyFactorShift16; // Value used by ISR
    } ControlValue;
    uint32_t PeriodMicros; // for CTC resolution of value of DividerInt is 8 times better
    // use float, since we have a logarithmic slider readout and therefore a lot of values between 1 and 2.
    float Frequency;    // Computed value derived from sPeriodInt
    // factor for mHz/Hz/kHz/MHz - times 1000 because of mHz handling
    uint32_t FrequencyFactorTimes1000; // 1 -> 1 mHz, 1000 -> 1 Hz, 1000000 -> 1 kHz
    uint8_t FrequencyFactorIndex; // 0->mHz, 1->Hz, 2->kHz, 3->MHz
    uint8_t Waveform;
    bool isOutputEnabled;
    uint8_t PrescalerRegisterValue; // for start/stop of square wave
};
extern struct FrequencyInfoStruct sFrequencyInfo;

extern const char FrequencyFactorChars[4];

void setWaveformMode(uint8_t aNewMode);
void cycleWaveformMode();
const char * cycleWaveformModePGMString();
const char * getWaveformModePGMString();
const char * getWaveformRangePGMString();

void initTimer1For8BitPWM();
bool setWaveformFrequency();
bool setSquareWaveFrequency();
void setFrequency(float aValue);
void setFrequencyFactor(int aIndexValue);

void stopWaveform();
void startWaveform();

// utility Function
void computeSineTableValues(uint8_t aSineTable[], unsigned int aNumber);

#endif /* WAVEFORMS_H_ */
