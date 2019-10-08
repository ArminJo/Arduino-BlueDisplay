/*
 * TouchDSOCommon.h
 *
 * Declarations for AVR and ARM common section
 *
 *  Copyright (C) 2017  Armin Joachimsmeyer
 *  Email: armin.joachimsmeyer@gmail.com
 *  License: GPL v3 (http://www.gnu.org/licenses/gpl.html)
 */

#ifndef TOUCHDSOCOMMON_H_
#define TOUCHDSOCOMMON_H_

#include "BDButton.h"
#include "BDSlider.h"
#ifdef AVR
#include "FrequencyGeneratorPage.h"
#endif

// Internal version
#define VERSION_DSO "3.1"
/*
 * Version 3.1
 * - stop response improved for fast mode.
 * - value computation for ultra fast modes fixed.
 * - millis() timer compensation formula fixed.
 * - AC/DC button and info line handling improved.
 */

#ifdef AVR
#else
// No PROGMEM on ARM
#define PROGMEM
#define __FlashStringHelper char
#define setCaptionPGM setCaption
#define drawTextPGM drawText
#define F(a) a
#define PSTR(a) a
#endif

#ifdef AVR
// Data buffer size (must be small enough to leave appr. 7 % (144 Byte) for stack
#define DATABUFFER_SIZE (3*REMOTE_DISPLAY_WIDTH) //960
#else
#ifdef STM32F303xC
#define DATABUFFER_SIZE_FACTOR 10
#else
#define DATABUFFER_SIZE_FACTOR 7
#endif
#endif

#define DISPLAY_VALUE_FOR_ZERO (REMOTE_DISPLAY_HEIGHT - 1)
//#define DISPLAY_VALUE_FOR_ZERO (REMOTE_DISPLAY_HEIGHT - 2) // Zero line is not exactly at bottom of display to improve readability

//Line colors
#define COLOR_VOLTAGE_PICKER COLOR_YELLOW
#define COLOR_VOLTAGE_PICKER_SLIDER RGB(0xFF,0XFF,0xE0) // Light Yellow
#define COLOR_TRIGGER_LINE COLOR_PURPLE
#define COLOR_TRIGGER_SLIDER RGB(0xFF,0XF0,0xFF) // light Magenta

#define COLOR_MAX_MIN_LINE COLOR_GREEN
#define COLOR_HOR_GRID_LINE_LABEL COLOR_BLUE
#define COLOR_HOR_GRID_LINE_LABEL_NEGATIVE COLOR_RED

/*
 * CHANNEL
 */
#define MAX_ADC_EXTERNAL_CHANNEL 4 // 5 channels 0-4, since ADC5/PC5 is used for AC/DC switching
#ifdef AVR
#define ADC_CHANNEL_COUNT ((MAX_ADC_EXTERNAL_CHANNEL + 1) + 2) // The number of external and internal ADC channels
#else
#define START_ADC_CHANNEL_INDEX 0  // see also ChannelSelectButtonString
#ifdef STM32F303xC
#define ADC_CHANNEL_COUNT 6 // The number of ADC channels
#else
#define ADC_CHANNEL_COUNT 6 // The number of ADC channels
#endif
extern uint8_t const ADCInputMUXChannels[ADC_CHANNEL_COUNT];
#endif
#define NUMBER_OF_CHANNELS_WITH_FIXED_ATTENUATOR 3 // Channel0 = /1, Ch1= /10, Ch2= /100

extern const char * const ADCInputMUXChannelStrings[ADC_CHANNEL_COUNT];
extern const char * const ChannelDivByButtonStrings[NUMBER_OF_CHANNELS_WITH_FIXED_ATTENUATOR];

/*
 * Trigger values
 */
#define TRIGGER_MODE_AUTOMATIC 0
#define TRIGGER_MODE_MANUAL_TIMEOUT 1
#define TRIGGER_MODE_MANUAL 2 // without timeout
#define TRIGGER_MODE_FREE 3 // waits at least 23 ms (255 samples) for trigger
#define TRIGGER_MODE_EXTERN 4
#define TRIGGER_HYSTERESIS_FOR_MODE_MANUAL 4

#ifdef AVR
/*****************************
 * Timebase stuff
 *****************************/
#define TIMEBASE_INDEX_START_VALUE 7 // 2 ms - shows 50 Hz

// ADC HW prescaler values
#define ADC_PRESCALE4    2 // is noisy
#define ADC_PRESCALE8    3 // is reasonable
#define ADC_PRESCALE16   4
#define ADC_PRESCALE32   5
#define ADC_PRESCALE64   6
#define ADC_PRESCALE128  7

#define ADC_PRESCALE_MAX_VALUE ADC_PRESCALE128
#define ADC_PRESCALE_START_VALUE ADC_PRESCALE128
#define ADC_PRESCALE_FOR_TRIGGER_SEARCH ADC_PRESCALE8

#define TIMER0_PRESCALE0    1
#define TIMER0_PRESCALE8    2
#define TIMER0_PRESCALE64   3
#define TIMER0_PRESCALE256  4
#define TIMER0_PRESCALE1024 5

/*
 * Since prescaler PRESCALE4 gives bad quality, use PRESCALE8 for 201 us range and display each value twice.
 * PRESCALE8 has pretty good quality, but PRESCALE16 (496 us/div) performs slightly better.
 *
 * Different Acquisition modes depending on Timebase:
 * Mode ultrafast  10-50us - ADC free running with PRESCALE4 - one loop for read and store 10 bit => needs double buffer space - interrupts blocked for duration of loop
 * Mode fast       101-201us - ADC free running with PRESCALE8 - one loop for read but pre process 10 -> 8 Bit and store - interrupts blocked for duration of loop
 * mode ISR        >= 496us  - ADC generates Interrupts. Waits free running with PRESCALE16 for trigger then switch to timer0 based timebase
 */

#define HORIZONTAL_GRID_COUNT 6
/**
 * Formula for Grid Height is:
 * 5 Volt Reference, 10 bit Resolution => 1023/5 = 204.6 Pixel per Volt
 * 1 Volt per Grid -> 204,6 pixel. With scale (shift) 2 => 51.15 pixel.
 * 0.5 Volt -> 102.3 pixel with scale (shift) 1 => 51.15 pixel
 * 0.2 Volt -> 40.96 pixel
 * 1.1 Volt Reference 1023/1.1 = 930 Pixel per Volt
 * 0.2 Volt -> 186 pixel with scale (shift) 2 => 46.5 pixel
 * 0.1 Volt -> 93 pixel with scale (shift) 1 => 46.5 pixel
 * 0.05 Volt -> 46.5 pixel
 */

#define HORIZONTAL_GRID_HEIGHT_1_1V_SHIFT8 11904 // 46.5*256 for 0.05 to 0.2 Volt/div for 6 divs per screen
#define HORIZONTAL_GRID_HEIGHT_2V_SHIFT8 6554 // 25.6*256 for 0.05 to 0.2 Volt/div for 10 divs per screen
#define ADC_CYCLES_PER_CONVERSION 13
#define TIMING_GRID_WIDTH 31 // with 31 instead of 32 the values fit better to 1-2-5 timebase scale
#define TIMEBASE_NUMBER_OF_ENTRIES 15 // the number of different timebases provided
#define TIMEBASE_NUMBER_OF_FAST_PRESCALE 8 // the number of prescale values not equal slowest possible prescale (PRESCALE128)
#define TIMEBASE_NUMBER_OF_FAST_MODES 5 // first 5 timebase (10 - 201) are fast free running modes with polling instead of ISR using PRESCALE4 + PRESCALE8
#define TIMEBASE_INDEX_ULTRAFAST_MODES 2 // first 3 timebase (10 - 50) using PRESCALE4 is ultra fast polling without preprocessing and therefore needs double buffer size
#define TIMEBASE_NUMBER_OF_XSCALE_CORRECTION 4  // number of timebase which are simulated by display XSale factor. Since PRESCALE4 gives bad quality, use PRESCALE8 and XScale for 201 us range
#define TIMEBASE_INDEX_MILLIS 6 // min index to switch to ms instead of us display
#define TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE 11 // min index where chart is drawn while buffer is filled (11 => 50 ms)
#else
/*
 * TIMEBASE
 */
#define TIMEBASE_INDEX_START_VALUE 12

#define CHANGE_REQUESTED_TIMEBASE_FLAG 0x01

#define TIMEBASE_NUMBER_OF_ENTRIES 21 // the number of different timebase provided - 1. entry is not uses until interleaved acquisition is implemented
#define TIMEBASE_NUMBER_OF_EXCACT_ENTRIES 8 // the number of exact float value for timebase because of granularity of clock division
#define TIMEBASE_FAST_MODES 7 // first modes are fast DMA modes
#define TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE 17 // min index where chart is drawn while buffer is filled
#define TIMEBASE_INDEX_CAN_USE_OVERSAMPLING 11 // min index where Min/Max oversampling is enabled
#ifdef STM32F303xC
#define TIMEBASE_NUMBER_START 1  // first reasonable Timebase to display - 0 if interleaving is realized
#define TIMEBASE_NUMBER_OF_XSCALE_CORRECTION 5  // number of timebase which are simulated by display XSale factor
#else
#define TIMEBASE_NUMBER_START 3  // first reasonable Timebase to display - we have only 0.8 MSamples
#define TIMEBASE_NUMBER_OF_XSCALE_CORRECTION 7  // number of timebase which are simulated by display XSale factor
#endif
#define TIMEBASE_INDEX_MILLIS 11 // min index to switch to ms instead of ns display
#define TIMEBASE_INDEX_MICROS 2 // min index to switch to us instead of ns display
#endif

extern const float TimebaseExactDivValuesMicros[] PROGMEM;
#define HORIZONTAL_LINE_LABELS_CAPION_X (REMOTE_DISPLAY_WIDTH - TEXT_SIZE_11_WIDTH * 4)
/*
 * OFFSET
 */
#define OFFSET_MODE_0_VOLT 0
#define OFFSET_MODE_AUTOMATIC 1
#define OFFSET_MODE_MANUAL 2    // not implemented for AVR Implies range mode manual.

/*
 * GUI LAYOUT,  POSITIONS + SIZES
 */
#define INFO_UPPER_MARGIN (1 + TEXT_SIZE_11_ASCEND)
#define INFO_LEFT_MARGIN 0

#ifdef AVR
#define FONT_SIZE_INFO_SHORT        TEXT_SIZE_18    // for 1 line info
#define FONT_SIZE_INFO_LONG         TEXT_SIZE_11    // for 2 lines info
#define FONT_SIZE_INFO_SHORT_ASC    TEXT_SIZE_18_ASCEND
#define FONT_SIZE_INFO_LONG_ASC     TEXT_SIZE_11_ASCEND
#define FONT_SIZE_INFO_LONG_WIDTH   TEXT_SIZE_11_WIDTH

#define SLIDER_SIZE 24
#define SLIDER_VPICKER_POS_X        0 // Position of slider
#define SLIDER_VPICKER_INFO_X       (SLIDER_VPICKER_POS_X + SLIDER_SIZE)
#define SLIDER_VPICKER_INFO_SHORT_Y (FONT_SIZE_INFO_SHORT + FONT_SIZE_INFO_SHORT_ASC)
#define SLIDER_VPICKER_INFO_LONG_Y  (2 * FONT_SIZE_INFO_LONG + FONT_SIZE_INFO_SHORT_ASC) // since font size is always 18

#define SLIDER_TLEVEL_POS_X         (14 * FONT_SIZE_INFO_LONG_WIDTH) // Position of slider
#define TRIGGER_LEVEL_INFO_SHORT_X  (SLIDER_TLEVEL_POS_X  + SLIDER_SIZE)
#define TRIGGER_LEVEL_INFO_LONG_X   (INFO_LEFT_MARGIN + (36 * FONT_SIZE_INFO_LONG_WIDTH))
#define TRIGGER_LEVEL_INFO_SHORT_Y  (FONT_SIZE_INFO_SHORT + FONT_SIZE_INFO_SHORT_ASC)
#define TRIGGER_LEVEL_INFO_LONG_Y   FONT_SIZE_INFO_LONG_ASC

#define SETTINGS_PAGE_ROW_INCREMENT BUTTON_HEIGHT_5_256_LINE_2
#define SETTINGS_PAGE_BUTTON_HEIGHT BUTTON_HEIGHT_5_256
#define START_PAGE_ROW_INCREMENT BUTTON_HEIGHT_4_256_LINE_2
#define START_PAGE_BUTTON_HEIGHT BUTTON_HEIGHT_4_256

#define SINGLESHOT_PPRINT_VALUE_X (REMOTE_DISPLAY_WIDTH - TEXT_SIZE_11_WIDTH)
#define SETTINGS_PAGE_INFO_Y (BUTTON_HEIGHT_5_256_LINE_5 - (TEXT_SIZE_11_DECEND + 1))
#else
#ifdef LOCAL_DISPLAY_EXISTS
#define FONT_SIZE_INFO_SHORT        TEXT_SIZE_11    // for 1 line info
#define FONT_SIZE_INFO_LONG         TEXT_SIZE_11    // for 3 lines info
#define FONT_SIZE_INFO_SHORT_ASC    TEXT_SIZE_11_ASCEND    // for 3 lines info
#define FONT_SIZE_INFO_LONG_ASC     TEXT_SIZE_11_ASCEND    // for 3 lines info
#define FONT_SIZE_INFO_LONG_WIDTH   TEXT_SIZE_11_WIDTH    // for 3 lines info
#else
#define FONT_SIZE_INFO_SHORT        TEXT_SIZE_16    // for 1 line info
#define FONT_SIZE_INFO_LONG         TEXT_SIZE_14    // for 3 lines info
#define FONT_SIZE_INFO_SHORT_ASC    TEXT_SIZE_16_ASCEND    // for 3 lines info
#define FONT_SIZE_INFO_LONG_ASC     TEXT_SIZE_14_ASCEND    // for 3 lines info
#define FONT_SIZE_INFO_LONG_WIDTH   TEXT_SIZE_14_WIDTH    // for 3 lines info
#endif

#define SLIDER_SIZE 24
#define SLIDER_VPICKER_POS_X        0 // Position of slider
#define SLIDER_VPICKER_INFO_X       (SLIDER_VPICKER_POS_X + SLIDER_SIZE)
#define SLIDER_VPICKER_INFO_SHORT_Y (FONT_SIZE_INFO_SHORT + FONT_SIZE_INFO_SHORT_ASC)
#define SLIDER_VPICKER_INFO_LONG_Y  (3 * FONT_SIZE_INFO_LONG + FONT_SIZE_INFO_SHORT_ASC) // since font size is always 18

#define SLIDER_TLEVEL_POS_X         (14 * FONT_SIZE_INFO_LONG_WIDTH) // Position of slider
#define TRIGGER_LEVEL_INFO_SHORT_X  (SLIDER_TLEVEL_POS_X  + SLIDER_SIZE)
#ifdef LOCAL_DISPLAY_EXISTS
#define TRIGGER_LEVEL_INFO_LONG_X   (11 * FONT_SIZE_INFO_LONG_WIDTH)
#else
#define TRIGGER_LEVEL_INFO_LONG_X   (11 * FONT_SIZE_INFO_LONG_WIDTH +1) // +1 since we have a special character in the string before
#endif
#define TRIGGER_LEVEL_INFO_SHORT_Y  (FONT_SIZE_INFO_SHORT + FONT_SIZE_INFO_SHORT_ASC)
#define TRIGGER_LEVEL_INFO_LONG_Y   (2 * FONT_SIZE_INFO_LONG  + FONT_SIZE_INFO_LONG_ASC)

#define TRIGGER_HIGH_DISPLAY_OFFSET 7 // for trigger state line
#define SETTINGS_PAGE_ROW_INCREMENT BUTTON_HEIGHT_5_LINE_2
#define SETTINGS_PAGE_BUTTON_HEIGHT BUTTON_HEIGHT_5
#define START_PAGE_ROW_INCREMENT BUTTON_HEIGHT_4_LINE_2
#define START_PAGE_BUTTON_HEIGHT BUTTON_HEIGHT_4
#endif

extern uint8_t sLastPickerValue;

#ifdef AVR
extern BDButton TouchButtonADCReference;
#else
extern BDButton TouchButtonFFT;
extern BDButton TouchButtonShowPretriggerValuesOnOff;
extern BDButton TouchButtonDSOMoreSettings;
extern BDButton TouchButtonCalibrateVoltage;
extern BDButton TouchButtonMinMaxMode;
extern BDButton TouchButtonDrawModeTriggerLine;
#endif
#ifdef LOCAL_DISPLAY_EXISTS
extern BDButton TouchButtonDrawModeLinePixel;
extern BDButton TouchButtonADS7846TestOnOff;
extern BDSlider TouchSliderBacklight;
#endif

extern BDButton TouchButtonSingleshot;
extern BDButton TouchButtonStartStopDSOMeasurement;

extern BDButton TouchButtonSlope;
extern BDButton TouchButtonTriggerMode;
extern BDButton TouchButtonTriggerDelay;
extern BDButton TouchButtonChartHistoryOnOff;
extern BDButton TouchButtonSlope;
extern BDButton TouchButtonAcDc;
extern BDButton TouchButtonChannels[];
extern BDButton TouchButtonChannelSelect;
extern BDButton TouchButtonChannelMode;
extern BDButton TouchButtonAutoOffsetMode;
extern const char AutoOffsetButtonStringMan[];
extern const char AutoOffsetButtonStringAuto[];
extern const char AutoOffsetButtonString0[];
extern BDButton TouchButtonAutoRangeOnOff;
extern BDButton TouchButtonSettingsPage;
extern BDButton TouchButtonFrequencyPage;

extern char SlopeButtonString[];
// the index of the slope indicator in char array
#define SLOPE_STRING_INDEX 6

extern BDButton TouchButtonTriggerMode;
extern const char TriggerModeButtonStringAuto[] PROGMEM;
extern const char TriggerModeButtonStringManualTimeout[] PROGMEM;
extern const char TriggerModeButtonStringManual[] PROGMEM;
extern const char TriggerModeButtonStringFree[] PROGMEM;
extern const char TriggerModeButtonStringExtern[] PROGMEM;

extern BDButton TouchButtonAutoRangeOnOff;
extern const char AutoRangeButtonStringAuto[] PROGMEM;
extern const char AutoRangeButtonStringManual[] PROGMEM;

extern BDSlider TouchSliderTriggerLevel;
extern BDSlider TouchSliderVoltagePicker;

// Measurement section
void startAcquisition(void);
void prepareForStart(void);
void setChannel(uint8_t aChannel);

// Stack info
void initStackFreeMeasurement(void);
uint16_t getStackFreeMinimumBytes(void);
void printFreeStack(void);

// Data analysis section
void computeMinMax(void);
void computePeriodFrequency(void);
void setTriggerLevelAndHysteresis(int aRawTriggerValue, int aRawTriggerHysteresis);

// Logic section
void resetOffset(void);
void setOffsetAutomatic(bool aNewState);
void setACMode(bool aNewACMode);
int changeOffsetGridCount(int aValue);
#ifdef AVR
uint8_t changeRange(int8_t aChangeAmount);
uint8_t changeTimeBaseValue(int8_t aChangeValue);
#else
int changeDisplayRangeAndAdjustOffsetGridCount(int aValue);
int changeTimeBaseValue(int aChangeValue);

#endif
int changeXScale(int aValue);

// Output and draw section
void initDSOGUI(void);

void redrawDisplay(void);
void drawStartPage(void);
void drawDSOSettingsPage(void);
void drawDSOMoreSettingsPage(void);

void drawGridLinesWithHorizLabelsAndTriggerLine();
void clearHorizontalLineAndRestoreGrid(int aYposition);
void drawTriggerLine(void);
void drawMinMaxLines(void);
void clearTriggerLine(uint8_t aTriggerLevelDisplayValue);
void drawRunningOnlyPartOfGui(void);
void activateChartGui(void);
#ifdef AVR
uint8_t scrollChart(int aValue);
uint8_t getDisplayFromRawInputValue(uint16_t aRawValue);
void drawDataBuffer(uint8_t *aByteBuffer, uint16_t aColor, uint16_t aClearBeforeColor);
#else
int scrollChart(int aValue);
int getDisplayFromRawInputValue(int aAdcValue);
void drawDataBuffer(uint16_t *aDataBufferPointer, int aLength, color16_t aColor, color16_t aClearBeforeColor, int aDrawMode,
        bool aDrawAlsoMin);
void startSystemInfoPage(void);
#endif

// Text output section
void printfTriggerDelay(char * aDataBufferPtr, uint16_t aTriggerDelayMillisOrMicros);
void printVCCAndTemperature(void);
void clearInfo(uint8_t aOldMode);
void printInfo(bool aRecomputeValues = true);
void printTriggerInfo(void);

// GUI event handler section
void doSwitchInfoModeOnTouchUp(struct TouchEvent * const aTochPosition);
void doLongTouchDownDSO(struct TouchEvent * const aTochPosition);
void doSwipeEndDSO(struct Swipe * const aSwipeInfo);
void doSetTriggerDelay(float aValue);

// Button handler section
#ifdef AVR
void doADCReference(BDButton * aTheTouchedButton, int16_t aValue);
#else
void doShowPretriggerValuesOnOff(BDButton * aTheTouchedButton, int16_t aValue);
void doShowFFT(BDButton * aTheTouchedButton, int16_t aValue);
void doMinMaxMode(BDButton * aTheTouchedButton, int16_t aValue);
void doShowMoreSettingsPage(BDButton * aTheTouchedButton, int16_t aValue);
void doShowSystemInfoPage(BDButton * aTheTouchedButton, int16_t aValue);
void doVoltageCalibration(BDButton * aTheTouchedButton, int16_t aValue);
void doDrawModeTriggerLine(BDButton * aTheTouchedButton, int16_t aValue);
#endif
void doStartStopDSO(BDButton * aTheTouchedButton, int16_t aValue);
void doDefaultBackButton(BDButton * aTheTouchedButton, int16_t aValue);
void doShowFrequencyPage(BDButton * aTheTouchedButton, int16_t aValue);
void doShowSettingsPage(BDButton * aTheTouchedButton, int16_t aValue);
void doTriggerSlope(BDButton * aTheTouchedButton, int16_t aValue);
void doStartSingleshot(BDButton * aTheTouchedButton, int16_t aValue);
void doTriggerMode(BDButton * aTheTouchedButton, int16_t aValue);
void doRangeMode(BDButton * aTheTouchedButton, int16_t aValue);
void doChartHistory(BDButton * aTheTouchedButton, int16_t aValue);
void doPromptForTriggerDelay(BDButton * aTheTouchedButton, int16_t aValue);
void doChannelSelect(BDButton * aTheTouchedButton, int16_t aValue);
void doOffsetMode(BDButton * aTheTouchedButton, int16_t aValue);
void doAcDcMode(BDButton * aTheTouchedButton, int16_t aValue);
#ifdef LOCAL_DISPLAY_EXISTS
void readADS7846Channels(void);
void doADS7846TestOnOff(BDButton * aTheTouchedButton, int16_t aValue);
void doDrawMode(BDButton * aTheTouchedButton, int16_t aValue);
#endif

// Slider handler section
void doTriggerLevel(BDSlider * aTheTouchedSlider, uint16_t aValue);
void doVoltagePicker(BDSlider * aTheTouchedSlider, uint16_t aValue);

// Button caption section
#ifdef AVR
#else
void setMinMaxModeButtonCaption(void);
#endif

void setSlopeButtonCaption(void);
void setTriggerModeButtonCaption(void);
void setAutoRangeModeAndButtonCaption(bool aNewAutoRangeMode);
void setChannelButtonsCaption(void);
void setReferenceButtonCaption(void);
void setACModeButtonCaption(void);
void setTriggerDelayCaption(void);
void setAutoOffsetButtonCaption(void);

uint32_t getMicrosFromHorizontalDisplayValue(uint16_t aDisplayValueHorizontal, uint8_t aNumberOfPeriods);

#ifdef AVR
#else
#endif
#ifndef AVR
#endif

#endif // TOUCHDSOCOMMON_H_
