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

#define FREQUENCY_FACTOR_INDEX_MILLI_HERTZ 0
#define FREQUENCY_FACTOR_INDEX_HERTZ 1
#define FREQUENCY_FACTOR_INDEX_KILO_HERTZ 2
#define FREQUENCY_FACTOR_INDEX_MEGA_HERTZ 3

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
     */
    float FrequencyNormalized; // values from 1 to 1000 for display and slider - effective frequency is sFrequencyNormalized * (sFrequencyFactorTimes1000 / 1000)
    uint32_t FrequencyFactorTimes1000; // factor for mHz/Hz/kHz/MHz - times 1000 because of mHz handling - 1 -> 1 mHz, 1000 -> 1 Hz, 1000000 -> 1 kHz
    uint8_t FrequencyFactorIndex; // index for FrequencyFactorChars[]. 0->mHz, 1->Hz, 2->kHz, 3->MHz

    /*
     * Internal (private) values
     */
    int32_t BaseFrequencyFactorAccumulator; //  Value used by ISR - used to handle fractions of BaseFrequencyFactorShift16

    uint8_t PrescalerRegisterValueBackup; // backup of old value for start/stop of square wave
};
extern struct FrequencyInfoStruct sFrequencyInfo;

extern const char FrequencyFactorChars[4]; // see FrequencyFactorIndex above

void setWaveformMode(uint8_t aNewMode);
void cycleWaveformMode();
const char * cycleWaveformModePGMString();
const char * getWaveformModePGMString();
float getPeriodMicros();

void setNormalizedFrequencyAndFactor(float aValue);
void setNormalizedFrequencyFactor(int aIndexValue);

void initTimer1For8BitPWM();
bool setWaveformFrequency();
bool setWaveformFrequency(float aFrequency);
bool setSquareWaveFrequency(float aFrequency);

void stopWaveform();
void startWaveform();

// utility Function
void computeSineTableValues(uint8_t aSineTable[], unsigned int aNumber);

#endif /* WAVEFORMS_H_ */
