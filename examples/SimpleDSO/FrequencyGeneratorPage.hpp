/*
 * FrequencyGeneratorPage.hpp
 *
 * Frequency output from 119 mHz (8.388 second) to 8 MHz square wave on Arduino using timer1.
 * Sine waveform output from 7,421 mHz to 7812.5 Hz
 * Triangle from 3.725 mHz to 1953.125 Hz
 * Sawtooth from 1.866 mHz to 3906.25 Hz
 *
 * !!!Do not run DSO acquisition and non square wave waveform generation at the same time!!!
 * Because of the interrupts at 62 kHz rate, DSO is almost not usable during non square wave waveform generation.
 * Waveform frequency is not stable and decreased, since not all TIMER1 OVERFLOW interrupts are handled.
 *
 * PWM RC-Filter suggestions
 * Simple: 2.2 kOhm and 100 nF
 * 2nd order (good for sine and triangle): 1 kOhm and 100 nF -> 4.7 kOhm and 22 nF
 * 2nd order (better for sawtooth):        1 kOhm and 22 nF  -> 4.7 kOhm and 4.7 nF
 *
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
 *
 */
#ifndef _FREQUENCY_GENERATOR_PAGE_HPP
#define _FREQUENCY_GENERATOR_PAGE_HPP

#if defined(AVR)
#include "FrequencyGeneratorPage.h"
#endif

#include "Waveforms.h"

static void (*sLastRedrawCallback)(void);

#define COLOR_BACKGROUND_FREQ COLOR16_WHITE

#if defined(AVR)
#define TIMER_PRESCALER_64 0x03
#define TIMER_PRESCALER_MASK 0x07
#endif

#define NUMBER_OF_FIXED_FREQUENCY_BUTTONS 10
#define NUMBER_OF_FREQUENCY_RANGE_BUTTONS 5

/*
 * Position + size
 */
#define FREQ_SLIDER_SIZE 10 // width of bar / border
#define FREQ_SLIDER_MAX_VALUE 300 // (BlueDisplay1.getDisplayWidth() - 20) = 300 length of bar
#define FREQ_SLIDER_X 5
#define FREQ_SLIDER_Y (4 * TEXT_SIZE_11_HEIGHT + 4)

/*
 * Direct frequency + range buttons
 */
#if defined(AVR)
const uint16_t FixedFrequencyButtonCaptions[NUMBER_OF_FIXED_FREQUENCY_BUTTONS] PROGMEM
= { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000 };

// the compiler cannot optimize 2 occurrences of the same PROGMEM string :-(
const char StringmHz[] PROGMEM = "mHz";
const char StringHz[] PROGMEM = "Hz";
const char String10Hz[] PROGMEM = "10Hz";
const char StringkHz[] PROGMEM = "kHz";
const char StringMHz[] PROGMEM = "MHz";

const char *RangeButtonStrings[5] = { StringmHz, StringHz, String10Hz, StringkHz, StringMHz };
#else
const uint16_t FixedFrequencyButtonCaptions[NUMBER_OF_FIXED_FREQUENCY_BUTTONS] = { 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000 };
const char *const RangeButtonStrings[5] = { "mHz", "Hz", "10Hz", "kHz", "MHz" };
const char FrequencyRangeChars[4] = { 'm', ' ', 'k', 'M' };
struct FrequencyInfoStruct sFrequencyInfo;

#endif
#define INDEX_OF_10HZ 2
static bool is10HzRange = true;

static const int BUTTON_INDEX_SELECTED_INITIAL = 2; // select 10Hz Button

/*
 * GUI
 */
BDButton TouchButtonFrequencyRanges[NUMBER_OF_FREQUENCY_RANGE_BUTTONS];
BDButton ActiveTouchButtonFrequencyRange; // Used to determine which range button is active

BDButton TouchButtonFrequencyStartStop;
BDButton TouchButtonGetFrequency;
BDButton TouchButtonWaveform;

#if defined(SUPPORT_LOCAL_DISPLAY)
BDButton TouchButton1;
BDButton TouchButton2;
BDButton TouchButton5;
BDButton TouchButton10;
BDButton TouchButton20;
BDButton TouchButton50;
BDButton TouchButton100;
BDButton TouchButton200;
BDButton TouchButton500;
BDButton TouchButton1k;
BDButton *const TouchButtonFixedFrequency[] = { &TouchButton1, &TouchButton2, &TouchButton5, &TouchButton10, &TouchButton20,
        &TouchButton50, &TouchButton100, &TouchButton200, &TouchButton500, &TouchButton1k };
#else
BDButton TouchButtonFirstFixedFrequency;
#endif

BDSlider TouchSliderFrequency;

void initFrequencyGeneratorPageGui(void);

void doSetFrequencyFromSliderValue(BDSlider *aTheTouchedSlider, uint16_t aFrequencySliderValue);

void doWaveformMode(BDButton *aTheTouchedButton, int16_t aValue);
void doSetFixedFrequency(BDButton *aTheTouchedButton, int16_t aNormalizedFrequency);
void doSetFrequencyRange(BDButton *aTheTouchedButton, int16_t aInputRangeIndex);
void doFrequencyGeneratorStartStop(BDButton *aTheTouchedButton, int16_t aValue);
void doGetFrequency(BDButton *aTheTouchedButton, int16_t aValue);

bool setWaveformFrequencyAndPrintValues();

void printFrequencyAndPeriod();
#if defined(AVR)
void setWaveformButtonCaption(void);
void initTimer1ForCTC(void);
#endif

/***********************
 * Code starts here
 ***********************/

void initFrequencyGenerator(void) {
#if defined(AVR)
    initTimer1ForCTC();
#else
    Synth_Timer_initialize(4711);
#endif
}

void initFrequencyGeneratorPage(void) {
    initFrequencyGenerator();
    /*
     * Initialize frequency and other fields to 200 Hz
     */
    sFrequencyInfo.isOutputEnabled = false;
    sFrequencyInfo.Waveform = WAVEFORM_SQUARE;
    setWaveformFrequency(200);

    sFrequencyInfo.isOutputEnabled = true; // to start output at first display of page

#if !defined(SUPPORT_LOCAL_DISPLAY)
    initFrequencyGeneratorPageGui();
#endif
}

void startFrequencyGeneratorPage(void) {
    BlueDisplay1.clearDisplay();

#if defined(SUPPORT_LOCAL_DISPLAY)
    // do it here to enable freeing of button resources in stopFrequencyGeneratorPage()
    initFrequencyGeneratorPageGui();
#endif

    setWaveformFrequencyFromNormalizedValues();

    drawFrequencyGeneratorPage();
    /*
     * save state
     */
    sLastRedrawCallback = getRedrawCallback();
    registerRedrawCallback(&drawFrequencyGeneratorPage);

#if !defined(AVR)
    Synth_Timer_Start();
#endif
}

void loopFrequencyGeneratorPage(void) {
    checkAndHandleEvents();
}

void stopFrequencyGeneratorPage(void) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    // free buttons
    for (unsigned int i = 0; i < NUMBER_OF_FIXED_FREQUENCY_BUTTONS; ++i) {
        TouchButtonFixedFrequency[i]->deinit();
    }

    for (int i = 0; i < NUMBER_OF_FREQUENCY_RANGE_BUTTONS; ++i) {
        TouchButtonFrequencyRanges[i].deinit();
    }
    TouchButtonFrequencyStartStop.deinit();
    TouchButtonGetFrequency.deinit();
    TouchSliderFrequency.deinit();
#  if defined(AVR)
    TouchButtonWaveform.deinit();
#  endif
#endif
    /*
     * restore previous state
     */
    registerRedrawCallback(sLastRedrawCallback);
}

void initFrequencyGeneratorPageGui() {
    // Frequency slider for 1 to 1000 at top of screen
    TouchSliderFrequency.init(FREQ_SLIDER_X, FREQ_SLIDER_Y, FREQ_SLIDER_SIZE, FREQ_SLIDER_MAX_VALUE,
    FREQ_SLIDER_MAX_VALUE, 0, COLOR16_BLUE, COLOR16_GREEN, FLAG_SLIDER_SHOW_BORDER | FLAG_SLIDER_IS_HORIZONTAL,
            &doSetFrequencyFromSliderValue);

    /*
     * Fixed frequency buttons next.
     * Example of button handling without button objects.
     * We rely on button handles / ID's being simple integers and increasing by one for each init.
     * We use a start button for initialization, which changes position, value and caption.
     * We use the start button ID as start id for drawing all buttons.
     */
    uint16_t tXPos = 0;
    uint16_t tFrequency;
#if defined(AVR)
    // captions are in PGMSPACE
    const uint16_t *tFrequencyCaptionPtr = &FixedFrequencyButtonCaptions[0];
    for (uint8_t i = 0; i < NUMBER_OF_FIXED_FREQUENCY_BUTTONS; ++i) {
        tFrequency = pgm_read_word(tFrequencyCaptionPtr);
        sprintf_P(sStringBuffer, PSTR("%u"), tFrequency);
#else
    for (uint8_t i = 0; i < NUMBER_OF_FIXED_FREQUENCY_BUTTONS; ++i) {
        tFrequency = FixedFrequencyButtonCaptions[i];
        sprintf(sStringBuffer, "%u", tFrequency);
#endif

#if defined(SUPPORT_LOCAL_DISPLAY)
        TouchButtonFixedFrequency[i]->init(tXPos, 96, BUTTON_WIDTH_10, BUTTON_HEIGHT_6, COLOR16_BLUE, sStringBuffer, TEXT_SIZE_11,
                0, tFrequency, &doSetFixedFrequency);
#else
        TouchButtonFirstFixedFrequency.init(tXPos, 98, BUTTON_WIDTH_10, BUTTON_HEIGHT_6, COLOR16_BLUE, sStringBuffer, TEXT_SIZE_11,
                0, tFrequency, &doSetFixedFrequency);
#endif

        tXPos += BUTTON_WIDTH_10 + BUTTON_DEFAULT_SPACING_QUARTER;
#if defined(AVR)
        tFrequencyCaptionPtr++;
#endif
    }
#if !defined(SUPPORT_LOCAL_DISPLAY)
    TouchButtonFirstFixedFrequency.mButtonHandle -= NUMBER_OF_FIXED_FREQUENCY_BUTTONS - 1;
#endif

    // Range next
    tXPos = 0;
    int tYPos = DISPLAY_HEIGHT - BUTTON_HEIGHT_4 - BUTTON_HEIGHT_5 - BUTTON_DEFAULT_SPACING;
    for (int i = 0; i < NUMBER_OF_FREQUENCY_RANGE_BUTTONS; ++i) {
        uint16_t tButtonColor = BUTTON_AUTO_RED_GREEN_FALSE_COLOR;
        if (i == BUTTON_INDEX_SELECTED_INITIAL) {
            tButtonColor = BUTTON_AUTO_RED_GREEN_TRUE_COLOR;
        }
        TouchButtonFrequencyRanges[i].init(tXPos, tYPos, BUTTON_WIDTH_5 + BUTTON_DEFAULT_SPACING_HALF,
        BUTTON_HEIGHT_5, tButtonColor, reinterpret_cast<const __FlashStringHelper*>(RangeButtonStrings[i]), TEXT_SIZE_22,
                FLAG_BUTTON_DO_BEEP_ON_TOUCH, i, &doSetFrequencyRange);

        tXPos += BUTTON_WIDTH_5 + BUTTON_DEFAULT_SPACING - 2;
    }

    ActiveTouchButtonFrequencyRange = TouchButtonFrequencyRanges[BUTTON_INDEX_SELECTED_INITIAL];

    TouchButtonFrequencyStartStop.init(0, DISPLAY_HEIGHT - BUTTON_HEIGHT_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, 0, F("Start"),
            TEXT_SIZE_26, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, sFrequencyInfo.isOutputEnabled,
            &doFrequencyGeneratorStartStop);
    TouchButtonFrequencyStartStop.setCaptionForValueTrue(F("Stop"));

    TouchButtonGetFrequency.init(BUTTON_WIDTH_3_POS_2, DISPLAY_HEIGHT - BUTTON_HEIGHT_4, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR16_BLUE, F("Hz..."), TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doGetFrequency);

#if defined(AVR)
    TouchButtonWaveform.init(BUTTON_WIDTH_3_POS_3, DISPLAY_HEIGHT - BUTTON_HEIGHT_4, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR16_BLUE, "", TEXT_SIZE_18, FLAG_BUTTON_DO_BEEP_ON_TOUCH, sFrequencyInfo.Waveform, &doWaveformMode);
    setWaveformButtonCaption();
#endif
}

void drawFrequencyGeneratorPage(void) {
    // do not clear screen here since it is called periodically for GUI refresh while DSO is running
    BDButton::deactivateAll();
    BDSlider::deactivateAll();
#if !defined(ARDUINO)
    TouchButtonMainHome.drawButton();
#else
    TouchButtonBack.drawButton();
#endif
    TouchSliderFrequency.drawSlider();

    BlueDisplay1.drawText(TEXT_SIZE_11_WIDTH, FREQ_SLIDER_Y + 3 * FREQ_SLIDER_SIZE + TEXT_SIZE_11_HEIGHT, F("1"), TEXT_SIZE_11,
    COLOR16_BLUE, COLOR_BACKGROUND_FREQ);
#if defined(AVR)
    BlueDisplay1.drawText(DISPLAY_WIDTH - 5 * TEXT_SIZE_11_WIDTH,
    FREQ_SLIDER_Y + 3 * FREQ_SLIDER_SIZE + TEXT_SIZE_11_HEIGHT, F("1000"), TEXT_SIZE_11, COLOR16_BLUE,
    COLOR_BACKGROUND_FREQ);
#else
    BlueDisplay1.drawText(BlueDisplay1.getDisplayWidth() - 5 * TEXT_SIZE_11_WIDTH,
    FREQ_SLIDER_Y + 3 * FREQ_SLIDER_SIZE + TEXT_SIZE_11_HEIGHT, ("1000"), TEXT_SIZE_11, COLOR16_BLUE,
    COLOR_BACKGROUND_FREQ);
#endif

    // fixed frequency buttons
    // we know that the buttons handles are increasing numbers
#if defined(SUPPORT_LOCAL_DISPLAY)
    for (uint8_t i = 0; i < NUMBER_OF_FIXED_FREQUENCY_BUTTONS - 1; ++i) {
        // Generate strings each time buttons are drawn since only the pointer to caption is stored in button
        sprintf(sStringBuffer, "%u", FixedFrequencyButtonCaptions[i]);
        TouchButtonFixedFrequency[i]->setCaption(sStringBuffer);
        TouchButtonFixedFrequency[i]->drawButton();
    }
    // label last button 1k instead of 1000 which is too long
    TouchButtonFixedFrequency[NUMBER_OF_FIXED_FREQUENCY_BUTTONS - 1]->setCaption("1k");
    TouchButtonFixedFrequency[NUMBER_OF_FIXED_FREQUENCY_BUTTONS - 1]->drawButton();
#else
    BDButton tButton(TouchButtonFirstFixedFrequency);
    for (uint8_t i = 0; i < NUMBER_OF_FIXED_FREQUENCY_BUTTONS; ++i) {
        tButton.drawButton();
        tButton.mButtonHandle++; // Simply increment id to get the next button
    }
#endif

    for (uint8_t i = 0; i < NUMBER_OF_FREQUENCY_RANGE_BUTTONS; ++i) {
        TouchButtonFrequencyRanges[i].drawButton();
    }

    TouchButtonFrequencyStartStop.drawButton();
    TouchButtonGetFrequency.drawButton();
#if defined(AVR)
    TouchButtonWaveform.drawButton();
#endif

    // show values
    printFrequencyAndPeriod();
}

/*
 * Handle the 10HzRange of frequency GUI
 */
void setFrequencyNormalizedForGUI(float aNormalizedFrequency) {
    if (is10HzRange) {
        // we must dynamically change frequency range
        if (aNormalizedFrequency <= 100) {
            setNormalizedFrequencyFactorFromRangeIndex(FREQUENCY_RANGE_INDEX_HERTZ);
            aNormalizedFrequency *= 10;
        } else {
            setNormalizedFrequencyFactorFromRangeIndex(FREQUENCY_RANGE_INDEX_KILO_HERTZ);
            aNormalizedFrequency /= 100;
        }
    }
    sFrequencyInfo.FrequencyNormalizedTo_1_to_1000 = aNormalizedFrequency;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"

/**
 * Convert linear slider value to exponential normalized frequency from 1 to 1000
 */
void doSetFrequencyFromSliderValue(BDSlider *aTheTouchedSlider, uint16_t aFrequencySliderValue) {
    float tNormalizedFrequencyFloat = aFrequencySliderValue;
    tNormalizedFrequencyFloat = tNormalizedFrequencyFloat / (FREQ_SLIDER_MAX_VALUE / 3); // gives 0-3
    // 950 byte program memory required for pow() and log10f()
    tNormalizedFrequencyFloat = (pow(10, tNormalizedFrequencyFloat)); // normalize value to 1-1000
    setFrequencyNormalizedForGUI(tNormalizedFrequencyFloat);
    setWaveformFrequencyAndPrintValues();
}

/**
 * Set frequency to fixed value 1,2,5,10...,1000
 */
void doSetFixedFrequency(BDButton *aTheTouchedButton, int16_t aNormalizedFrequency) {
    setFrequencyNormalizedForGUI(aNormalizedFrequency);
    // Play error feedback tone, if frequency is not available for this waveform
    bool tErrorOrClippingHappend = setWaveformFrequencyAndPrintValues();
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalTouchButton::playFeedbackTone(tErrorOrClippingHappend);
#else
    BDButton::playFeedbackTone(tErrorOrClippingHappend);
#endif
}

/**
 * Sets the frequency range (mHz - MHz)
 * Set color for old and new button
 */
void doSetFrequencyRange(BDButton *aTheTouchedButton, int16_t aInputRangeIndex) {

    if (ActiveTouchButtonFrequencyRange != *aTheTouchedButton) {
        // Handling of 10 Hz button
        // convert input range index to output range index
        if (aInputRangeIndex == INDEX_OF_10HZ) {
            is10HzRange = true;
        } else {
            is10HzRange = false;
        }
        uint8_t tOutputRangeIndex = aInputRangeIndex;
        if (aInputRangeIndex >= INDEX_OF_10HZ) {
            tOutputRangeIndex = aInputRangeIndex - 1;
        }

        // No MHz for PWM waveforms
        if (aInputRangeIndex != FREQUENCY_RANGE_INDEX_MEGA_HERTZ || sFrequencyInfo.Waveform == WAVEFORM_SQUARE) {

            // Set color for old and new button
            ActiveTouchButtonFrequencyRange.setButtonColorAndDraw( BUTTON_AUTO_RED_GREEN_FALSE_COLOR);
            ActiveTouchButtonFrequencyRange = *aTheTouchedButton;
            aTheTouchedButton->setButtonColorAndDraw( BUTTON_AUTO_RED_GREEN_TRUE_COLOR);

            setNormalizedFrequencyFactorFromRangeIndex(tOutputRangeIndex);
            setWaveformFrequencyAndPrintValues();
        }
    }
}

#if defined(AVR)
void setWaveformButtonCaption(void) {
    TouchButtonWaveform.setCaption(getWaveformModePGMString(), (DisplayControl.DisplayPage == DSO_PAGE_FREQUENCY));
}
#endif

void doWaveformMode(BDButton *aTheTouchedButton, int16_t aValue) {
#if defined(AVR)
    cycleWaveformMode();
    setWaveformButtonCaption();
#endif
}

#if defined(SUPPORT_LOCAL_DISPLAY)
/**
 * gets frequency value from numberpad
 * @param aTheTouchedButton
 * @param aValue
 */
void doGetFrequency(BDButton *aTheTouchedButton, int16_t aValue) {
    TouchSliderFrequency.deactivate();
    float tNumber = getNumberFromNumberPad(NUMBERPAD_DEFAULT_X, 0, COLOR16_BLUE);
// check for cancel
    if (!isnan(tNumber)) {
        sFrequencyInfo.Frequency = tNumber;
    }
    drawFrequencyGeneratorPage();
    setWaveformFrequencyAndPrintValues();
}
#else

/**
 * Handler for number receive event - set frequency to float value
 */
void doSetFrequency(float aValue) {
    setWaveformFrequency(aValue);
    printFrequencyAndPeriod();
}

/**
 * Request frequency numerical
 */
void doGetFrequency(BDButton *aTheTouchedButton, int16_t aValue) {
    BlueDisplay1.getNumberWithShortPrompt(&doSetFrequency, F("frequency [Hz]"));
}
#endif

void doFrequencyGeneratorStartStop(BDButton *aTheTouchedButton, int16_t aValue) {
    sFrequencyInfo.isOutputEnabled = aValue;
    if (aValue) {
        // Start timer
#if !defined(AVR)
        Synth_Timer_Start();
#endif
        setWaveformFrequencyAndPrintValues();
    } else {
        // Stop timer
#if defined(AVR)
        stopWaveform();
#else
        Synth_Timer_Stop();
#endif
    }
}

/*
 * Prints frequency and period and sets slider accordingly
 */
void printFrequencyAndPeriod() {

    float tPeriodMicros;

#if defined(AVR)
    dtostrf(sFrequencyInfo.FrequencyNormalizedTo_1_to_1000, 9, 3, &sStringBuffer[20]);
    sprintf_P(sStringBuffer, PSTR("%s%cHz"), &sStringBuffer[20], FrequencyRangeChars[sFrequencyInfo.FrequencyRangeIndex]);

#else
    snprintf(sStringBuffer, sizeof sStringBuffer, "%9.3f%cHz", sFrequencyInfo.FrequencyNormalizedTo_1_to_1000,
            FrequencyRangeChars[sFrequencyInfo.FrequencyRangeIndex]);
#endif

// print frequency
    BlueDisplay1.drawText(FREQ_SLIDER_X + 2 * TEXT_SIZE_22_WIDTH, TEXT_SIZE_22_HEIGHT, sStringBuffer, TEXT_SIZE_22,
    COLOR16_RED, COLOR_BACKGROUND_FREQ);

// output period use float, since we have 1/8 us for square wave
    tPeriodMicros = getPeriodMicros();

    char tUnitChar = '\xB5';    // micro
    if (tPeriodMicros > 10000) {
        tPeriodMicros /= 1000;
        tUnitChar = 'm';
    }

#if defined(AVR)
    dtostrf(tPeriodMicros, 10, 3, &sStringBuffer[20]);
    sprintf_P(sStringBuffer, PSTR("%s%cs"), &sStringBuffer[20], tUnitChar);
#else
    snprintf(sStringBuffer, sizeof sStringBuffer, "%10.3f%cs", tPeriodMicros, tUnitChar);
#endif
    BlueDisplay1.drawText(FREQ_SLIDER_X, TEXT_SIZE_22_HEIGHT + 4 + TEXT_SIZE_22_ASCEND, sStringBuffer, TEXT_SIZE_22,
    COLOR16_BLUE, COLOR_BACKGROUND_FREQ);

// 950 byte program memory required for pow() and log10f()
    uint16_t tSliderValue;
    tSliderValue = log10f(sFrequencyInfo.FrequencyNormalizedTo_1_to_1000) * (FREQ_SLIDER_MAX_VALUE / 3);
    if (is10HzRange) {
        if (sFrequencyInfo.FrequencyRangeIndex == FREQUENCY_RANGE_INDEX_KILO_HERTZ) {
            tSliderValue += 2 * (FREQ_SLIDER_MAX_VALUE / 3);
        } else {
            tSliderValue -= (FREQ_SLIDER_MAX_VALUE / 3);
        }
    }
    TouchSliderFrequency.setValueAndDrawBar(tSliderValue);
}

/**
 * Computes Autoreload value for synthesizer from 8,381 mHz (0xFFFFFFFF) to 18 MHz (0x02) and prints frequency value
 * @param aSetSlider
 * @param global variable Frequency
 * @return true if error / clipping happened
 */
bool setWaveformFrequencyAndPrintValues() {
    bool tErrorOrClippingHappend = setWaveformFrequencyFromNormalizedValues();
    printFrequencyAndPeriod();
    return tErrorOrClippingHappend;
}

#if !defined(AVR)
// content for AVR is in Waveforms.cpp

#define WAVEFORM_SQUARE     0
#define WAVEFORM_SINE       1
#define WAVEFORM_TRIANGLE   2
#define WAVEFORM_SAWTOOTH   3

void setNormalizedFrequencyFactorFromRangeIndex(uint8_t aFrequencyRangeIndex) {
    sFrequencyInfo.FrequencyRangeIndex = aFrequencyRangeIndex;
    uint32_t tNormalizedFactorTimes1000 = 1;
    while (aFrequencyRangeIndex > 0) {
        tNormalizedFactorTimes1000 *= 1000;
        aFrequencyRangeIndex--;
    }
    sFrequencyInfo.FrequencyNormalizedFactorTimes1000 = tNormalizedFactorTimes1000;
}

/*
 * Set display values sFrequencyNormalized and sFrequencyFactorIndex
 *
 * Problem is set e.g. value of 1 Hz as "1000mHz" or "1Hz"?
 * so we just try to keep the existing range.
 * First put value of 1000 to next range,
 * then undo if value < 1.00001 and existing range is one lower
 */
void setNormalizedFrequencyAndFactor(float aFrequency) {
    uint8_t tFrequencyRangeIndex = 1;
    // normalize Frequency to 1 - 1000 and compute FrequencyRangeIndex
    if (aFrequency < 1) {
        tFrequencyRangeIndex = 0; //mHz
        aFrequency *= 1000;
    } else {
        // 1000.1 to avoid switching to next range because of resolution issues
        while (aFrequency >= 1000) {
            aFrequency /= 1000;
            tFrequencyRangeIndex++;
        }
    }

    // check if tFrequencyFactorIndex - 1 fits better
    if (aFrequency < 1.00001 && sFrequencyInfo.FrequencyRangeIndex == (tFrequencyRangeIndex - 1)) {
        aFrequency *= 1000;
        tFrequencyRangeIndex--;
    }

    setNormalizedFrequencyFactorFromRangeIndex(tFrequencyRangeIndex);
    sFrequencyInfo.FrequencyNormalizedTo_1_to_1000 = aFrequency;
}

bool setWaveformFrequency(float aValue) {
    bool hasError = false;
    if (sFrequencyInfo.Waveform == WAVEFORM_SQUARE) {
        float tDivider = 36000000 / aValue;
        uint32_t tDividerInt = tDivider;
        if (tDividerInt < 2) {
            hasError = true;
            tDividerInt = 2;
        }

#  if defined(STM32F30X)
        Synth_Timer32_SetReloadValue(tDividerInt);
#  else
        uint32_t tPrescalerValue = (tDividerInt >> 16) + 1; // +1 since at least divider by 1
        if (tPrescalerValue > 1) {
            //we have prescaler > 1 -> adjust reload value to be less than 0x10001
            tDividerInt /= tPrescalerValue;
        }
        Synth_Timer16_SetReloadValue(tDividerInt, tPrescalerValue);
        tDividerInt *= tPrescalerValue;
#  endif
        sFrequencyInfo.ControlValue.DividerInt = tDividerInt;
        // recompute frequency
        sFrequencyInfo.Frequency = 36000000 / tDividerInt;
        setNormalizedFrequencyAndFactor(sFrequencyInfo.Frequency);
    } else {
        hasError = true;
    }

    return hasError;
}

bool setWaveformFrequencyFromNormalizedValues() {
    return setWaveformFrequency((sFrequencyInfo.FrequencyNormalizedTo_1_to_1000 * sFrequencyInfo.FrequencyNormalizedFactorTimes1000) / 1000);
}

float getPeriodMicros() {
    return sFrequencyInfo.ControlValue.DividerInt / 36.0f;
}
#endif // !defined(AVR)

#endif // _FREQUENCY_GENERATOR_PAGE_HPP
