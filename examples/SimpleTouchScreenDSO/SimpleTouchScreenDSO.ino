/*
 * SimpleTouchScreenDSO.cpp
 *
 * This program converts your Arduino UNO with a mSD-Shield and a MI0283QT-2 adapter to a simple DSO
 *
 *  Copyright (C) 2012-2023  Armin Joachimsmeyer
 *  Email: armin.joachimsmeyer@gmail.com
 *
 *  This file is part of SimpleTouchScreenDSO https://github.com/ArminJo/SimpleTouchScreenDSO.
 *
 *  SimpleTouchScreenDSO is free software: you can redistribute it and/or modify
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
 *
 *      Features:
 *      No dedicated hardware, just off the shelf components + c software
 *      300 kSamples per second
 *      Automatic trigger and offset value selection
 *      700 Byte data buffer
 *      Min max and average display
 *      Fast draw mode. Up to 45 screens per second in pixel mode
 *      Switching between fast pixel and slower line display mode
 *      All settings can be changed during measurement by touching the (invisible) buttons
 *      All AVR ADC input channels selectable
 *      Touch trigger level select
 *      5 Volt or internal 1.1 Volt Reference
 *
 *      Build on:
 *      Arduino Uno Rev 3 https://shop.watterott.com/Arduino-Uno-R3                                 24 EUR
 *      mSD-Shield https://shop.watterott.com/mSD-Shield-v2-microSD-RTC-Datenlogger-Shield          16.5 EUR
 *      MI0283QT-2 Adapter https://shop.watterott.com/MI0283QT-Adapter-v1-inkl-28-LCD-Touchpanel    32.5 EUR
 *      Total                                                                                       73 EUR
 *
 */

/*
 * DOCUMENTATION
 *
 * INFO LINE
 * 101us    - Time for div (31 pixel)
 * A        - Slope of trigger A=ascending D=descending
 * C3       - Input channel (1-5) and T=AVR-temperature R=1.1Volt-internal-reference G=internal-ground
 * 1.35 2.40 4.38 - Minimum, arithmetic-average and, maximum voltage of current chart (In hold mode, chart is larger than display!)
 * 3.03V    - Peak to peak voltage of current chart (In hold mode, chart is larger than display!)
 * 0.86V    - Value of current trigger level
 * 5        - Reference used 5=5V 1  1=1.1Volt-internal-reference
 *
 * SECOND LINE
 * 20ms     - Period
 * 50Hz     - Frequency
 * 0.45     - Value of voltage picker line
 *
 * BUTTONS
 *
 * display
 * while running, switch between upper info line on/off
 * while stopped, switch between:
 * 1. chart+ voltage picker
 * 2. chart + info line + grid + voltage picker
 * 3. only gui buttons
 *
 *
 * ">" and "<" buttons
 * while running, change timebase
 * while stopped, horizontal scroll through data buffer
 *
 */

/*
 *
 * Safety circuit and AC/DC switch
 * 3 resistors   2 diodes   1 capacitor   1 switch
 *
 *                ADC INPUT PIN
 *                      /\
 *                      |
 *   VSS-------|<|------+-----|<|-----+-GND
 *                      |             |
 *   VREF----|R 2.2M|---+---|R 2.2M|--+
 *                      |
 *                      |
 *            \   o-----+
 *             \        |
 *    AC/DC     \       =  C 0.1uF
 *    Switch     \      |
 *                o-----+
 *                      |
 *                      |
 *                      +---|R 1K  |-----<  INPUT DIRECT
 *                      |
 *                      |
 *                      +---|R 2.2M|-+---<  INPUT 10 VOLT
 *                      +---|R 2.2M|-+
 *                      +------||----+
 *                      |  app. 80 pF (adjustable trimmer)
 *                      |
 *  ATTENUATOR          +---|R 3.3M|-+---<  INPUT 20 VOLT
 *                      +------||----+
 *                      |  app. 25 pF
 *                      |
 *                      +---|R 4.7M|-|R 4.7M|--+---<  INPUT 50 VOLT
 *                      |                      |
 *                      +-----------||---------+
 *                               app. 10 pF
 *
 */

/*
 * 5.0 V / 1024.0 = 0,00488 V is the resolution of the ADC
 * Thus, depending of scale factor, the printed values for 2 adjacent display (min/max) values can be the same
 * or the picker value does not reflect the real sample value (e.g. shown for min/max)
 */

//#define TRACE
//#define DEBUG
//#define TIMING_DEBUG
//#define USE_RTC  //if a DS1307 is connected to the I2C bus - is checked at startup :-)
#include <Arduino.h>

/*
 * Settings to configure the LocalTouchGUI library
 */
#define LOCAL_GUI_FEEDBACK_TONE_PIN 2
#define SUPPORT_ONLY_TEXT_SIZE_11_AND_22  // Saves 248 bytes program memory
#define DISABLE_REMOTE_DISPLAY  // Suppress drawing to Bluetooth connected display. Allow only drawing on the locally attached display
#define SUPPORT_LOCAL_DISPLAY   // Supports simultaneously drawing on the locally attached display. Not (yet) implemented for all commands!
#define FONT_8X12               // Font size used here
#include "LocalHX8347DDisplay.hpp" // The implementation of the local display must be included first since it defines LOCAL_DISPLAY_HEIGHT etc.
#include "LocalGUI.hpp"         // Includes the sources for LocalTouchButton etc.

#ifdef USE_RTC
#ifdef __cplusplus
extern "C" {
#include <i2cmaster.h>
}
#endif
#define DS1307_ADDR 0xD0 // 0x68 shifted left
bool rtcAvailable = false;
#endif
/**
 * PINS
 * 2    Tone
 * 3    Timing Debug / (IRQ for ADS7846)
 * 4    CS          SD CARD
 * 5    PAIRED      BlueDisplay / (BUSY far ADS7846)
 * 6    CS          ADS7846
 * 7    CS          MI0283QT2
 * 8    RST         MI0283QT2
 * 9    LED         MI0283QT2
 * 10
 * 11   MOSI        MI0283QT2 / ADS7846
 * 12   MISO        MI0283QT2 / ADS7846
 * 13   CLK         MI0283QT2 / ADS7846
 */
#if defined(TIMING_DEBUG)
#define TIMING_DEBUG_PIN PORTD3 // PIN 3
#define TIMING_DEBUG_PORT PORTD
#define TIMING_DEBUG_PORT_INPUT PIND
inline void toggleTimingDebug() {
    TIMING_DEBUG_PORT_INPUT = (1 << TIMING_DEBUG_PIN);
}
inline void setTimingDebug() {
    TIMING_DEBUG_PORT |= (1 << TIMING_DEBUG_PIN);
}
inline void resetTimingDebug() {
    TIMING_DEBUG_PORT &= ~(1 << TIMING_DEBUG_PIN);
}
#endif

// Data buffer size (must be small enough to leave appr. 7 % (144 Byte) for stack
#define DATABUFFER_SIZE   580

#define ADC_START_CHANNEL   3  // see also ChannelSelectButtonString
#define INFO_UPPER_MARGIN   (1 + TEXT_SIZE_11_ASCEND)
#define INFO_LINE_SPACING   2
#define INFO_LEFT_MARGIN    4

#define THOUSANDS_SEPARATOR '.'
/*
 * COLORS
 */
#define COLOR_BACKGROUND_DSO        COLOR16_WHITE

// Data colors
#define COLOR_DATA_RUN              COLOR16_BLUE
#define COLOR_DATA_HOLD             COLOR16_RED
// to see old chart values
#define COLOR_DATA_ERASE_LOW        COLOR16(0xA0,0xFF,0xA0)
#define COLOR_DATA_ERASE_HIGH       COLOR16(0x20,0xFF,0x20)

//Line colors
#define COLOR_VOLTAGE_PICKER        COLOR16_YELLOW
#define COLOR_VOLTAGE_PICKER_SLIDER COLOR16(0xFF,0XFF,0xB0) // Light Yellow
#define COLOR_TRIGGER_LINE          COLOR16_PURPLE
#define COLOR_TRIGGER_SLIDER        COLOR16(0xFF,0XD0,0xFF) // light Magenta

#define COLOR_MAX_MIN_LINE          0X0200 // light green
#define COLOR_HOR_REF_LINE_LABEL    COLOR16_BLUE
#define COLOR_TIMING_LINES          COLOR16(0x00,0x98,0x00)

// GUI element colors
#define COLOR_GUI_CONTROL           COLOR16(0xC0,0x00,0x00)
#define COLOR_GUI_TRIGGER           COLOR16(0x00,0x00,0xD0) // blue
#define COLOR_GUI_SOURCE_TIMEBASE   COLOR16(0x00,0x90,0x00)
#define COLOR_GUI_DISPLAY_CONTROL   COLOR16(0xC8,0xC8,0x00)

#define COLOR_INFO_BACKGROUND       COLOR16(0xC8,0xC8,0x00)

#define COLOR_SLIDER                COLOR16(0xD0,0xE0,0xD0)

/**
 * GUI
 */
#define TP_EEPROMADDR (E2END -1 - sizeof(CAL_MATRIX)) //eeprom address for calibration data - 28 bytes
bool sTouchPanelGetPressureValid = false; // Flag to indicate that TouchPanel.service was just called before

/**********************
 * Buttons
 *********************/
#define BUTTON_SPACING 15

LocalTouchButton TouchButtonStartStop;

LocalTouchButton TouchButtonLeft;
LocalTouchButton TouchButtonRight;

//LocalTouchButton TouchButtonTestTimeIncrement;
//LocalTouchButton TouchButtonTestTimeDecrement;

LocalTouchButton TouchButtonAutoTriggerOnOff;

// the warning "only initialized variables can be placed into program memory area"
// is because of a known bug of the gnu avr complier version
const char AutoTriggerButtonStringAuto[] PROGMEM = "Trigger\nauto";
const char AutoTriggerButtonStringManual[] PROGMEM = "Trigger\nman";

LocalTouchButton TouchButtonAutoOffsetOnOff;
const char AutoOffsetButtonStringAuto[] PROGMEM = "Offset\nauto";
const char AutoOffsetButtonString0[] PROGMEM = "Offset\n0V";

LocalTouchButton TouchButtonAutoRangeOnOff;
const char AutoRangeButtonStringAuto[] PROGMEM = "Range\nauto";
const char AutoRangeButtonStringManual[] PROGMEM = "Range\nman";

LocalTouchButton TouchButtonADCReference;
const char ReferenceButton5_0V[] PROGMEM = "Ref\n5.0V";
const char ReferenceButton1_1V[] PROGMEM = "Ref\n1.1V";

LocalTouchButton TouchButtonChannelSelect;

#ifdef USE_RTC
// since channel 4 + 5 are used for HW SPI
#define MAX_ADC_CHANNEL 3
#else
#define MAX_ADC_CHANNEL 5
#endif
char ChannelSelectButtonString[] = "Channel\n3";
#define CHANNEL_STRING_INDEX 8 // the index of the channel number in char array
LocalTouchButton TouchButtonDisplayMode;

LocalTouchButton TouchButtonSettings;
const char ButtonStringSingleshot[] PROGMEM = "Single";
const char ButtonStringBack[] PROGMEM = "Back";

LocalTouchButton TouchButtonBack_Singleshot;
#define SINGLESHOT_PPRINT_VALUE_X (37 * TEXT_SIZE_11_WIDTH)

LocalTouchButton TouchButtonSlope;
const char SlopeButtonStringAscending[] PROGMEM = "Slope\nascending";
const char SlopeButtonStringDecending[] PROGMEM = "Slope\ndecending";

LocalTouchButton TouchButtonChartHistory;
const char ChartHistoryButtonStringOff[] PROGMEM = "History\noff";
const char ChartHistoryButtonStringLow[] PROGMEM = "History\nlow";
const char ChartHistoryButtonStringOn[] PROGMEM = "History\non";
const char *const ChartHistoryButtonStrings[] = { ChartHistoryButtonStringOff, ChartHistoryButtonStringLow,
        ChartHistoryButtonStringOn };

/*
 * Backlight
 */
#define BACKLIGHT_MIN_BRIGHTNESS_VALUE 1
LocalTouchSlider TouchSliderBacklight;

#define SLIDER_TLEVEL_VALUE_X (24 * TEXT_SIZE_11_WIDTH)
#define SLIDER_VPICKER_VALUE_X (19 * TEXT_SIZE_11_WIDTH)

LocalTouchSlider TouchSliderTriggerLevel;

LocalTouchSlider TouchSliderVoltagePicker;
uint8_t sLastVoltagePickerValue;

/*
 * Display control
 */
#define DISPLAY_MODE_SHOW_MAIN_GUI      0 // Show main GUI and no data
#define DISPLAY_MODE_CHART              1 // Show grid and data, do not show main GUI
#define DISPLAY_MODE_SHOW_SETTINGS_GUI  2 // Settings page

uint16_t EraseColors[] = { COLOR_BACKGROUND_DSO, COLOR_DATA_ERASE_LOW, COLOR_DATA_ERASE_HIGH };
struct DisplayControlStruct {
    uint8_t TriggerLevelDisplayValue; // For clearing old line of manual trigger level setting
    uint8_t DisplayMode;
    bool showInfoMode;
    uint8_t EraseColorIndex;
    uint16_t EraseColor; // The color for erasing old chart. If != WHITE we have a history effect.
};
DisplayControlStruct DisplayControl;

/*****************************
 * Timebase stuff
 *****************************/
#define PRESCALE4    2
#define PRESCALE32   5
#define PRESCALE64   6
#define PRESCALE128  7

#define PRESCALE_MIN_VALUE PRESCALE4
#define PRESCALE_MAX_VALUE PRESCALE128
#define PRESCALE_START_VALUE PRESCALE128
#define TIMEBASE_INDEX_START_VALUE PRESCALE_START_VALUE - PRESCALE_MIN_VALUE

/*
 * Different Acquisition modes depending on Timebase:
 * Mode ultrafast - ADC free running - one loop for read and store 10 bit => needs double buffer space - interrupts blocked for duration of loop
 * mode fast      - ADC free running - one loop for read pre process 10 -> 8 Bit and store - interrupts blocked for duration of loop
 * mode isr without delay - ADC generates Interrupts - because of ISR initial delay for push just start next conversion immediately
 * mode isr with delay    - ADC generates Interrupts - busy delay, then start next conversion to match 1,2,5 timebase scale
 * mode isr with multiple read - ADC generates Interrupts - to avoid excessive busy delays, start one ore more intermediate conversion just for delay purposes
 */

#define HORIZONTAL_GRID_COUNT 6
/**
 * Formula for Grid Height is:
 * 5V = 1024 => 1024/5 Pixel per Volt
 * 0.2 Volt -> 40.96 pixel
 * 0.5 Volt -> 102.4 pixel with scale (shift) 1 => 51.2 pixel
 * 1.1V Reference
 * 0.05 Volt -> 46.545454 pixel
 * 0.5 Volt -> 465.45 pixel with scale (shift) 3 => 58.181818 pixel
 */
#define HORIZONTAL_GRID_HEIGHT_5V_SHIFT8 13094 // 51.15*256=13094.4 for 0.5 to 2.0 Volt /div
#define HORIZONTAL_GRID_HEIGHT_5V_02_Range_SHIFT8 10476 // ((1023/5)*0.2)*256=10475.52 for 0.2 Volt /div (shift = 0)
#define HORIZONTAL_GRID_HEIGHT_1_1V_SHIFT8 11904 // 46.5*256 for 0.05 to 0.2 Volt /div
#define HORIZONTAL_GRID_HEIGHT_1_1V_05_Range_SHIFT8 14848 // 58
#define ADC_CYCLES_PER_CONVERSION 13
#define TIMING_GRID_WIDTH 31 // with 31 instead of 32 the values fit better to 1-2-5 timebase scale
#define TIMEBASE_NUMBER_OF_ENTRIES 12 // the number of different timebases provided
#define TIMEBASE_INDEX_ULTRAFAST_MODE 0 // first mode is ultrafast
#define TIMEBASE_INDEX_FAST_MODES 1 // first 2 modes are fast free running modes
#define TIMEBASE_INDEX_MILLIS 3 // min index to switch to ms instead of us display
#define TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE 6 // min index where chart is drawn while buffer is filled
// the delay between ADC end of conversion and first start of delay loop in ISR
#define ISR_ZERO_DELAY_MICROS 3
#define ISR_DELAY_MICROS 6 // delay from interrupt to delay code
#define ADC_CONVERSION_AS_DELAY_MICROS 112 // 448
// Values for Prescale 4(13*0,25=3,25us),8(6,5),16(13),32(26),64(51),128(104us) and above
// +1 -10 and -24 are manual correction values for the 1/4 microsecond resolution
// resolution of delay is 1/4 microsecond
const uint16_t TimebaseDelayValues[TIMEBASE_NUMBER_OF_ENTRIES] = { 0, 0, 3 - ISR_ZERO_DELAY_MICROS, //
        ((16 - ADC_CYCLES_PER_CONVERSION) * 2 - ISR_DELAY_MICROS) * 4 + 1, // 1   |   1 -   3
        ((16 - ADC_CYCLES_PER_CONVERSION) * 4 - ISR_DELAY_MICROS) * 4 - 10, // 14  |  13 -  27
        ((20 - ADC_CYCLES_PER_CONVERSION) * 8 - ISR_DELAY_MICROS) * 4 - 24, // 176 | 172 - 203
        ((40 - ADC_CYCLES_PER_CONVERSION) * 8 - ISR_DELAY_MICROS) * 4 - 24, // 816 | 812 - 843
        ((81 - ADC_CYCLES_PER_CONVERSION) * 8 - ISR_DELAY_MICROS) * 4 - 24, // 2128|
        ((202 - ADC_CYCLES_PER_CONVERSION) * 8 - ISR_DELAY_MICROS) * 4 - 24, //
        ((403 - ADC_CYCLES_PER_CONVERSION) * 8 - ISR_DELAY_MICROS) * 4 - 24, //
        ((806 - ADC_CYCLES_PER_CONVERSION) * 8 - ISR_DELAY_MICROS) * 4 - 24, //
        ((2016 - ADC_CYCLES_PER_CONVERSION) * 8 - ISR_DELAY_MICROS) * 4U - 24, // 64048 - and no integer overflow!
        };
// for 31 grid
uint16_t TimebaseDivPrintValues[TIMEBASE_NUMBER_OF_ENTRIES] = { 101, 201, 496, 1, 2, 5, 10, 20, 50, 100, 200, 500 };
// exact values for 31 grid - for period and frequency
const uint32_t TimebaseDivExactValues[TIMEBASE_NUMBER_OF_ENTRIES] PROGMEM = { 101 /*100,75 (31*13*0,25)*/,
        201 /*201,5 (31*13*0,5)*/, 496 /*(31*16*1)*/, 992 /*(31*16*2)*/, 1984 /*(31*16*4)*/, 4960 /*(31*20*8)*/, 9920 /*(31*40*8)*/,
        20088 /*(31*81*8)*/, 50096 /*(31*202*8)*/, 99944 /*(31*403*8)*/, 199888 /*(31*806*8)*/, 499968 /*(31*2016*8)*/};

/****************************************
 * Automatic triggering and range stuff
 */
#define TRIGGER_WAIT_NUMBER_OF_SAMPLES (10 * LOCAL_DISPLAY_WIDTH) // Number of "DISPLAY_WIDTH" Samples used for detecting the trigger condition
// States of tTriggerStatus
#define TRIGGER_START 0 // No trigger condition met
#define TRIGGER_BEFORE_THRESHOLD 1 // slope condition met, wait to go beyond threshold hysteresis
#define TRIGGER_OK 2 // Trigger condition met
#define TRIGGER_HYSTERESIS_MANUAL 2 // value for effective trigger hysteresis in manual trigger mode
#define TRIGGER_HYSTERESIS_MIN 0x08 // min value for automatic effective trigger hysteresis
#define ADC_CYCLES_PER_CONVERSION 13
#define SCALE_CHANGE_DELAY_MILLIS 2000

/******************************
 * Measurement control values
 */
struct MeasurementControlStruct {
    // State
    bool IsRunning;
    bool StopRequested;
    // Trigger flag for ISR and single shot mode
    bool searchForTrigger;
    bool isSingleShotMode;

    uint8_t ADCReference; // DEFAULT = 1 =VCC   INTERNAL = 3 = 1.1V
    // Input select
    uint8_t ADCInputMUXChannel;
    char ADCInputMUXChannelChar;

    // Trigger
    bool TriggerSlopeRising;
    uint16_t RawTriggerLevel;
    uint16_t TriggerLevelUpper;
    uint16_t TriggerLevelLower;
    uint16_t ValueBeforeTrigger;

    bool TriggerValuesAutomatic; // adjust values automatically
    bool OffsetAutomatic; // false -> offset = 0 Volt
    uint8_t TriggerStatus;
    uint16_t TriggerSampleCount; // for trigger timeout
    uint16_t TriggerTimeoutSampleCount; // ISR max samples before trigger timeout

    // Statistics (for info and auto trigger)
    uint16_t RawValueMin;
    uint16_t RawValueMax;
    uint16_t ValueMinForISR;
    uint16_t ValueMaxForISR;
    uint16_t ValueAverage;
    uint32_t IntegrateValueForAverage;
    uint32_t PeriodMicros;

    // Timebase
    bool TimebaseFastFreerunningMode;
    uint8_t TimebaseIndex;
    // volatile saves 2 registers push in ISR
    // 1/4 micros loop duration/resolution
    volatile uint16_t TimebaseDelay;
    // remaining micros for long delays
    uint16_t TimebaseDelayRemaining;
    // to signal that it is safe to do TouchPanel.service();
    bool TimebaseJustDelayed;

    bool RangeAutomatic; // RANGE_MODE_AUTOMATIC, MANUAL

    // Shift and scale
    uint16_t ValueOffset;
    uint8_t ValueShift; // shift (division) value  (0-3) for different voltage ranges
    uint16_t HorizontalGridSizeShift8; // depends on shift  for 5V reference 0,02 -> 41 other -> 51.2
    float HorizontalGridVoltage; // voltage per grid for offset etc.
    uint8_t NumberOfGridLinesToSkipForOffset; // number of bottom line for offset != 0 Volt.
    uint32_t TimestampLastRangeChange;

} MeasurementControl;

// Union to speed up the combination of low and high bytes to a word
// it is not optimal since the compiler still generates 2 unnecessary moves
// but using  -- value = (high << 8) | low -- gives 5 unnecessary instructions
union Myword {
    struct {
        uint8_t LowByte;
        uint8_t HighByte;
    } byte;
    uint16_t Word;
    uint8_t *BytePointer;
};

// if max value > DISPLAY_USAGE the ValueShift is incremented
#define DISPLAY_USAGE 220
#define DISPLAY_ZERO_OFFSET 1
#define DISPLAY_VALUE_FOR_ZERO (LOCAL_DISPLAY_HEIGHT - 1 - DISPLAY_ZERO_OFFSET)
#define DISPLAY_VALUE_FOR_ZERO_SHIFT8  (((240L-2)*256L)+128)

/***********************
 *   Loop control
 ***********************/
bool ButtonPressedFirst; // if true disables checkAllSliders()
// last sample of millis() in loop as reference for loop duration
uint32_t sMillisLastLoop = 0;
uint16_t sMillisSinceLastDemoOutput = 0;
#define MILLIS_BETWEEN_INFO_OUTPUT 1000

uint16_t MillisSinceLastTPCheck = 0;
#define MILLIS_BETWEEN_TOUCH_PANEL_CHECK 300

/***************
 * Debug stuff
 ***************/
#if defined(DEBUG)
#include "digitalWriteFast.h"
uint16_t DebugValue1;
uint16_t DebugValue2;
uint16_t DebugValue3;
uint16_t DebugValue4;
#endif

/***********************************************************************************
 * Put those variables at the end of data section (.data + .bss + .noinit)
 * adjacent to stack in order to have stack overflows running into
 * DataBuffer array, which is only completely used in ultrafast or single shot mode
 * *********************************************************************************/
/*
 * a string buffer for any purpose...
 */
char sStringBuffer[40] __attribute__((section(".noinit")));
// 40 is #character per screen line (TFTDisplay.getWidth() / TEXT_SIZE_11_WIDTH)
// safety net - array overflow will overwrite only DisplayBuffer

/*
 * Data buffer
 */
#define DATABUFFER_DISPLAY_INCREMENT (TIMING_GRID_WIDTH)

struct DataBufferStruct {
    uint8_t DisplayBuffer[LOCAL_DISPLAY_WIDTH];
    uint8_t *DataBufferNextInPointer;
    uint8_t *DataBufferNextDrawPointer; // pointer to DataBuffer - for draw-while-acquire mode
    uint16_t DataBufferNextDrawIndex; // index in DisplayBuffer - for draw-while-acquire mode
    // to detect end of acquisition in interrupt service routine
    uint8_t *DataBufferEndPointer;
    // Used to synchronize ISR with main loop
    bool DataBufferFull;
    // AcqusitionSize is LOCAL_DISPLAY_WIDTH except on last acquisition before stop then it is DATABUFFER_SIZE
    uint16_t AcquisitionSize;
    // Pointer for horizontal scrolling
    uint8_t *DataBufferDisplayStart;
    uint8_t DataBuffer[DATABUFFER_SIZE];
} DataBufferControl __attribute__((section(".noinit")));

/*******************************************************************************************
 * Function declaration section
 *******************************************************************************************/
void startAcquisition();
void acquireDataFast();
void computeAutoTrigger();
void computeAutoRange();
bool changeInputRange(uint8_t aValue);
void computeAutoOffset();
void createGUI();
void drawGui();
void drawSettingsGui();
void activatePartOfGui();
void redrawDisplay();

void doTimeBaseDisplayScroll(LocalTouchButton *const aTheTouchedButton, int16_t aValue);
void doTriggerAutoOnOff(LocalTouchButton *const aTheTouchedButton, int16_t aValue);
void doTriggerSlope(LocalTouchButton *const aTheTouchedButton, int16_t aValue);
void doAutoOffsetOnOff(LocalTouchButton *const aTheTouchedButton, int16_t aValue);
void doRangeMode(LocalTouchButton *const aTheTouchedButton, int16_t aValue);
void doADCReference(LocalTouchButton *const aTheTouchedButton, int16_t aValue);
void doChannelSelect(LocalTouchButton *const aTheTouchedButton, int16_t aValue);
void doDisplayMode(LocalTouchButton *const aTheTouchedButton, int16_t aValue);
void doSettings(LocalTouchButton *const aTheTouchedButton, int16_t aValue);
void doTriggerSingleshot(LocalTouchButton *const aTheTouchedButton, int16_t aValue);
void doStartStop(LocalTouchButton *const aTheTouchedButton, int16_t aValue);
void doChartHistory(LocalTouchButton *const aTheTouchedButton, int16_t aValue);
void doBacklight(LocalTouchSlider *aTheTouchedSlider, uint16_t aBrightness);
void doTriggerLevel(LocalTouchSlider *aTheTouchedSlider, uint16_t aBrightness);
void doVoltagePicker(LocalTouchSlider *aTheTouchedSlider, uint16_t aBrightness);

void clearTriggerLine(uint8_t aTriggerLevelDisplayValue);
void drawTriggerLine();
void drawGridLinesWithHorizLabelsAndTriggerLine();
void clearHorizontalGridLinesAndHorizontalLineLabels();
void drawGrid();
void drawDataBuffer(uint8_t *aDataBufferPointer, color16_t aColor, color16_t aClearBeforeColor);
void DrawOneDataBufferValue();

void printInfo();
#if defined(DEBUG)
void printTPData(bool aGuiTouched);
void printDebugData();
#endif

void computeMicrosPerPeriod();
void FeedbackTone(bool isNoError);
void FeedbackToneOK();
uint8_t getDisplayFromRawValue(uint16_t aAdcValue);
uint16_t getRawFromDisplayValue(uint8_t aValue);
float getFloatFromDisplayValue(uint8_t aValue);
void setMUX(uint8_t aChannel);
inline void setPrescaleFactor(uint8_t aFactor);
void setReference(uint8_t aReference);

#ifdef USE_RTC
uint8_t bcd2bin(uint8_t val);
uint8_t bin2bcd(uint8_t val);
void showRTCTime();
void setRTCTime(uint8_t * aRTCBuffer, bool aForcedSet);
#endif
/*******************************************************************************************
 * Program code starts here
 * Setup section
 *******************************************************************************************/
void setup() {

    pinMode(LOCAL_GUI_FEEDBACK_TONE_PIN, OUTPUT);
#if defined(TIMING_DEBUG)
    pinMode(TIMING_DEBUG_PIN, OUTPUT);
#endif

    // Shutdown TWI, enable all timers, USART and ADC
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__) || defined(ARDUINO_AVR_LEONARDO) || defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__)
    PRR0 = (1 << PRTWI);
#else
    PRR = (1 << PRTWI);
#endif
    // Disable  digital input on all ADC channel pins to reduce power consumption
    DIDR0 = ADC0D | ADC1D | ADC2D | ADC3D | ADC4D | ADC5D;

#if defined(TRACE)
    Serial.begin(115200);
    // Just to know which program is running on my Arduino
    delay(2000);
    Serial.println(F("START " __FILE__ " from " __DATE__));
#endif

    //init display
    // faster SPI mode - works fine :-)
    LocalDisplay.init(2); //spi-clk = Fcpu/2
//  LocalDisplay.init(4); //spi-clk = Fcpu/4
#if defined(TRACE)
    Serial.println(F("After display init"));
#endif
    //init touch controller
    TouchPanel.initAndCalibrateOnPress(TP_EEPROMADDR);

    createGUI();
    drawGui();

    // initialize values
    MeasurementControl.IsRunning = false;
    MeasurementControl.TriggerSlopeRising = true;
    MeasurementControl.ADCInputMUXChannelChar = '3';
    setMUX(ADC_START_CHANNEL);

    MeasurementControl.TriggerValuesAutomatic = true;

    setReference(DEFAULT); // DEFAULT = 1 =VCC   INTERNAL = 3 = 1.1V

    MeasurementControl.RangeAutomatic = true;
    MeasurementControl.OffsetAutomatic = false;
    MeasurementControl.ValueShift = 2;
    MeasurementControl.HorizontalGridSizeShift8 = HORIZONTAL_GRID_HEIGHT_5V_SHIFT8;
    MeasurementControl.HorizontalGridVoltage = 1;
// changeInputRange(2) saves 30 Bytes but removes and draws the grid;
    /*
     * changeInputRange() needs:
     * ADCReference
     * OffsetAutomatic
     * HorizontalGridSizeShift8
     * changeInputRange()  sets:
     * ValueShift
     * HorizontalGridVoltage
     */

    MeasurementControl.isSingleShotMode = false;
    MeasurementControl.TimebaseFastFreerunningMode = false;
    MeasurementControl.ValueOffset = 0;
    MeasurementControl.TimebaseDelay = TimebaseDelayValues[TIMEBASE_INDEX_START_VALUE];
    MeasurementControl.TimebaseDelayRemaining = TimebaseDelayValues[TIMEBASE_INDEX_START_VALUE];
    MeasurementControl.TimebaseIndex = TIMEBASE_INDEX_START_VALUE;
    MeasurementControl.TriggerTimeoutSampleCount = TRIGGER_WAIT_NUMBER_OF_SAMPLES;
    setPrescaleFactor(PRESCALE_START_VALUE);

    DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[0];

    // Setup ADC

    DisplayControl.EraseColor = EraseColors[0];
    DisplayControl.EraseColorIndex = 0;

#ifdef USE_RTC
// activate internal pullups for twi.
    digitalWrite(SDA, 1);
    digitalWrite(SCL, 1);
    i2c_init();//Set speed

    /*
     * check if a RTC is connected
     */
    if (!i2c_start_timeout(DS1307_ADDR + I2C_WRITE, 60000)) {
        rtcAvailable = true;
        i2c_stop();
        uint8_t RtcBuf[7] = {20, 51, 21, 3, 01, 10, 13};
        //only set the date+time once
        setRTCTime(RtcBuf, true);//sec, min, hour, ,dayOfWeek, day, month, year
    }

#endif

    LocalTouchButton::playFeedbackTone();
    delay(100);
    LocalTouchButton::playFeedbackTone();

#if defined(TRACE)
    Serial.println(F("Start loop"));
#endif

}

/************************************************************************
 * main loop - 32 microseconds
 ************************************************************************/
// noreturn saves program space!
void __attribute__((noreturn)) loop() {
    uint32_t tMillis, tMillisOfLoop;

    for (;;) {
        tMillis = millis();
        tMillisOfLoop = tMillis - sMillisLastLoop;
        sMillisLastLoop = tMillis;
        sMillisSinceLastDemoOutput += tMillisOfLoop;
        MillisSinceLastTPCheck += tMillisOfLoop;

        if (MeasurementControl.IsRunning) {
            bool tDrawWhileAcquire = (MeasurementControl.TimebaseIndex >= TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE);

            if (MeasurementControl.TimebaseFastFreerunningMode) {
                /*
                 * Fast mode here
                 */
                acquireDataFast();
            }
            if (DataBufferControl.DataBufferFull) {
                /*
                 * Data (from InterruptServiceRoutine) is ready
                 */
                MeasurementControl.ValueAverage = (MeasurementControl.IntegrateValueForAverage
                        + (DataBufferControl.AcquisitionSize / 2)) / DataBufferControl.AcquisitionSize;
                uint8_t tLastTriggerDisplayValue = DisplayControl.TriggerLevelDisplayValue;

                computeAutoTrigger();
                computeAutoRange();
                computeAutoOffset();
                // handle trigger line
                DisplayControl.TriggerLevelDisplayValue = getDisplayFromRawValue(MeasurementControl.RawTriggerLevel);

                if (tLastTriggerDisplayValue != DisplayControl.TriggerLevelDisplayValue) {
                    clearTriggerLine(tLastTriggerDisplayValue);
                    drawTriggerLine();
                }

                TouchPanel.readData();
                sTouchPanelGetPressureValid = true;

                if (MeasurementControl.StopRequested) {
                    MeasurementControl.StopRequested = false;
                    // delayed tone for stop
                    FeedbackToneOK();
                    if (MeasurementControl.isSingleShotMode) {
                        // Clear singleshot character
                        LocalDisplay.drawChar(INFO_LEFT_MARGIN + SINGLESHOT_PPRINT_VALUE_X, INFO_UPPER_MARGIN + TEXT_SIZE_11_HEIGHT,
                                ' ', TEXT_SIZE_11, COLOR16_BLACK, COLOR_BACKGROUND_DSO);
                    }
                    // Clear old chart
                    drawDataBuffer(&DataBufferControl.DisplayBuffer[0], DisplayControl.EraseColor, 0);
                    drawGrid();
                    DisplayControl.showInfoMode = true;
                    printInfo();
                    MeasurementControl.IsRunning = false;
                    drawDataBuffer(&DataBufferControl.DataBuffer[0], COLOR_DATA_HOLD, 0);
                } else {
                    /*
                     * Normal loop clear old chart, start next acquisition and draw new chart
                     */
                    if (!tDrawWhileAcquire) {
                        // Clear old chart
                        //drawDataBuffer(&DataBufferControl.DisplayBuffer[0], DisplayControl.EraseColor, 0);
                    } else {
                        // check if chart is completely drawn
                        while (DataBufferControl.DataBufferNextDrawPointer != DataBufferControl.DataBufferNextInPointer) {
                            DrawOneDataBufferValue();
                        }
                    }
                    // starts next acquisition
                    startAcquisition();
                    if (!tDrawWhileAcquire) {
                        // Output data to screen
                        drawDataBuffer(&DataBufferControl.DataBuffer[0], COLOR_DATA_RUN, DisplayControl.EraseColor);
                    }
                }
            } else {
                if (tDrawWhileAcquire) {
                    // check if new data available, then draw one value/line
                    if (DataBufferControl.DataBufferNextDrawPointer != DataBufferControl.DataBufferNextInPointer) {
                        DrawOneDataBufferValue();
                    }
                }
            }
#if defined(DEBUG)
//            printDebugData();
#endif
            if (MeasurementControl.isSingleShotMode && MeasurementControl.searchForTrigger
                    && sMillisSinceLastDemoOutput > MILLIS_BETWEEN_INFO_OUTPUT) {
                /*
                 * Single shot mode here
                 * output actual values every second - abuse the average value ;-)
                 * and check touch
                 */
                MeasurementControl.ValueAverage = MeasurementControl.ValueBeforeTrigger;
                TouchPanel.readData(); // 22 / 270  Microseconds without/with touch
                sTouchPanelGetPressureValid = true;
            }

            /*
             * check of touch panel at least every MILLIS_BETWEEN_TOUCH_PANEL_CHECK (300)
             */
            if (!sTouchPanelGetPressureValid && MeasurementControl.TimebaseJustDelayed
                    && MillisSinceLastTPCheck > MILLIS_BETWEEN_TOUCH_PANEL_CHECK) { // 300
                // must be protected against interrupts otherwise sometimes (rarely) it may get random values
                TouchPanel.readData();
                sTouchPanelGetPressureValid = true;
                MillisSinceLastTPCheck = 0;
            }
        } else {
            /*
             * Measurement stopped here
             */
            TouchPanel.readData(); // 22 / 270  Microseconds without/with touch
            sTouchPanelGetPressureValid = true;
#ifdef USE_RTC
            if (rtcAvailable && DisplayControl.DisplayMode == DISPLAY_MODE_CHART && DisplayControl.showInfoMode) {
                showRTCTime();
            }
#endif
        }

        /*
         * GUI handling - 28 microseconds if no touch
         */
        if (sTouchPanelGetPressureValid) {
            sTouchPanelGetPressureValid = false;
            handleTouchPanelEvents();
        }

        if (sMillisSinceLastDemoOutput > MILLIS_BETWEEN_INFO_OUTPUT && MeasurementControl.IsRunning) {
            sMillisSinceLastDemoOutput = 0;
            // refresh grid
            drawGridLinesWithHorizLabelsAndTriggerLine();
            if (DisplayControl.showInfoMode) {
                computeMicrosPerPeriod();
                printInfo();
            }
        }
        MeasurementControl.TimebaseJustDelayed = false;
    } // for(;;)
}
/* Main loop end */

void redrawDisplay() {
    LocalDisplay.clearDisplay(COLOR_BACKGROUND_DSO);
    if (MeasurementControl.IsRunning) {
        if (!MeasurementControl.TriggerValuesAutomatic) {
            TouchSliderTriggerLevel.drawBar();
        }
        if (DisplayControl.DisplayMode == DISPLAY_MODE_SHOW_SETTINGS_GUI) {
            drawSettingsGui();
        }
        // refresh grid
        drawGridLinesWithHorizLabelsAndTriggerLine();
        if (DisplayControl.showInfoMode) {
            printInfo();
        }
    } else {
        /*
         * Analyze mode here
         */
        if (DisplayControl.DisplayMode == DISPLAY_MODE_SHOW_MAIN_GUI) {
            drawGui();
        } else if (DisplayControl.DisplayMode == DISPLAY_MODE_CHART) {
            drawGrid();
            drawDataBuffer(&DataBufferControl.DataBuffer[0], COLOR_DATA_HOLD, 0);
            if (DisplayControl.showInfoMode) {
                printInfo();
            }
        } else {
            drawSettingsGui();
        }
    }

}
/************************************************************************
 * Measurement section
 ************************************************************************/

/*
 * prepares all variables for new acquisition
 * switches between fast an interrupt mode depending on TIMEBASE_FAST_MODES
 * sets ADC status register including prescaler
 */
void startAcquisition() {
    if (MeasurementControl.isSingleShotMode) {
        // Start and request immediate stop
        MeasurementControl.StopRequested = true;
        DataBufferControl.AcquisitionSize = DATABUFFER_SIZE;
        DataBufferControl.DataBufferEndPointer = &DataBufferControl.DataBuffer[DATABUFFER_SIZE - 1];
    } else {
        if (MeasurementControl.StopRequested) {
            return;
        }
    }
#if defined(TIMING_DEBUG)
    //setTimingDebug();
#endif
    /*
     * setup new interrupt cycle only if not to be stopped
     */
    DataBufferControl.DataBufferNextInPointer = &DataBufferControl.DataBuffer[0];
    DataBufferControl.DataBufferNextDrawPointer = &DataBufferControl.DataBuffer[0];
    DataBufferControl.DataBufferNextDrawIndex = 0;
// start with waiting for triggering condition
    MeasurementControl.IntegrateValueForAverage = 0;
    MeasurementControl.TriggerSampleCount = 0;
    MeasurementControl.TriggerStatus = TRIGGER_START;
    MeasurementControl.searchForTrigger = true;
    DataBufferControl.DataBufferFull = false;
//	MeasurementControl.IntegrateCount = 0;
    MeasurementControl.TimebaseJustDelayed = false;

    uint8_t tTimebaseIndex = MeasurementControl.TimebaseIndex;
    if (tTimebaseIndex <= TIMEBASE_INDEX_FAST_MODES) {
        MeasurementControl.TimebaseFastFreerunningMode = true;
    } else {
        MeasurementControl.TimebaseFastFreerunningMode = false;
    }

    // get hardware prescale value
    uint8_t tTimebaseHWValue = tTimebaseIndex + PRESCALE_MIN_VALUE;
    if (tTimebaseHWValue > PRESCALE_MAX_VALUE) {
        tTimebaseHWValue = PRESCALE_MAX_VALUE;
    }
#if defined(TIMING_DEBUG)
//   resetTimingDebug();
#endif
    /*
     * Start acquisition in free running mode for trigger detection
     */
    //	ADCSRB = 0; // free running mode  - is default
    if (MeasurementControl.TimebaseFastFreerunningMode) {
        // NO Interrupt in FastMode
        ADCSRA = ((1 << ADEN) | (1 << ADSC) | (1 << ADATE) | (1 << ADIF) | tTimebaseHWValue);
    } else {
        //  enable ADC interrupt, start with free running mode,
        ADCSRA = ((1 << ADEN) | (1 << ADSC) | (1 << ADATE) | (1 << ADIF) | tTimebaseHWValue | (1 << ADIE));
    }
}

/*
 * Fast ADC read routine
 */
void acquireDataFast() {
    /**********************************
     * wait for triggering condition
     **********************************/
    Myword tUValue;
    uint8_t tTriggerStatus = TRIGGER_START;
    uint16_t i;
    uint16_t tValueOffset = MeasurementControl.ValueOffset;

    /*
     * Wait for trigger for max. 10 screens
     */
// start the first conversion and clear bit to recognize next conversion has finished
    ADCSRA |= (1 << ADIF) | (1 << ADSC);
    // if trigger condition not met it runs forever in single shot mode
    for (i = TRIGGER_WAIT_NUMBER_OF_SAMPLES; i != 0 || MeasurementControl.isSingleShotMode; --i) {
        // wait for free running conversion to finish
        while (bit_is_clear(ADCSRA, ADIF)) {
            ;
        }
        // Get value
        tUValue.byte.LowByte = ADCL;
        tUValue.byte.HighByte = ADCH;
        // without "| (1 << ADSC)" it does not work - undocumented feature???
        ADCSRA |= (1 << ADIF) | (1 << ADSC); // clear bit to recognize next conversion has finished

        /*
         * detect trigger slope
         */
        if (MeasurementControl.TriggerSlopeRising) {
            if (tTriggerStatus == TRIGGER_START) {
                // rising slope - wait for value below 1. threshold
                if (tUValue.Word < MeasurementControl.TriggerLevelLower) {
                    tTriggerStatus = TRIGGER_BEFORE_THRESHOLD;
                }
            } else {
                // rising slope - wait for value to rise above 2. threshold
                if (tUValue.Word > MeasurementControl.TriggerLevelUpper) {
                    break;
                }
            }
        } else {
            if (tTriggerStatus == TRIGGER_START) {
                // falling slope - wait for value above 1. threshold
                if (tUValue.Word > MeasurementControl.TriggerLevelUpper) {
                    tTriggerStatus = TRIGGER_BEFORE_THRESHOLD;
                }
            } else {
                // falling slope - wait for value to go below 2. threshold
                if (tUValue.Word < MeasurementControl.TriggerLevelLower) {
                    break;
                }
            }
        }
    }
    MeasurementControl.searchForTrigger = false; // for single shot mode

    /********************************
     * read a buffer of data
     ********************************/
// setup for min, max, average
    uint16_t tValueMax = tUValue.Word;
    uint16_t tValueMin = tUValue.Word;
    uint8_t tIndex = MeasurementControl.TimebaseIndex;
    uint16_t tLoopCount = DataBufferControl.AcquisitionSize;
    uint8_t *DataPointer = &DataBufferControl.DataBuffer[0];
    uint8_t *DataPointerFast = &DataBufferControl.DataBuffer[0];
    uint32_t tIntegrateValue = 0;

    cli();
    if (tIndex <= TIMEBASE_INDEX_ULTRAFAST_MODE) {
        if (DATABUFFER_SIZE / 2 < LOCAL_DISPLAY_WIDTH) {
            // No space for 2 times LOCAL_DISPLAY_WIDTH values
            // I hope the compiler will remove the code ;-)
            tLoopCount = DATABUFFER_SIZE / 2;
        } else if (tLoopCount > LOCAL_DISPLAY_WIDTH) {
            tLoopCount = tLoopCount / 2;
        }
        /*
         * do very fast reading without processing
         */
        for (i = tLoopCount; i != 0; --i) {
            uint8_t tLow, tHigh;
            while (bit_is_clear(ADCSRA, ADIF)) {
                ;
            }
            tLow = ADCL;
            tHigh = ADCH;
            ADCSRA |= (1 << ADIF) | (1 << ADSC);
            *DataPointerFast++ = tLow;
            *DataPointerFast++ = tHigh;
        }
        sei();
        DataPointerFast = &DataBufferControl.DataBuffer[0];
    }
    for (i = tLoopCount; i != 0; --i) {
        if (tIndex <= TIMEBASE_INDEX_ULTRAFAST_MODE) {
            // get values from ultrafast buffer
            tUValue.byte.LowByte = *DataPointerFast++;
            tUValue.byte.HighByte = *DataPointerFast++;
        } else {
            // get values from adc
            // wait for free running conversion to finish
            // ADCSRA here is E5
            while (bit_is_clear(ADCSRA, ADIF)) {
                ;
            }
            // ADCSRA here is F5
            // duration: get Value included min 1,2 micros
            tUValue.byte.LowByte = ADCL;
            tUValue.byte.HighByte = ADCH;
            // without "| (1 << ADSC)" it does not work - undocumented feature???
            ADCSRA |= (1 << ADIF) | (1 << ADSC); // clear bit to recognize next conversion has finished
            //ADCSRA here is E5
        }
        /*
         * process value
         */

        tIntegrateValue += tUValue.Word;
        // get max and min for display and automatic triggering - needs 0,4 microseconds
        if (tUValue.Word > tValueMax) {
            tValueMax = tUValue.Word;
        } else if (tUValue.Word < tValueMin) {
            tValueMin = tUValue.Word;
        }

        /***************************************************
         * transform 10 bit value in order to fit on screen
         ***************************************************/
        if (tUValue.Word < tValueOffset) {
            tUValue.Word = 0;
        } else {
            tUValue.Word -= tValueOffset;
        }
        uint8_t tValueByte = tUValue.Word >> MeasurementControl.ValueShift;
        //clip Value to LOCAL_DISPLAY_HEIGHT
        if (tValueByte > DISPLAY_VALUE_FOR_ZERO) {
            tValueByte = 0;
        } else {
            tValueByte = (DISPLAY_VALUE_FOR_ZERO) - tValueByte;
        }
        // now value is a byte and fits to screen (which is 240 high)
        *DataPointer++ = tValueByte;
    }
// enable interrupt
    sei();
    if (tIndex == 0) {
        // set remaining of buffer to zero
        for (i = tLoopCount; i != 0; --i) {
            *DataPointer++ = 0;
        }
    }
    ADCSRA &= ~(1 << ADATE); // Disable auto-triggering
    MeasurementControl.RawValueMax = tValueMax;
    MeasurementControl.RawValueMin = tValueMin;
    if (tIndex == 0 && tLoopCount > LOCAL_DISPLAY_WIDTH) {
        // compensate for half sample count in last measurement in fast mode
        tIntegrateValue *= 2;
    }
    MeasurementControl.IntegrateValueForAverage = tIntegrateValue;
    DataBufferControl.DataBufferFull = true;
}

/*
 * Interrupt service routine for adc interrupt
 * used only for "slow" mode because ISR overhead is to much for fast mode
 * app. 7 microseconds + 2 for push + 2 for pop
 */

ISR(ADC_vect) {
    if (MeasurementControl.TimebaseDelay == 0) {
        // 3 + 8 Pushes + in + eor =24 cycles
        // + 4+ cycles for jump to isr
        // +  4 for load compare and branch
        // gives 32 cycles = 2 micros
        ADCSRA |= (1 << ADSC);
    }

    Myword tUValue;
    tUValue.byte.LowByte = ADCL;
    tUValue.byte.HighByte = ADCH;

    if (MeasurementControl.searchForTrigger) {
        bool tTriggerFound = false;
        /*
         * Trigger detection here
         */
        uint8_t tTriggerStatus = MeasurementControl.TriggerStatus;

        if (MeasurementControl.TriggerSlopeRising) {
            if (tTriggerStatus == TRIGGER_START) {
                // rising slope - wait for value below 1. threshold
                if (tUValue.Word < MeasurementControl.TriggerLevelLower) {
                    MeasurementControl.TriggerStatus = TRIGGER_BEFORE_THRESHOLD;
                }
            } else {
                // rising slope - wait for value to rise above 2. threshold
                if (tUValue.Word > MeasurementControl.TriggerLevelUpper) {
                    // start reading into buffer
                    tTriggerFound = true;
                }
            }
        } else {
            if (tTriggerStatus == TRIGGER_START) {
                // falling slope - wait for value above 1. threshold
                if (tUValue.Word > MeasurementControl.TriggerLevelUpper) {
                    MeasurementControl.TriggerStatus = TRIGGER_BEFORE_THRESHOLD;
                }
            } else {
                // falling slope - wait for value to go below 2. threshold
                if (tUValue.Word < MeasurementControl.TriggerLevelLower) {
                    // start reading into buffer
                    tTriggerFound = true;
                }
            }
        }

        if (!tTriggerFound) {
            // two consecutive if's save one register push and pop
            if (MeasurementControl.isSingleShotMode) {
                // no timeout for SingleShotMode -> return
                MeasurementControl.ValueBeforeTrigger = tUValue.Word;
                return;
            }
            /*
             * Trigger timeout handling
             */
            MeasurementControl.TriggerSampleCount++;
            if (MeasurementControl.TriggerSampleCount < MeasurementControl.TriggerTimeoutSampleCount) {
                /*
                 * Trigger condition not met and timeout not reached
                 */
                return;
            }
        }
        // Trigger found -> reset trigger flag and initialize max and min
        MeasurementControl.searchForTrigger = false;
        MeasurementControl.ValueMaxForISR = tUValue.Word;
        MeasurementControl.ValueMinForISR = tUValue.Word;
        // stop free running mode
        ADCSRA &= ~(1 << ADATE);

    }
    /*
     * read buffer data
     */
    // take only last value for chart
    bool tUseValue = true;
    if (MeasurementControl.TimebaseDelay != 0) {
        uint16_t tRemaining = MeasurementControl.TimebaseDelayRemaining;
        if (tRemaining > (ADC_CONVERSION_AS_DELAY_MICROS * 4)) {
            // instead of busy wait just start a new conversion, which lasts 112 micros ( 13 + 1 (delay) clock cycles at 8 micros )
            MeasurementControl.TimebaseDelayRemaining -= (ADC_CONVERSION_AS_DELAY_MICROS * 4);
            MeasurementControl.TimebaseJustDelayed = true;
            tUseValue = false;
        } else {
            // busy wait for remaining micros
            // delayMicroseconds() needs 2 registers more (=1/4 microsecond call overhead)
            // here we have 1/4 microseconds resolution
            __asm__ __volatile__ (
                    "1: sbiw %0,1" "\n\t" // 2 cycles
                    "brne 1b" : : "r" (MeasurementControl.TimebaseDelayRemaining)// 2 cycles
            );
            // restore initial value
            MeasurementControl.TimebaseDelayRemaining = MeasurementControl.TimebaseDelay;
        }
        // 5 micros from Interrupt till here
        // start the next conversion since in this mode adc is not free running
        ADCSRA |= (1 << ADSC);
    }

    /*
     * do further processing of raw data
     */
    // get max and min for display and automatic triggering
    if (tUValue.Word > MeasurementControl.ValueMaxForISR) {
        MeasurementControl.ValueMaxForISR = tUValue.Word;
    } else if (tUValue.Word < MeasurementControl.ValueMinForISR) {
        MeasurementControl.ValueMinForISR = tUValue.Word;
    }

    if (tUseValue) {
        /*
         * c code (see line below) needs 5 register more (to push and pop)#
         * so do it with assembler and 1 additional register (for high byte :-()
         */
//	MeasurementControl.IntegrateValueForAverage += tUValue.Word;
        __asm__ (
                /* could use __tmp_reg__ as synonym for r24*/
                /* add low byte */
                "ld	    r24, Z  ; \n"
                "add	r24, %[lowbyte] ; \n"
                "st	    Z+, r24 ; \n"
                /* add high byte */
                "ld	    r24, Z  ; \n"
                "adc	r24, %[highbyte] ; \n"
                "st	    Z+, r24 ; \n"
                /* add carry */
                "ld	    r24, Z  ; \n"
                "adc	r24, __zero_reg__ ; \n"
                "st	    Z+, r24 ; \n"
                /* add carry */
                "ld	    r24, Z  ; \n"
                "adc	r24, __zero_reg__ ; \n"
                "st	    Z+, r24 ; \n"
                :/*no output*/
                : [lowbyte] "r" (tUValue.byte.LowByte), [highbyte] "r" (tUValue.byte.HighByte), "z" (&MeasurementControl.IntegrateValueForAverage)
                : "r24"
        );

// detect end of buffer
        uint8_t *tDataBufferPointer = DataBufferControl.DataBufferNextInPointer;
        if (tDataBufferPointer <= DataBufferControl.DataBufferEndPointer) {
            /***************************************************
             * transform 10 bit value in order to fit on screen
             ***************************************************/
            if (tUValue.Word < MeasurementControl.ValueOffset) {
                tUValue.Word = 0;
            } else {
                tUValue.Word = tUValue.Word - MeasurementControl.ValueOffset;
            }
            uint8_t tValueByte = tUValue.Word >> MeasurementControl.ValueShift;
            //clip Value to LOCAL_DISPLAY_HEIGHT
            if (tValueByte > DISPLAY_VALUE_FOR_ZERO) {
                tValueByte = 0;
            } else {
                tValueByte = (DISPLAY_VALUE_FOR_ZERO) - tValueByte;
            }
            // store display byte value
            *tDataBufferPointer++ = tValueByte;
            // prepare for next
            DataBufferControl.DataBufferNextInPointer = tDataBufferPointer;
        } else {
            // stop acquisition
            ADCSRA &= ~(1 << ADIE); // disable ADC interrupt
            // copy max and min values of measurement for display purposes
            MeasurementControl.RawValueMin = MeasurementControl.ValueMinForISR;
            MeasurementControl.RawValueMax = MeasurementControl.ValueMaxForISR;
            /*
             * signal to main loop that acquisition ended
             * Main loop is responsible to start a new acquisition via call of startAcquisition();
             */
            DataBufferControl.DataBufferFull = true;
        }
    }
}

void setLevelAndHysteresis(int aRawTriggerValue, int aRawTriggerHysteresis) {
    MeasurementControl.RawTriggerLevel = aRawTriggerValue;
    if (MeasurementControl.TriggerSlopeRising) {
        MeasurementControl.TriggerLevelUpper = aRawTriggerValue;
        MeasurementControl.TriggerLevelLower = aRawTriggerValue - aRawTriggerHysteresis;
    } else {
        MeasurementControl.TriggerLevelLower = aRawTriggerValue;
        MeasurementControl.TriggerLevelUpper = aRawTriggerValue + aRawTriggerHysteresis;
    }
}

/*
 * sets hysteresis, shift and offset such that graph are from bottom to DISPLAY_USAGE
 * If old values are reasonable don't change them to avoid jitter
 */
void computeAutoTrigger() {
    if (!MeasurementControl.TriggerValuesAutomatic) {
        return;
    }
    uint16_t tPeakToPeak = MeasurementControl.RawValueMax - MeasurementControl.RawValueMin;
// middle between min and max
    uint16_t tTriggerValue = MeasurementControl.RawValueMin + (tPeakToPeak / 2);

    /*
     * set effective hysteresis to quarter delta and clip to reasonable value
     */
    uint8_t TriggerHysteresis = tPeakToPeak / 4;
    if (TriggerHysteresis < TRIGGER_HYSTERESIS_MIN) {
        TriggerHysteresis = TRIGGER_HYSTERESIS_MIN;
    }
    setLevelAndHysteresis(tTriggerValue, TriggerHysteresis);
}

bool changeInputRange(uint8_t aValue) {
    bool tRetValue = true;
    MeasurementControl.ValueShift = aValue;
    /*
     * Grid is changed if leaving or entering highest resolution with grid size 41,
     * so we better clear old grid and labels before
     */
    clearHorizontalGridLinesAndHorizontalLineLabels();

    uint8_t tFactor = 1 << aValue;
    float tHorizontalGridVoltage;
    uint16_t tHorizontalGridSizeShift8;
    if (MeasurementControl.ADCReference == DEFAULT) {
        /*
         * 5 Volt reference
         */
        if (aValue == 0) {
            tHorizontalGridSizeShift8 = HORIZONTAL_GRID_HEIGHT_5V_02_Range_SHIFT8;
            tHorizontalGridVoltage = 0.2;
        } else {
            tHorizontalGridSizeShift8 = HORIZONTAL_GRID_HEIGHT_5V_SHIFT8;
            if (aValue == 3) {
                // 10 Volt range, since we cannot display values of 4.9 volt in 5 volt range on a 240 pixel display
                aValue = 4;
            }
            tHorizontalGridVoltage = 0.5 * aValue;
        }
    } else {
        /*
         * 1.1 Volt reference
         */
        if (aValue == 3) {
            tHorizontalGridSizeShift8 = HORIZONTAL_GRID_HEIGHT_1_1V_05_Range_SHIFT8;
            tHorizontalGridVoltage = 0.5;
        } else {
            tHorizontalGridSizeShift8 = HORIZONTAL_GRID_HEIGHT_1_1V_SHIFT8;
            tHorizontalGridVoltage = 0.05 * tFactor;
        }
    }
    MeasurementControl.HorizontalGridSizeShift8 = tHorizontalGridSizeShift8;
    MeasurementControl.HorizontalGridVoltage = tHorizontalGridVoltage;
    drawGridLinesWithHorizLabelsAndTriggerLine();
    return tRetValue;

}

void computeAutoRange() {
    if (MeasurementControl.RangeAutomatic) {

        // get relevant peak2peak value
        uint16_t tPeakToPeak = MeasurementControl.RawValueMax;
        if (MeasurementControl.OffsetAutomatic) {
            // Value min(display) is NOT fixed at zero
            tPeakToPeak -= MeasurementControl.RawValueMin;
        }

        /*
         * set automatic range
         */
        uint8_t tOldValueShift = MeasurementControl.ValueShift;
        uint8_t tNewValueShift = 0;
        if (tPeakToPeak > (DISPLAY_USAGE * 4)) {
            tNewValueShift = 3;
        } else if (tPeakToPeak >= DISPLAY_USAGE * 2) {
            tNewValueShift = 2;
        } else if (tPeakToPeak >= DISPLAY_USAGE) {
            tNewValueShift = 1;
        }
        uint32_t tActualMillis = millis();
        if (tOldValueShift != tNewValueShift) {
            /*
             * delay for changing range to higher resolution
             */
            if (tNewValueShift > tOldValueShift) {
                changeInputRange(tNewValueShift);
            } else {
                // wait n-milliseconds before switch to higher resolution (lower index)
                if (tActualMillis - MeasurementControl.TimestampLastRangeChange > SCALE_CHANGE_DELAY_MILLIS) {
                    MeasurementControl.TimestampLastRangeChange = tActualMillis;
                    changeInputRange(tNewValueShift);
                }
            }
        } else {
            // reset "delay"
            MeasurementControl.TimestampLastRangeChange = tActualMillis;
        }
    }
}
/**
 * compute offset based on center value in order display values at center of screen
 */
void computeAutoOffset() {
    if (MeasurementControl.OffsetAutomatic) {
        uint16_t tValueMiddleY = MeasurementControl.RawValueMin
                + ((MeasurementControl.RawValueMax - MeasurementControl.RawValueMin) / 2);

        uint16_t tRawValuePerGrid = MeasurementControl.HorizontalGridSizeShift8 >> (8 - MeasurementControl.ValueShift);
        // take next multiple of HorizontalGridSize which is smaller than tValueMiddleY
        int16_t tNumberOfGridLinesToSkip = (tValueMiddleY / tRawValuePerGrid) - 3;    // adjust to bottom of display (minus 3 lines)
        if (tNumberOfGridLinesToSkip < 0) {
            tNumberOfGridLinesToSkip = 0;
        }
        // avoid jitter by not changing number if it delta is only 1
        if (abs(MeasurementControl.NumberOfGridLinesToSkipForOffset - tNumberOfGridLinesToSkip) > 1) {
            MeasurementControl.ValueOffset = tNumberOfGridLinesToSkip * tRawValuePerGrid;
            MeasurementControl.NumberOfGridLinesToSkipForOffset = tNumberOfGridLinesToSkip;
        }
    }
}
/***********************************************************************
 * GUI initialization and drawing stuff
 ***********************************************************************/
void createGUI() {

    // Button for Singleshot (and settings/back)
    TouchButtonBack_Singleshot.init(BUTTON_WIDTH_3_POS_3, 0, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_GUI_CONTROL,
            ButtonStringSingleshot, TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH | LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE, 0,
            &doTriggerSingleshot);

// big start stop button
    TouchButtonStartStop.init(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_3, 2 * BUTTON_HEIGHT_4 + BUTTON_SPACING,
    COLOR_GUI_CONTROL, PSTR("Start\nStop"), TEXT_SIZE_22,
    LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE, 0, &doStartStop);

    // Button for settings page
    TouchButtonSettings.init(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_GUI_CONTROL,
            PSTR("Settings"), TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH | LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE, 0,
            &doSettings);

    // Buttons for timebase +/- -NO button beep. Beep is done by handler.
    TouchButtonRight.init(BUTTON_WIDTH_5_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_5, BUTTON_HEIGHT_4,
    COLOR_GUI_SOURCE_TIMEBASE, ">", TEXT_SIZE_22, FLAG_BUTTON_NO_BEEP_ON_TOUCH, 1, &doTimeBaseDisplayScroll);

    TouchButtonLeft.init(0, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_5, BUTTON_HEIGHT_4, COLOR_GUI_SOURCE_TIMEBASE, "<",
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, -1, &doTimeBaseDisplayScroll);
//    TouchButtonLeft.setButtonAutorepeatTiming(500, 500, 2, 250);

    /*
     * Display + History
     */
// Button for switching display mode
    TouchButtonDisplayMode.init(0, 0, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_GUI_DISPLAY_CONTROL, PSTR("Display\nmode"),
    TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH | LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE, 0, &doDisplayMode);

// Button for chart history (erase color)
    TouchButtonChartHistory.init(BUTTON_WIDTH_5_POS_3, BUTTON_HEIGHT_4_LINE_4,
    LOCAL_DISPLAY_WIDTH - (BUTTON_WIDTH_3_POS_2) - (BUTTON_WIDTH_5_POS_3), BUTTON_HEIGHT_4, COLOR_GUI_DISPLAY_CONTROL,
            ChartHistoryButtonStrings[0], TEXT_SIZE_11,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH | LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE, 0, &doChartHistory);

    /*
     * Settings Page
     * 2. column
     */
    // Button for auto trigger on off
    TouchButtonAutoTriggerOnOff.init(BUTTON_WIDTH_3_POS_2, 0, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR_GUI_TRIGGER,
            AutoTriggerButtonStringAuto, TEXT_SIZE_11,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH | LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE, 0, &doTriggerAutoOnOff);

    // Button for slope
    TouchButtonSlope.init(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
    COLOR_GUI_TRIGGER, SlopeButtonStringAscending, TEXT_SIZE_11,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH | LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE, 0, &doTriggerSlope);

    // Button for range
    TouchButtonAutoRangeOnOff.init(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_3, BUTTON_WIDTH_3,
    BUTTON_HEIGHT_4, COLOR_GUI_TRIGGER, AutoRangeButtonStringAuto, TEXT_SIZE_11,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH | LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE, 0, &doRangeMode);

    // Button for auto offset on off
    TouchButtonAutoOffsetOnOff.init(BUTTON_WIDTH_3_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
    COLOR_GUI_TRIGGER, AutoOffsetButtonString0, TEXT_SIZE_11,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH | LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE, 0, &doAutoOffsetOnOff);

    /*
     * 3. column
     */
// Button for channel select
    TouchButtonChannelSelect.init(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
    COLOR_GUI_SOURCE_TIMEBASE, ChannelSelectButtonString, TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH,
            MeasurementControl.ADCInputMUXChannel, &doChannelSelect);

// Button for reference voltage switching
    TouchButtonADCReference.init(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4,
    COLOR_GUI_SOURCE_TIMEBASE, ReferenceButton5_0V, TEXT_SIZE_11,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH | LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE, 0, &doADCReference);

    /*
     * SLIDER
     */
    TouchSliderBacklight.init(40, 60, 16, BACKLIGHT_MAX_BRIGHTNESS_VALUE, BACKLIGHT_MAX_BRIGHTNESS_VALUE,
    BACKLIGHT_START_BRIGHTNESS_VALUE, COLOR16_BLUE, COLOR16_GREEN, FLAG_SLIDER_SHOW_BORDER | FLAG_SLIDER_SHOW_VALUE, &doBacklight);
    TouchSliderBacklight.setCaption("Backlight");

    // slightly visible slider for voltage picker
    TouchSliderVoltagePicker.init(SLIDER_VPICKER_VALUE_X - TEXT_SIZE_11_WIDTH + 4, 0, 24, DISPLAY_VALUE_FOR_ZERO,
    LOCAL_DISPLAY_HEIGHT - 1, 0, COLOR16_WHITE, COLOR_VOLTAGE_PICKER_SLIDER, FLAG_SLIDER_VALUE_BY_CALLBACK, &doVoltagePicker);
    TouchSliderVoltagePicker.setBarBackgroundColor(COLOR_VOLTAGE_PICKER_SLIDER);

    // slightly visible slider for trigger level
    TouchSliderTriggerLevel.init(SLIDER_TLEVEL_VALUE_X - 4, 0, 24, DISPLAY_VALUE_FOR_ZERO, LOCAL_DISPLAY_HEIGHT - 1, 0,
    COLOR16_WHITE, COLOR_TRIGGER_SLIDER, FLAG_SLIDER_VALUE_BY_CALLBACK, &doTriggerLevel);
    TouchSliderTriggerLevel.setBarBackgroundColor(COLOR_TRIGGER_SLIDER);

}

void drawGui() {
    DisplayControl.DisplayMode = DISPLAY_MODE_SHOW_MAIN_GUI;
    LocalDisplay.clearDisplay(COLOR_BACKGROUND_DSO);
    LocalTouchButton::deactivateAll();
    LocalTouchSlider::deactivateAll();

    TouchButtonStartStop.drawButton();
    TouchButtonBack_Singleshot.drawButton();
    TouchButtonSettings.drawButton();
    TouchButtonDisplayMode.drawButton();
    TouchButtonChartHistory.drawButton();
    TouchButtonLeft.drawButton();
    TouchButtonRight.drawButton();

    // this test the MLText function :-), but cost 380 bytes :-(
    LocalDisplay.drawMLText(10, BUTTON_HEIGHT_4_LINE_2 + 34, F("Welcome to\nArduino DSO"), TEXT_SIZE_22, COLOR16_BLUE, COLOR16_NO_BACKGROUND);

    // Just show the buttons, do not activate them
    TouchButtonLeft.deactivate();
    TouchButtonRight.deactivate();
}

void drawSettingsGui() {

    LocalTouchButton::deactivateAll();
    LocalTouchSlider::deactivateAll();

    // here the back button
    TouchButtonBack_Singleshot.drawButton();
    TouchButtonAutoTriggerOnOff.drawButton();
    TouchButtonSlope.drawButton();
    TouchButtonAutoRangeOnOff.drawButton();
    TouchButtonAutoOffsetOnOff.drawButton();
    TouchButtonChannelSelect.drawButton();
    TouchButtonADCReference.drawButton();
    TouchSliderBacklight.drawSlider();
}

/*
 * activate elements if returning from settings screen or if starting acquisition
 */
void activatePartOfGui() {
// first deactivate all buttons
    LocalTouchButton::deactivateAll();

    TouchButtonStartStop.activate();
    TouchButtonBack_Singleshot.activate();
    TouchButtonSettings.activate();
    TouchButtonDisplayMode.activate();
    TouchButtonChartHistory.activate();
    TouchButtonLeft.activate();
    TouchButtonRight.activate();

    TouchSliderVoltagePicker.drawBar();
    TouchSliderVoltagePicker.activate();
    if (!MeasurementControl.TriggerValuesAutomatic) {
        TouchSliderTriggerLevel.drawBar();
        TouchSliderTriggerLevel.activate();
    }
}

/************************************************************************
 * GUI handler section
 ************************************************************************/
/*
 * running -> Changes timebase
 * stopped -> Horizontal scrolls through data buffer
 */
void doTimeBaseDisplayScroll(LocalTouchButton *const aTheTouchedButton __attribute__((unused)), int16_t aValue) {
    bool tErrorBeep = false;
    if (MeasurementControl.IsRunning) {
        /*
         * set timebase here - value 1 means increment timebase index
         */

        uint8_t tOldIndex = MeasurementControl.TimebaseIndex;

        // positive value means increment timebase index!
        int8_t tNewIndex = tOldIndex + aValue;
        if (tNewIndex < 0) {
            tNewIndex = 0;
            tErrorBeep = true;
        } else if (tNewIndex > TIMEBASE_NUMBER_OF_ENTRIES - 1) {
            tNewIndex = TIMEBASE_NUMBER_OF_ENTRIES - 1;
            tErrorBeep = true;
        } else {
            bool tStartNewAcquisition = false;

            if (tOldIndex >= TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE && tNewIndex < TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE) {
                // from draw while acquire to normal mode -> stop acquisition, clear old chart, and start a new one
                ADCSRA &= ~(1 << ADIE); // stop acquisition - disable ADC interrupt
                tStartNewAcquisition = true;
            }

            if (tOldIndex < TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE && tNewIndex >= TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE) {
                // from normal to draw while acquire mode
                ADCSRA &= ~(1 << ADIE); // stop acquisition - disable ADC interrupt
                tStartNewAcquisition = true;
            }
            // delay handling - programmatic delays between adc conversions
            MeasurementControl.TimebaseIndex += aValue;
            MeasurementControl.TimebaseDelay = TimebaseDelayValues[MeasurementControl.TimebaseIndex];
            MeasurementControl.TimebaseDelayRemaining = TimebaseDelayValues[MeasurementControl.TimebaseIndex];
            // use 2 * TimebaseDivExactValues as trigger timeout
            uint32_t tTriggerTimeoutSampleCount32 = pgm_read_dword(&TimebaseDivExactValues[MeasurementControl.TimebaseIndex]);
            uint16_t tTriggerTimeoutSampleCount = tTriggerTimeoutSampleCount32;
            if (tTriggerTimeoutSampleCount32 < TRIGGER_WAIT_NUMBER_OF_SAMPLES) {
                tTriggerTimeoutSampleCount = TRIGGER_WAIT_NUMBER_OF_SAMPLES;
            }
            if (tTriggerTimeoutSampleCount < 0x8000) {
                tTriggerTimeoutSampleCount *= 2;
            } else {
                tTriggerTimeoutSampleCount = 0xFFFF;
            }
            MeasurementControl.TriggerTimeoutSampleCount = tTriggerTimeoutSampleCount;

            printInfo();
            if (tStartNewAcquisition) {
                startAcquisition();
            }
        }
    } else if (DisplayControl.DisplayMode != DISPLAY_MODE_SHOW_MAIN_GUI) {
        /*
         * set start of display in data buffer
         */
        DataBufferControl.DataBufferDisplayStart += aValue * DATABUFFER_DISPLAY_INCREMENT;
        if (DataBufferControl.DataBufferDisplayStart < &DataBufferControl.DataBuffer[0]) {
            DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[0];
            tErrorBeep = true;
        } else if (DataBufferControl.DataBufferDisplayStart
                > &DataBufferControl.DataBuffer[DATABUFFER_SIZE - LOCAL_DISPLAY_WIDTH]) {
            DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[DATABUFFER_SIZE - LOCAL_DISPLAY_WIDTH];
            tErrorBeep = true;
        }
        // delete old graph
        drawDataBuffer(&DataBufferControl.DisplayBuffer[0], DisplayControl.EraseColor, 0);
        // draw new one
        drawDataBuffer(DataBufferControl.DataBufferDisplayStart, COLOR_DATA_HOLD, 0);
    }
    FeedbackTone(!tErrorBeep);

}

/*
 * toggle between automatic and manual trigger voltage value
 */
void doTriggerAutoOnOff(LocalTouchButton *const aTheTouchedButton __attribute__((unused)), int16_t aValue __attribute__((unused))) {
    MeasurementControl.TriggerValuesAutomatic = (!MeasurementControl.TriggerValuesAutomatic);
    if (MeasurementControl.TriggerValuesAutomatic) {
        TouchButtonAutoTriggerOnOff.setCaption((const __FlashStringHelper *)AutoTriggerButtonStringAuto);
    } else {
        TouchButtonAutoTriggerOnOff.setCaption((const __FlashStringHelper *)AutoTriggerButtonStringManual);
    }
    TouchButtonAutoTriggerOnOff.drawButton();
}

/*
 * toggle between ascending and descending trigger slope
 */
void doTriggerSlope(LocalTouchButton *const aTheTouchedButton __attribute__((unused)), int16_t aValue __attribute__((unused))) {
    MeasurementControl.TriggerSlopeRising = (!MeasurementControl.TriggerSlopeRising);
    if (MeasurementControl.TriggerSlopeRising) {
        TouchButtonSlope.setCaption((const __FlashStringHelper *)SlopeButtonStringAscending);
    } else {
        TouchButtonSlope.setCaption((const __FlashStringHelper *)SlopeButtonStringDecending);
    }
    TouchButtonSlope.drawButton();
}

/*
 * toggle between auto and 0 Volt offset
 */
void doAutoOffsetOnOff(LocalTouchButton *const aTheTouchedButton __attribute__((unused)), int16_t aValue __attribute__((unused))) {
    MeasurementControl.OffsetAutomatic = !MeasurementControl.OffsetAutomatic;
    if (MeasurementControl.OffsetAutomatic) {
        TouchButtonAutoOffsetOnOff.setCaption((const __FlashStringHelper *)AutoOffsetButtonStringAuto);
    } else {
        TouchButtonAutoOffsetOnOff.setCaption((const __FlashStringHelper *)AutoOffsetButtonString0);
        // disable auto offset
        MeasurementControl.ValueOffset = 0;
    }
    TouchButtonAutoOffsetOnOff.drawButton();
}

void doRangeMode(LocalTouchButton *const aTheTouchedButton __attribute__((unused)), int16_t aValue __attribute__((unused))) {
    MeasurementControl.RangeAutomatic = !MeasurementControl.RangeAutomatic;
    if (MeasurementControl.RangeAutomatic) {
        TouchButtonAutoRangeOnOff.setCaption((const __FlashStringHelper *)AutoRangeButtonStringAuto);
    } else {
        TouchButtonAutoRangeOnOff.setCaption((const __FlashStringHelper *)AutoRangeButtonStringManual);
    }
    TouchButtonAutoRangeOnOff.drawButton();
}

/*
 * toggle between 5 and 1.1 Volt reference
 */
void doADCReference(LocalTouchButton *const aTheTouchedButton __attribute__((unused)), int16_t aValue __attribute__((unused))) {
    uint8_t tReference = MeasurementControl.ADCReference;
    if (tReference == DEFAULT) {
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__)
        tReference = INTERNAL1V1;
#else
        tReference = INTERNAL;
#endif
        TouchButtonADCReference.setCaption((const __FlashStringHelper *)ReferenceButton1_1V);
    } else {
        tReference = DEFAULT;
        TouchButtonADCReference.setCaption((const __FlashStringHelper *)ReferenceButton5_0V);
    }
    setReference(tReference);
    TouchButtonADCReference.drawButton();
}

/*
 * Cycle through all external and internal adc channels
 */
void doChannelSelect(LocalTouchButton *const aTheTouchedButton __attribute__((unused)), int16_t aValue __attribute__((unused))) {
    uint8_t tChannel = MeasurementControl.ADCInputMUXChannel;
    tChannel++;
    if (tChannel <= MAX_ADC_CHANNEL) {
        // Standard AD channels
        MeasurementControl.ADCInputMUXChannelChar = '0' + tChannel;
    } else if (tChannel == MAX_ADC_CHANNEL + 1) {
        tChannel = 8; // Temperature
        MeasurementControl.ADCInputMUXChannelChar = 'T';
    } else if (tChannel == 9) {
        tChannel = 14; // 1.1 Reference
        MeasurementControl.ADCInputMUXChannelChar = 'R';
    } else if (tChannel == 15) {
        MeasurementControl.ADCInputMUXChannelChar = 'G'; // Ground
    } else if (tChannel == 16) {
        // Channel 0
        tChannel = 0;
        MeasurementControl.ADCInputMUXChannelChar = 0x30;
    }
    setMUX(tChannel); // this setsADCInputMUXChannel
// set channel number in caption
    ChannelSelectButtonString[CHANNEL_STRING_INDEX] = MeasurementControl.ADCInputMUXChannelChar;
    TouchButtonChannelSelect.setCaptionAndDraw(ChannelSelectButtonString);
}

/*
 * Button handler for "display" button
 * If running switch between upper info line on/off
 * If stopped switch between:
 * 1. Data chart + voltage picker + info line
 * 2. Display of main GUI
 * 3. Data chart + voltage picker - no info line
 */
void doDisplayMode(LocalTouchButton *const aTheTouchedButton __attribute__((unused)), int16_t aValue __attribute__((unused))) {
    if (MeasurementControl.IsRunning) {
        // Toggle show info mode
        DisplayControl.showInfoMode = !DisplayControl.showInfoMode;
        if (DisplayControl.showInfoMode) {
            printInfo();
        } else {
            // Erase former info line
            LocalDisplay.fillRectRel(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN, LOCAL_DISPLAY_WIDTH - INFO_LEFT_MARGIN,
                    (2 * TEXT_SIZE_11) + INFO_LINE_SPACING + 1, COLOR_BACKGROUND_DSO);
            // draw grid
            drawGridLinesWithHorizLabelsAndTriggerLine();
        }
    } else {
        /*
         * Stopped here, analyze mode
         * switch between:
         * 1. Data chart + voltage picker + info line
         * 2. Display of main GUI
         * 3. Data chart + voltage picker - no info line
         */
        if (DisplayControl.DisplayMode == DISPLAY_MODE_SHOW_MAIN_GUI) {
            // only chart (and voltage picker)
            DisplayControl.DisplayMode = DISPLAY_MODE_CHART;

            // flag to avoid initial clearing of (not existent) picker and trigger level lines
            sLastVoltagePickerValue = 0xFF;
            // enable left and right buttons and draw slider border
            activatePartOfGui();
            redrawDisplay();
        } else if (!DisplayControl.showInfoMode) {
            // from only chart to chart + info
            DisplayControl.showInfoMode = true;
            printInfo();
        } else {
            // clear screen and show only gui
            DisplayControl.DisplayMode = DISPLAY_MODE_SHOW_MAIN_GUI;
            DisplayControl.showInfoMode = false;
            redrawDisplay();
        }
    }
}

/*
 * show gui of settings screen
 */
void doSettings(LocalTouchButton *const aTheTouchedButton __attribute__((unused)), int16_t aValue __attribute__((unused))) {
    // convert singleshot to back button - no color change needed :-)
    TouchButtonBack_Singleshot.setCaption((const __FlashStringHelper *)ButtonStringBack);
    if (!MeasurementControl.IsRunning) {
        LocalDisplay.clearDisplay(COLOR_BACKGROUND_DSO);
    }
    DisplayControl.DisplayMode = DISPLAY_MODE_SHOW_SETTINGS_GUI;
    drawSettingsGui();
}

/*
 * set to singleshot mode and draw an indicating "S"
 */
void doTriggerSingleshot(LocalTouchButton *const aTheTouchedButton __attribute__((unused)), int16_t aValue __attribute__((unused))) {
    if (DisplayControl.DisplayMode == DISPLAY_MODE_SHOW_SETTINGS_GUI) {
        /*
         * Here, this button is the back button of the settings page!
         */
        TouchSliderBacklight.deactivate();
        TouchButtonBack_Singleshot.setCaption((const __FlashStringHelper *)ButtonStringSingleshot);
        if (MeasurementControl.IsRunning) {
            DisplayControl.DisplayMode = DISPLAY_MODE_CHART;
            LocalDisplay.clearDisplay(COLOR_BACKGROUND_DSO);
            drawGrid();
            if (DisplayControl.showInfoMode) {
                printInfo();
            }
            activatePartOfGui();
        } else {
            drawGui();
        }
    } else {
        // single shot button handling
        MeasurementControl.isSingleShotMode = true;
        LocalDisplay.clearDisplay(COLOR_BACKGROUND_DSO);
        drawGridLinesWithHorizLabelsAndTriggerLine();
        // draw an S to indicate running single shot trigger
        LocalDisplay.drawChar(INFO_LEFT_MARGIN + SINGLESHOT_PPRINT_VALUE_X, INFO_UPPER_MARGIN + TEXT_SIZE_11_HEIGHT, 'S',
        TEXT_SIZE_11, COLOR16_BLACK, COLOR_INFO_BACKGROUND);

        // prepare info output - at least 1 sec later
        sMillisSinceLastDemoOutput = 0;
        MeasurementControl.RawValueMax = 0;
        MeasurementControl.RawValueMin = 0;

        // Start a new single shot
        MeasurementControl.IsRunning = true;
        startAcquisition();
    }
}

void doStartStop(LocalTouchButton *const aTheTouchedButton __attribute__((unused)), int16_t aValue __attribute__((unused))) {
    if (MeasurementControl.IsRunning) {
        /*
         * Stop here
         * for the last measurement read full buffer size
         * Do this asynchronously to the interrupt routine in order to extend a running or started acquisition
         * stop single shot mode
         */
        DataBufferControl.DataBufferEndPointer = &DataBufferControl.DataBuffer[DATABUFFER_SIZE - 1];
        if (DataBufferControl.DataBufferFull) {
            // start a new acquisition
            startAcquisition();
        }
        // in SingleShotMode Stop is always requested
        if (MeasurementControl.StopRequested) {
            // for stop requested 2 times -> stop immediately
            uint8_t *tEndPointer = DataBufferControl.DataBufferNextInPointer;
            DataBufferControl.DataBufferEndPointer = tEndPointer;
            // clear trailing buffer space not used
            memset(tEndPointer, 0, ((uint8_t*) &DataBufferControl.DataBuffer[DATABUFFER_SIZE]) - ((uint8_t*) tEndPointer));
        }
        MeasurementControl.StopRequested = true;
        MeasurementControl.isSingleShotMode = false;
        // AcquisitionSize is used in synchronous fast loop so we can set it here
        DataBufferControl.AcquisitionSize = DATABUFFER_SIZE;
        DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[0];
        // no feedback tone, it kills the timing!
    } else {
        // Start here
        FeedbackToneOK();
        LocalDisplay.clearDisplay(COLOR_BACKGROUND_DSO);
        activatePartOfGui();
        DisplayControl.showInfoMode = true;
        DisplayControl.DisplayMode = DISPLAY_MODE_CHART;
        drawGridLinesWithHorizLabelsAndTriggerLine();
        DataBufferControl.DataBufferEndPointer = &DataBufferControl.DataBuffer[LOCAL_DISPLAY_WIDTH - 1];
        DataBufferControl.AcquisitionSize = LOCAL_DISPLAY_WIDTH;

        MeasurementControl.isSingleShotMode = false;
        MeasurementControl.IsRunning = true;
        startAcquisition();
    }
}

/*
 * Cycle through 3 history modes ( => use 3 different erase colors)
 */
void doChartHistory(LocalTouchButton *const aTheTouchedButton __attribute__((unused)), int16_t aValue __attribute__((unused))) {
    DisplayControl.EraseColorIndex++;
    if (DisplayControl.EraseColorIndex > 2) {
        DisplayControl.EraseColorIndex = 0;
    }
    TouchButtonChartHistory.setCaption((const __FlashStringHelper *)ChartHistoryButtonStrings[DisplayControl.EraseColorIndex]);
    DisplayControl.EraseColor = EraseColors[DisplayControl.EraseColorIndex];
    if (DisplayControl.DisplayMode == DISPLAY_MODE_SHOW_MAIN_GUI) {
        // show new caption
        TouchButtonChartHistory.drawButton();
    }
}

void doBacklight(LocalTouchSlider *const aTheTouchedSlider __attribute__((unused)), uint16_t aBrightness) {
    if (aBrightness < BACKLIGHT_MIN_BRIGHTNESS_VALUE) {
        aBrightness = BACKLIGHT_MIN_BRIGHTNESS_VALUE;
    }
    LocalDisplay.setBacklightBrightness(aBrightness);
}

/*
 * The value printed has a resolution of 0,00488 * scale factor
 */
void doTriggerLevel(LocalTouchSlider *const aTheTouchedSlider __attribute__((unused)), uint16_t aValue) {
// in auto-trigger mode show only computed value and do not modify it
    if (MeasurementControl.TriggerValuesAutomatic) {
        // already shown :-)
        return;
    }
// to get display value take DISPLAY_VALUE_FOR_ZERO - aValue and vice versa
    aValue = DISPLAY_VALUE_FOR_ZERO - aValue;
    if (DisplayControl.TriggerLevelDisplayValue == aValue) {
        return;
    }

// clear old trigger line
    clearTriggerLine(DisplayControl.TriggerLevelDisplayValue);
// store actual display value
    DisplayControl.TriggerLevelDisplayValue = aValue;

    if (!MeasurementControl.TriggerValuesAutomatic) {
        // modify trigger values
        uint16_t tLevel = getRawFromDisplayValue(aValue);
        setLevelAndHysteresis(tLevel, TRIGGER_HYSTERESIS_MANUAL);
    }
// draw new line
    drawTriggerLine();
}

void restoreGraphAtYpos(uint16_t tYpos) {
    if (!MeasurementControl.IsRunning) {
        // restore graph
        uint8_t *ScreenBufferPointer = &DataBufferControl.DisplayBuffer[0];
        for (uint16_t i = 0; i < LOCAL_DISPLAY_WIDTH; ++i) {
            uint8_t tValueByte = *ScreenBufferPointer++;
            if (tValueByte == tYpos) {
                LocalDisplay.drawPixel(i, tValueByte, COLOR_DATA_HOLD);
            }
        }
    }
}

/*
 * The value printed has a resolution of 0,00488 * scale factor
 */
void doVoltagePicker(LocalTouchSlider *aTheTouchedSlider __attribute__((unused)), uint16_t aValue) {
    char tVoltageBuffer[6];
    if (sLastVoltagePickerValue == aValue) {
        return;
    }
    if (sLastVoltagePickerValue != 0xFF) {
        // clear old line
        uint16_t tYpos = DISPLAY_VALUE_FOR_ZERO - sLastVoltagePickerValue;
        LocalDisplay.drawLineRel(0, tYpos, LOCAL_DISPLAY_WIDTH, 0, COLOR_BACKGROUND_DSO);
        // restore grid at old y position
        for (uint16_t tXPos = TIMING_GRID_WIDTH - 1; tXPos < LOCAL_DISPLAY_WIDTH - 1; tXPos += TIMING_GRID_WIDTH) {
            LocalDisplay.drawPixel(tXPos, tYpos, COLOR_TIMING_LINES);
        }
        restoreGraphAtYpos(tYpos);
    }
// draw new line
    uint8_t tValue = DISPLAY_VALUE_FOR_ZERO - aValue;
    LocalDisplay.drawLineRel(0, tValue, LOCAL_DISPLAY_WIDTH, 0, COLOR_VOLTAGE_PICKER);
    sLastVoltagePickerValue = aValue;

    float tVoltage = getFloatFromDisplayValue(tValue);
    dtostrf(tVoltage, 4, 2, tVoltageBuffer);
    tVoltageBuffer[4] = 'V';
    tVoltageBuffer[5] = '\0';
    LocalDisplay.drawText(INFO_LEFT_MARGIN + SLIDER_VPICKER_VALUE_X, INFO_UPPER_MARGIN + 13, tVoltageBuffer, 11, COLOR16_BLACK,
    COLOR_INFO_BACKGROUND);
}

/************************************************************************
 * Graphical output section
 ************************************************************************/

void clearTriggerLine(uint8_t aTriggerLevelDisplayValue) {
    LocalDisplay.drawLineRel(0, aTriggerLevelDisplayValue, LOCAL_DISPLAY_WIDTH, 0, COLOR_BACKGROUND_DSO);
    if (DisplayControl.showInfoMode) {
        // restore grid at old y position
        for (uint16_t tXPos = TIMING_GRID_WIDTH - 1; tXPos < LOCAL_DISPLAY_WIDTH - 1; tXPos += TIMING_GRID_WIDTH) {
            LocalDisplay.drawPixel(tXPos, aTriggerLevelDisplayValue, COLOR_TIMING_LINES);
        }
    }
    restoreGraphAtYpos(aTriggerLevelDisplayValue);
}

void drawTriggerLine() {
    uint8_t tValue = DisplayControl.TriggerLevelDisplayValue;
    LocalDisplay.drawLineRel(0, tValue, LOCAL_DISPLAY_WIDTH, 0, COLOR_TRIGGER_LINE);
}

#define HORIZONTAL_LINE_LABELS_CAPION_X (TEXT_SIZE_11_WIDTH * 36)

void clearHorizontalGridLinesAndHorizontalLineLabels() {
    int8_t tCaptionOffset = TEXT_SIZE_11_HEIGHT;
// (DISPLAY_VALUE_FOR_ZERO) * 0x100) + 0x80 for better rounding
    for (int32_t tYPosLoop = DISPLAY_VALUE_FOR_ZERO_SHIFT8; tYPosLoop > 0; tYPosLoop -=
            MeasurementControl.HorizontalGridSizeShift8) {
        uint16_t tYPos = tYPosLoop / 0x100;
// clear line
        LocalDisplay.drawLineRel(0, tYPos, LOCAL_DISPLAY_WIDTH, 0, COLOR_BACKGROUND_DSO);
// clear label
        LocalDisplay.fillRectRel(HORIZONTAL_LINE_LABELS_CAPION_X, tYPos - tCaptionOffset, (4 * TEXT_SIZE_11_WIDTH) - 1,
        TEXT_SIZE_11_HEIGHT, COLOR_BACKGROUND_DSO);
        tCaptionOffset = TEXT_SIZE_11_HEIGHT / 2;
    }
}

/*
 * draws vertical timing + horizontal reference voltage lines
 */
void drawGridLinesWithHorizLabelsAndTriggerLine() {
// vertical (timing) lines
    for (uint16_t tXPos = TIMING_GRID_WIDTH - 1; tXPos < LOCAL_DISPLAY_WIDTH; tXPos += TIMING_GRID_WIDTH) {
        LocalDisplay.drawLineRel(tXPos, 0, 0, LOCAL_DISPLAY_HEIGHT, COLOR_TIMING_LINES);
    }

    /*
     * drawHorizontalLineLabels starting with the line for zero
     */
    float tActualVoltage = 0;
    if (MeasurementControl.OffsetAutomatic) {
        tActualVoltage = MeasurementControl.HorizontalGridVoltage * MeasurementControl.NumberOfGridLinesToSkipForOffset;
    }
    char tStringBuffer[5];
    // Draw first caption just above the zero line
    int8_t tCaptionOffset = TEXT_SIZE_11_HEIGHT;
    for (int32_t tYPosLoop = DISPLAY_VALUE_FOR_ZERO_SHIFT8; tYPosLoop > 0; tYPosLoop -=
            MeasurementControl.HorizontalGridSizeShift8) {
        uint16_t tYPos = tYPosLoop / 0x100;

        // Draw horizontal line
        LocalDisplay.drawLineRel(0, tYPos, LOCAL_DISPLAY_WIDTH, 0, COLOR_TIMING_LINES);
        dtostrf(tActualVoltage, 4, 2, tStringBuffer);

        // Draw label
        LocalDisplay.drawText(HORIZONTAL_LINE_LABELS_CAPION_X, tYPos - tCaptionOffset, tStringBuffer, 11, COLOR_HOR_REF_LINE_LABEL,
        COLOR16_NO_BACKGROUND);
        // draw next label on the line
        tCaptionOffset = TEXT_SIZE_11_ASCEND / 2;
        tActualVoltage += MeasurementControl.HorizontalGridVoltage;
    }
    drawTriggerLine();
}
/*
 * draws min, max and timing lines
 */
void drawGrid() {
// draw max line
    uint8_t tValueDisplay = getDisplayFromRawValue(MeasurementControl.RawValueMax);
    if (tValueDisplay != 0) {
        LocalDisplay.drawLineRel(0, tValueDisplay, LOCAL_DISPLAY_WIDTH, 0, COLOR_MAX_MIN_LINE);
    }
// min line
    tValueDisplay = getDisplayFromRawValue(MeasurementControl.RawValueMin);
    if (tValueDisplay != DISPLAY_VALUE_FOR_ZERO) {
        LocalDisplay.drawLineRel(0, tValueDisplay, LOCAL_DISPLAY_WIDTH, 0, COLOR_MAX_MIN_LINE);
    }
// draw timing lines
    drawGridLinesWithHorizLabelsAndTriggerLine();
}

/*
 * Draws on screen - 9ms for drawPixelfast
 * @param aColor                - If is WHITE then chart is removed :-).
 * @param  DataBufferPointer    - Data is taken from here and written to DisplayBuffer.
 */
void drawDataBuffer(uint8_t *aDataBufferPointer, color16_t aColor, color16_t aClearBeforeColor) {
    uint16_t i;
    uint8_t *DisplayBufferWritePointer = &DataBufferControl.DisplayBuffer[0];
    uint8_t *DisplayBufferReadPointer = &DataBufferControl.DisplayBuffer[0];

    uint8_t tLastValueClear;
    uint8_t tValueClear;
    if (aClearBeforeColor > 0) {
        /*
         * Erase old line for x=0 on display with aClearBeforeColor in advance
         * before writing the first new line. Use values from DisplayBuffer for this purpose.
         */
        tLastValueClear = *DisplayBufferReadPointer++;
        tValueClear = *DisplayBufferReadPointer++;
        LocalDisplay.drawLineFastOneX(0, tLastValueClear, tValueClear, aClearBeforeColor);
        tLastValueClear = tValueClear;
    }

    uint8_t tLastDataValue = *aDataBufferPointer++;
    *DisplayBufferWritePointer++ = tLastDataValue;
    for (i = 0; i < LOCAL_DISPLAY_WIDTH - 1; ++i) {
        if (aClearBeforeColor > 0 && i != LOCAL_DISPLAY_WIDTH - 1) {
            // erase one x value in advance in order not to overwrite the x+1 part of line just drawn before
            tValueClear = *DisplayBufferReadPointer++; // Get new value for clearing from DisplayBuffer
            // tLastValueClear is known to be initialized, because we run it only if aClearBeforeColor > 0 and then it was written before
            LocalDisplay.drawLineFastOneX(i + 1, tLastValueClear, tValueClear, aClearBeforeColor);
            tLastValueClear = tValueClear;
        }
        // draw line
        uint8_t tNewDataValue = *aDataBufferPointer++;
        *DisplayBufferWritePointer++ = tNewDataValue; // write processed value to DisplayBuffer
        /*
         * Draw the new value/line to display :-)
         */
        LocalDisplay.drawLineFastOneX(i, tLastDataValue, tNewDataValue, aColor);
        tLastDataValue = tNewDataValue;
    }
}

/*
 * Draws only one chart value - used for drawing while sampling
 */
void DrawOneDataBufferValue() {
    uint16_t tBufferIndex = DataBufferControl.DataBufferNextDrawIndex;
// check if end of display buffer reached
// needed for last acquisition which uses the whole databuffer
    if (tBufferIndex < LOCAL_DISPLAY_WIDTH) {
        uint8_t *tDisplayBufferPointer = &DataBufferControl.DisplayBuffer[tBufferIndex];
        uint8_t tLastValueByte;
        uint8_t tNextValueByte;
        uint8_t tValueByte = *tDisplayBufferPointer;

        /*
         * clear old line with EraseColor which enables the optional history effect
         */
        if (tBufferIndex < LOCAL_DISPLAY_WIDTH - 1) {
            // fetch next value and clear line in advance
            tNextValueByte = DataBufferControl.DisplayBuffer[tBufferIndex + 1];
            LocalDisplay.drawLineFastOneX(tBufferIndex, tValueByte, tNextValueByte, DisplayControl.EraseColor);
        }

        /*
         * get new value
         */
        tValueByte = *DataBufferControl.DataBufferNextDrawPointer++;
        *tDisplayBufferPointer = tValueByte;

        if (tBufferIndex != 0 && tBufferIndex <= LOCAL_DISPLAY_WIDTH - 1) {
            // get last value and draw line
            tLastValueByte = DataBufferControl.DisplayBuffer[tBufferIndex - 1];
            LocalDisplay.drawLineFastOneX(tBufferIndex - 1, tLastValueByte, tValueByte, COLOR_DATA_RUN);
        }
        tBufferIndex++;
        DataBufferControl.DataBufferNextDrawIndex = tBufferIndex;
    }
}

/************************************************************************
 * Text output section
 ************************************************************************/
/*
 * Output info line
 * for documentation see line 33 of this file
 */
void printInfo() {
    char tSlopeChar;
    char tTimebaseUnitChar;
    char tReferenceChar;
    char tMinStringBuffer[6];
    char tAverageStringBuffer[6];
    char tMaxStringBuffer[6];
    char tP2PStringBuffer[6];
    char tTriggerStringBuffer[6];
    float tRefMultiplier;

    float tVoltage;

    if (MeasurementControl.TriggerSlopeRising) {
        tSlopeChar = 'A';
    } else {
        tSlopeChar = 'D';
    }

    if (MeasurementControl.ADCReference == DEFAULT) {
        tReferenceChar = '5';
        tRefMultiplier = 5.0 / 1023.0;
    } else {
        tReferenceChar = '1';
        tRefMultiplier = 1.1 / 1023.0;
    }

// 2 kByte code size
    tVoltage = tRefMultiplier * MeasurementControl.RawValueMin;
    dtostrf(tVoltage, 4, 2, tMinStringBuffer);
    tVoltage = tRefMultiplier * MeasurementControl.ValueAverage;
    dtostrf(tVoltage, 4, 2, tAverageStringBuffer);
    tVoltage = tRefMultiplier * MeasurementControl.RawValueMax;
    dtostrf(tVoltage, 4, 2, tMaxStringBuffer);
    tVoltage = tRefMultiplier * (MeasurementControl.RawValueMax - MeasurementControl.RawValueMin);
    dtostrf(tVoltage, 4, 2, tP2PStringBuffer);

    if (MeasurementControl.TriggerSlopeRising) {
        tVoltage = MeasurementControl.TriggerLevelUpper;
    } else {
        tVoltage = MeasurementControl.TriggerLevelLower;
    }
    tVoltage = tRefMultiplier * tVoltage;
    dtostrf(tVoltage, 4, 2, tTriggerStringBuffer);

    uint16_t tTimebaseUnitsPerGrid;
    if (MeasurementControl.TimebaseIndex >= TIMEBASE_INDEX_MILLIS) {
        tTimebaseUnitChar = 'm';
    } else {
        tTimebaseUnitChar = '\xF5'; // micro
    }
    tTimebaseUnitsPerGrid = TimebaseDivPrintValues[MeasurementControl.TimebaseIndex];

    // First line
    sprintf_P(sStringBuffer, PSTR("%3u%cs %c C%c %s %s %s %sV %sV %c"), tTimebaseUnitsPerGrid, tTimebaseUnitChar, tSlopeChar,
            MeasurementControl.ADCInputMUXChannelChar, tMinStringBuffer, tAverageStringBuffer, tMaxStringBuffer, tP2PStringBuffer,
            tTriggerStringBuffer, tReferenceChar);
    LocalDisplay.drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN, sStringBuffer, TEXT_SIZE_11, COLOR16_BLACK, COLOR_INFO_BACKGROUND);

    /*
     * Period and frequency
     */
    uint32_t tMicrosPeriod = MeasurementControl.PeriodMicros;
    uint32_t tHertz = 0;
    if (tMicrosPeriod != 0) {
        tHertz = 1000000 / tMicrosPeriod;
    }
    char tPeriodUnitChar = '\xF5'; // micro
    if (tMicrosPeriod >= 50000l) {
        tMicrosPeriod = tMicrosPeriod / 1000;
        tPeriodUnitChar = 'm';
    }

    sprintf_P(sStringBuffer, PSTR(" %5lu%cs  %5luHz"), tMicrosPeriod, tPeriodUnitChar, tHertz);
    if (tMicrosPeriod >= 1000) {
        // format nicely - 44 bytes
        // set separator for thousands
        sStringBuffer[0] = sStringBuffer[1];
        sStringBuffer[1] = sStringBuffer[2];
        sStringBuffer[2] = THOUSANDS_SEPARATOR;
    }
    if (tHertz >= 1000) {
        // set separator for thousands
        sStringBuffer[9] = sStringBuffer[10];
        sStringBuffer[10] = sStringBuffer[11];
        sStringBuffer[11] = THOUSANDS_SEPARATOR;
    }
//	 44 bytes
//	char * StringPointer = &sStringBuffer[2];
//	char tChar;
//	for (uint8_t i = 2; i > 0; i--) {
//		tChar = *StringPointer;
//	set separator for thousands
//		*StringPointer-- = THOUSANDS_SEPARATOR;
//		tReferenceChar = *StringPointer;
//		*StringPointer-- = tChar;
//		*StringPointer = tReferenceChar;
//		StringPointer = &sStringBuffer[11];
//	}

    // second line
    LocalDisplay.drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + TEXT_SIZE_11 + INFO_LINE_SPACING, sStringBuffer, TEXT_SIZE_11,
    COLOR16_BLACK, COLOR_INFO_BACKGROUND);
}

#if defined(DEBUG)
//show touchpanel data
void printTPData(bool aGuiTouched) {
    sprintf_P(sStringBuffer, PSTR("X:%03i|%04i Y:%03i|%04i P:%03i"), TouchPanel.getX(), TouchPanel.getXraw(),
            TouchPanel.getY(), TouchPanel.getYraw(), TouchPanel.getPressure());
    uint16_t tColor = COLOR_BACKGROUND_DSO;
    if (aGuiTouched) {
        tColor = COLOR_GREEN;
    }
    drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + 3 * TEXT_SIZE_11_HEIGHT, sStringBuffer, 1, COLOR16_BLACK, tColor);
}

void printDebugData() {
    sprintf_P(sStringBuffer, PSTR("%5u,%5u,%5u,%5u"), DebugValue1, DebugValue2, DebugValue3, DebugValue4);
    drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + 2 * TEXT_SIZE_11_HEIGHT, sStringBuffer, 1, COLOR16_BLACK, COLOR_BACKGROUND_DSO);
}
#endif

/************************************************************************
 * Utility section
 ************************************************************************/

void computeMicrosPerPeriod() {
    /*
     * detect micros of period
     */
    uint8_t *DataPointer = &DataBufferControl.DisplayBuffer[0];
    uint8_t tValue;
    uint16_t tCount = 0;
    uint16_t tCountPosition = 0;
    uint16_t i = 0;
    uint8_t tTriggerStatus = TRIGGER_START;
    uint8_t tTriggerValueUpper = getDisplayFromRawValue(MeasurementControl.TriggerLevelUpper);
    uint8_t tTriggerValueLower = getDisplayFromRawValue(MeasurementControl.TriggerLevelLower);

    uint32_t tMicrosPerPeriod;
    for (; i < LOCAL_DISPLAY_WIDTH; ++i) {
        tValue = *DataPointer++;
        //
        // Display buffer contains inverted values !!!
        //
        if (MeasurementControl.TriggerSlopeRising) {
            if (tTriggerStatus == TRIGGER_START) {
                // rising slope - wait for value below 1. threshold
                if (tValue > tTriggerValueLower) {
                    tTriggerStatus = TRIGGER_BEFORE_THRESHOLD;
                }
            } else {
                // rising slope - wait for value to rise above 2. threshold
                if (tValue < tTriggerValueUpper) {
                    if (i < 5) {
                        // no reliable value ??? skip result
                        break;
                    } else {
                        // search for next slope
                        tCount++;
                        tCountPosition = i;
                        tTriggerStatus = TRIGGER_START;
                    }
                }
            }
        } else {
            if (tTriggerStatus == TRIGGER_START) {
                // falling slope - wait for value above 1. threshold
                if (tValue < tTriggerValueUpper) {
                    tTriggerStatus = TRIGGER_BEFORE_THRESHOLD;
                }
            } else {
                // falling slope - wait for value to go below 2. threshold
                if (tValue > tTriggerValueLower) {
                    if (i < 5) {
                        // no reliable value ??? skip result
                        break;
                    } else {
                        // search for next slope
                        tCount++;
                        tCountPosition = i;
                        tTriggerStatus = TRIGGER_START;
                    }
                }
            }
        }
    }

    // compute microseconds per period
    if (tCountPosition == 0 || tCount == 0) {
        MeasurementControl.PeriodMicros = 0;
    } else {
        tMicrosPerPeriod = tCountPosition + 1;
        tMicrosPerPeriod = tMicrosPerPeriod * pgm_read_dword(&TimebaseDivExactValues[MeasurementControl.TimebaseIndex]);
        tMicrosPerPeriod = tMicrosPerPeriod / (tCount * TIMING_GRID_WIDTH);
        MeasurementControl.PeriodMicros = tMicrosPerPeriod;
    }
}

void FeedbackTone(bool isNoError) {
    if (isNoError) {
        tone(LOCAL_GUI_FEEDBACK_TONE_PIN, 3000, 50);
    } else {
        // two short beeps
        tone(LOCAL_GUI_FEEDBACK_TONE_PIN, 3000, 30);
        delay(60);
        tone(LOCAL_GUI_FEEDBACK_TONE_PIN, 3000, 30);
    }
}

void FeedbackToneOK() {
    FeedbackTone(true);
}

uint8_t getDisplayFromRawValue(uint16_t aAdcValue) {
    aAdcValue -= MeasurementControl.ValueOffset;
    aAdcValue >>= MeasurementControl.ValueShift;
//clip Value to LOCAL_DISPLAY_HEIGHT
    if (aAdcValue > DISPLAY_VALUE_FOR_ZERO) {
        aAdcValue = 0;
    } else {
        aAdcValue = DISPLAY_VALUE_FOR_ZERO - aAdcValue;
    }
    return aAdcValue;
}

uint16_t getRawFromDisplayValue(uint8_t aValue) {
    aValue = DISPLAY_VALUE_FOR_ZERO - aValue;
    uint16_t aValue16 = aValue << MeasurementControl.ValueShift;
    aValue16 = aValue16 + MeasurementControl.ValueOffset;
    return aValue16;
}
/*
 * computes corresponding voltage from display y position (LOCAL_DISPLAY_HEIGHT - 1 -> 0 Volt)
 */
float getFloatFromDisplayValue(uint8_t aValue) {
    aValue = DISPLAY_VALUE_FOR_ZERO - aValue;
    uint16_t aValue16 = aValue << MeasurementControl.ValueShift;
    aValue16 = aValue16 + MeasurementControl.ValueOffset;
    return (5.0 / 1023.0) * aValue16;
}

void setMUX(uint8_t aChannel) {
    ADMUX = (ADMUX & ~0x0F) | aChannel;
    MeasurementControl.ADCInputMUXChannel = aChannel;
}

inline void setPrescaleFactor(uint8_t aFactor) {
    ADCSRA = (ADCSRA & ~0x07) | aFactor;
}

void setReference(uint8_t aReference) {
    ADMUX = (ADMUX & ~0xC0) | (aReference << 6);
    MeasurementControl.ADCReference = aReference;
}

/************************************************************************
 * RTC section
 ************************************************************************/
#ifdef USE_RTC

uint8_t bcd2bin(uint8_t val) {
    return val - 6 * (val >> 4);
}
uint8_t bin2bcd(uint8_t val) {
    return val + 6 * (val / 10);
}

/*
 * aRTCBuffer Pointer to uint8 [7] 0 is second, 3 is day of week,  6 is year
 */
void readRtcTime(uint8_t * aRTCBuffer) {
    /*
     * get time from RTC
     */
// write start address
    i2c_start(DS1307_ADDR + I2C_WRITE);// set device address and write mode
    i2c_write(0x00);
    i2c_stop();

// read 6 bytes from start address
    i2c_start(DS1307_ADDR + I2C_READ);// set device address and read mode
    for (uint8_t i = 0; i < 6; ++i) {
        aRTCBuffer[i] = bcd2bin(i2c_readAck());
    }
// read year and stop
    aRTCBuffer[6] = bcd2bin(i2c_readNak());
    i2c_stop();
}

void setRTCTime(uint8_t * aRTCBuffer, bool aForcedSet) {
    uint8_t RtcBuf[7];
    readRtcTime(RtcBuf);
    for (uint8_t i = 6; i >= 0; --i) {
        if (aRTCBuffer[i] > RtcBuf[i]) {
            // new date is after (later than) actual date do update
            aForcedSet = true;
            break;
        } else if (!aForcedSet && aRTCBuffer[i] < RtcBuf[i]) {
            // new date is before actual date => leave aForcedSet as false and quit
            break;
        }
    }

    if (aForcedSet) {
        //Write Start Address
        i2c_start(DS1307_ADDR + I2C_WRITE);// set device address and write mode
        i2c_write(0x00);
        // write data
        for (uint8_t i = 0; i < 6; ++i) {
            i2c_write(bin2bcd(aRTCBuffer[i]));
        }
        i2c_stop();
    }
}

/*
 * 10,8 ms
 */
void showRTCTime() {
    static uint8_t sLastSecond;
    uint8_t RtcBuf[7];
    readRtcTime(RtcBuf);

//buf[3] is day of week
    if (RtcBuf[0] != sLastSecond) {
        sLastSecond = RtcBuf[0];
        sprintf_P(sStringBuffer, PSTR("%02i.%02i.%04i %02i:%02i:%02i"), RtcBuf[4], RtcBuf[5], RtcBuf[6] + 2000, RtcBuf[2], RtcBuf[1],
                RtcBuf[0]);
        LocalDisplay.drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + 2 * 13, sStringBuffer, 11, COLOR16_RED, COLOR_BACKGROUND_DSO);
    }
}
#endif

