/*
 * SimpleTouchScreenDSO.cpp
 *
 *  Copyright (C) 2015  Armin Joachimsmeyer
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
 *      Features:
 *      No dedicated hardware, just a plain arduino, a HC-05 Bluetooth module and this software.
 *      Full touch screen control of all parameters.
 *      150/300 kSamples per second.
 *      Supports AC Measurement with (passive) external attenuator circuit.
 *      3 different types of external attenuator detected by software.
 *        - no attenuator (pin 8+9 left open).
 *        - passive attenuator with /1, /10, /100 attenuation (pin 8 connected to ground).
 *        - active attenuator (pin 9 connected to ground).
 *      Automatic trigger, range and offset value selection.
 *      External trigger as well as delayed trigger possible.
 *      1120 byte data buffer - 3 * display width.
 *      Min, max, average and peak to peak display.
 *      Period and frequency display.
 *      All settings can be changed during measurement.
 *      Gesture (swipe) control of timebase and chart.
 *      All AVR ADC input channels selectable.
 *      Touch trigger level select.
 *      1.1 volt internal reference. 5 volt (VCC) also usable.
 *
 *      The code can also be used as an example of C++/Assembler coding
 *      and complex interrupt handling routine
 *
 *      The touch DSO software has 4 pages.
 *      1. The start page which shows all the hidden buttons available at the chart page.
 *      2. The chart page which shows the data and info line(s).
 *      3. The settings page.
 *      4. The frequency generator page.
 *      The display of new data on the settings and frequency generator page helps to understand the effect of the settings chosen.
 *
 *      Control features:
 *      Long touch toggles between display of start and chart page.
 *      On chart page at the left a voltage picker (slider) is shown.
 *      If trigger manual is enabled a second slider area is shown to select trigger value.
 *      Short touch on chart page at no (hidden) button area and switches through the info line modes.
 *      Horizontal swipe on chart page changes timebase while running, else scrolls the data chart.
 *      Vertical swipe on chart page changes the range if manual range is enabled.
 *
 *      Buttons at the settings page:
 *      "Trigger man timeout" means manual trigger value, but with timeout, i.e. if trigger condition not met, new data is shown after timeout.
 *          - This is not really a manual trigger level mode, but it helps to find the right trigger value.
 *      "Trigger man" means manual trigger, but without timeout, i.e. if trigger condition not met, no new data is shown.
 *      "Trigger free" means free running trigger, i.e. trigger condition is always met.
 *      "Trigger ext" uses pin 2 as external trigger source.
 *      "Trigger delay" can be specified from 4 us to 64.000 ms (if you really want).
 *
 *      Info line at the chart page
 *      Short version - 1 line
 *      Average voltage, peak to peak voltage, frequency, timebase
 *
 *      Long version - 2 lines
 *      1. line: timebase, trigger slope, channel, (min, average, max, peak to peak) voltage, trigger level, reference voltage.
 *                  - timebase: Time for div (31 pixel).
 *                  - (min, average, max, peak to peak) voltage: In hold mode, chart is larger than display and the values are for the whole chart!
 *                  - Reference voltage: 5=5V 1  1=1.1Volt-internal-reference.
 *                  - Number of input channel (1-5) and Temp=AVR-temperature VRef=1.1Volt-internal-reference
 *
 *      2. line: frequency, period, 1st interval, 2nd interval (, trigger delay)
 *
 *      "Offset man" is currently not implemented and works like "Offset 0V".
 *
 *      "DC / AC" Button is only visible for these channels having a AC/DC switch/input at configurations ATTENUATOR_TYPE_FIXED_ATTENUATOR + ATTENUATOR_TYPE_ACTIVE_ATTENUATOR
 */

/*
 * SIMPLE EXTERNAL ATTENUATOR CIRCUIT (configured by connecting pin 8 to ground)
 *
 * Attenuator circuit and AC/DC switch
 *
 *                ADC INPUT_0  1.1 volt         ADC INPUT_1 11 volt        ADC INPUT_2 110 volt
 *                      /\                         /\     ______              /\    ______
 *                      |                          +-----| 220k |----+        +----| 10k  |-----------+
 *                      |     _______              |      ______     |        |     ------            |
 *                      +----| >4.7M |----+        +-----| 220k |----+        |     ______            |
 *                      |     -------     |        |      ------     |        +----| 4.7M |-+ 2*4.7M or
 *                      _                 |        _                 |        _     ------  _ 3*3.3M  |
 *                     | |                |       | |                |       | |           | |        |
 *                     | | 10k            |       | | 1M             |       | | 1 M       | | 4.7 M  |
 *                     | |                |       | |                |       | |           | |        |
 *                      -                 |        -                 |        -             -         |
 *       VREF           |                 |        |                 |        |             |         |
 *         /\           +----+            |        +----+            |        +----+        +----+    |
 *         |            |    |            |        |    |            |        |    |        |    |    |
 *         _            |    = C 0.1uF    |        |    = C 0.1uF    |        |    = 0.1uF  |    = 0.1uF  400 volt
 *        | |           |    |            |        |    |            |        |    |        |    |    |
 *        | | 100k      O    O            |        O    O            |        O    O        O    O    |
 *        | |          DC   AC            |       DC   AC            |       DC   AC       DC   AC    |
 *         -                              |                          |                   1000 V Range |
 *         |                              |                          |                                |
 *   A5 ---+-------------+----------------+--------------------------+--------------------------------+
 * AC bias |             |
 *         _             |   Sine/Triangle/Sawtooth   D10                D2
 *        | |            |            O               \/                 /\
 *        | | 100k       = 6.8uF      |   ______      |                  |
 *        | |            |            +--| 2.2k |-----+                  _
 *         -             |            |   ------      |                 | |
 *         |             |            = 100nF         |                 | | 100k
 *         |             |            |               O                 |_|
 *  GND ---+-------------+------------+      SquareWave 0.1 - 8 MHz      |                                                     -
 *                                                                       O
 *                                                                External_Trigger
 *
 *
 */

/*
 * Attention: since 5.0 / 1024.0 = 0,004883 volt is the resolution of the ADC, depending of scale factor:
 * 1. The output values for 2 adjacent display (min/max) values can be identical
 * 2. The voltage picker value may not reflect the real sample value (e.g. shown for min/max)
 */

/*
 * PIN
 * 2    External trigger input
 * 3   Not yet used
 * 4   - If active attenuator, attenuator range control, else not yet used
 * 5   - If active attenuator, attenuator range control, else not yet used
 * 6   - If active attenuator, AC (high) / DC (low) relay, else debug output of half the timebase of Timer 0 for range 496 us and higher -> frequency <= 31,25 kHz (see changeTimeBaseValue())
 * 7   Not yet used
 * 8    Attenuator configuration input with internal pullup - bit 0
 * 9    Attenuator configuration input with internal pullup - bit 1  11-> no attenuator attached, 10-> simple (channel 0-2) attenuator attached, 0x-> active (channel 0-1) attenuator attached
 * 10   Frequency / waveform generator output of Timer1 (16 bit)
 * 11  - If active attenuator, square wave for VEE (-5 volt) generation by timer2 output, else not yet used
 * 12  Not yet used
 * 13   Internal LED / timing debug output
 *
 * A5   AC bias - if mode is AC, high impedance (input), if mode is DC, set as output LOW
 *
 * Timer0  8 bit - Generates sample frequency
 * Timer1 16 bit - Internal waveform generator
 * Timer2  8 bit - Triggers ISR for Arduino millis() since timer0 is not available for this (switched from timer 0 at setup())
 */

/*
 * IMPORTANT - do not use Arduino Serial.* here otherwise the usart interrupt kills the timing.
 */

//#define DEBUG
#include <Arduino.h>

/*
 * Settings to configure the BlueDisplay library and to reduce its size
 */
//#define BLUETOOTH_BAUD_RATE BAUD_115200  // Activate this, if you have reprogrammed the HC05 module for 115200, otherwise 9600 is used as baud rate
//#define DO_NOT_NEED_BASIC_TOUCH_EVENTS // Disables basic touch events like down, move and up. Saves 620 bytes program memory and 36 bytes RAM
#define USE_SIMPLE_SERIAL // Do not use the Serial object. Saves up to 1250 bytes program memory and 185 bytes RAM, if Serial is not used otherwise
#include "BlueDisplay.hpp"

#include "SimpleTouchScreenDSO.h"
#include "FrequencyGeneratorPage.h"

#include "digitalWriteFast.h"

#if ! defined(USE_SIMPLE_SERIAL)
#error TouchScreenDSO works only with USE_SIMPLE_SERIAL activated, since the serial interrupts kill the DSO timing!
#endif
/**********************
 * Buttons
 *********************/

BDButton TouchButtonBack;
// global flag for page control. Is evaluated by calling loop or page and set by buttonBack handler
bool sBackButtonPressed;

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__)
//#define INTERNAL1V1 2
#undef INTERNAL
#define INTERNAL 2
#else
#define INTERNAL 3
#endif

#if !defined(TIMSK2)
// on ATmega32U4 we have no timer2 but one timer3
#define TIMSK2 TIMSK3
#define TOIE2  TOIE3
#endif

#define ADC_TEMPERATURE_CHANNEL 8
#define ADC_1_1_VOLT_CHANNEL 0x0E

/*
 * Timebase values overview:                                              Polling mode
 *                            conversion                                   Fast Ultra
 * idx range   ADCpresc. clk     us    us/div  us/320  x-scale  TIMER0 CTC mode fast
 * 0   10us    PRESCALE4 0.25     3.25  101.75  1040    10  prescaler            x
 * 1   20us    PRESCALE4 0.25     3.25  101.75  1040     5      micros           x
 * 2   50us    PRESCALE4 0.25     3.25  101.75  1040     2           value       x
 * 3  101us    PRESCALE8  0.5     6.5   201.5   2080     2                  x
 * 4  201us    PRESCALE8  0.5     6.5   201.5   2080     1                  x
 * 5  496us    PRESCALE16   1    16     496     5120     1     8  0.5   32
 * 6    1ms    PRESCALE32   2    32     992    10240     1     8  0.5   64
 * 7    2ms    PRESCALE64   4    64    1984    20480     1     8  0.5  128
 * 8    5ms    PRESCALE128  8   160    4960    51200     1    64    4   40
 * 9   10ms    PRESCALE128  8   320    9920   102400     1    64    4   80  Draw while
 * 10  20ms    PRESCALE128  8   648   20088   207360     1    64    4  162   acquire
 * 11  50ms    PRESCALE128  8  1616   50096   517120     1   256   16  101      x
 * 12 100ms    PRESCALE128  8  3224   99944   517120     1   256   16  201.5    x
 * 12 100ms    PRESCALE128  8  3216   99696   517120     1   256   16  201      x
 * 12 100ms    PRESCALE128  8  3232  100192   517120     1   256   16  202      x
 * 13 200ms    PRESCALE128  8  6448  199888   517120     1  1024   64  100.75   x
 * 14 200ms    PRESCALE128  8  6464  200384   517120     1  1024   64  101      x
 * 15 500ms    PRESCALE128  8 16128  499968  5160960     1  1024   64  252      x
 */

// for 31 grid
const uint16_t TimebaseDivPrintValues[TIMEBASE_NUMBER_OF_ENTRIES] PROGMEM = { 10, 20, 50, 101, 201, 496, 1, 2, 5, 10, 20, 50, 100,
        200, 500 };
// exact values for 31 grid - for period and frequency
const float TimebaseExactDivValuesMicros[TIMEBASE_NUMBER_OF_ENTRIES] PROGMEM
= { 100.75/*(31*13*0,25)*/, 100.75, 100.75, 201.5, 201.5 /*(31*13*0,5)*/, 496 /*(31*16*1)*/, 992 /*(31*16*2)*/, 1984 /*(31*16*4)*/,
        4960 /*(31*20*8)*/, 9920 /*(31*40*8)*/, 20088 /*(31*81*8)*/, 50096 /*(31*202*8)*/, 99696 /*(31*201*16)*/,
        200384 /*(31*808*8)*/, 499968 /*(31*2016*8)*/};

const uint8_t xScaleForTimebase[TIMEBASE_NUMBER_OF_XSCALE_CORRECTION] = { 10, 5, 2, 2 }; // multiply displayed values to simulate a faster timebase.
// since prescale PRESCALE4 has bad quality use PRESCALE8 for 201 us range and display each value twice
// All timebase >= TIMEBASE_NUMBER_OF_FAST_PRESCALE use PRESCALE128
const uint8_t ADCPrescaleValueforTimebase[TIMEBASE_NUMBER_OF_FAST_PRESCALE] = { ADC_PRESCALE4, ADC_PRESCALE4, ADC_PRESCALE4,
ADC_PRESCALE8, ADC_PRESCALE8, ADC_PRESCALE16 /*496us*/, ADC_PRESCALE32, ADC_PRESCALE64 /*2ms*/};

const uint8_t CTCValueforTimebase[TIMEBASE_NUMBER_OF_ENTRIES - TIMEBASE_NUMBER_OF_FAST_MODES] = { 32/*496us*/, 64, 128/*2ms*/, 40,
        80/*10ms*/, 162, 101, 201, 101, 252 };
// only for information - actual code needs 2 bytes more than code using this table, but this table takes 10 byte of RAM/Stack
const uint8_t CTCPrescaleValueforTimebase[TIMEBASE_NUMBER_OF_ENTRIES - TIMEBASE_NUMBER_OF_FAST_MODES] = { TIMER0_PRESCALE8/*496us*/,
TIMER0_PRESCALE8, TIMER0_PRESCALE8/*2ms*/, TIMER0_PRESCALE64,
TIMER0_PRESCALE64/*10ms*/, TIMER0_PRESCALE64, TIMER0_PRESCALE256, TIMER0_PRESCALE256, TIMER0_PRESCALE1024,
TIMER0_PRESCALE1024 };

/*
 * storage for millis value to enable compensation for interrupt disable at signal acquisition etc.
 */
extern volatile unsigned long timer0_millis;

/****************************************
 * Automatic triggering and range stuff
 */
#define TRIGGER_WAIT_NUMBER_OF_SAMPLES 3300 // Number of samples (<=112us) used for detecting the trigger condition
#define ADC_CYCLES_PER_CONVERSION 13
#define SCALE_CHANGE_DELAY_MILLIS 2000

#define ADC_MAX_CONVERSION_VALUE (1024 -1) // 10 bit
#define ATTENUATOR_FACTOR 10
#define DISPLAY_AC_ZERO_OFFSET_GRID_COUNT (-3)

/******************************
 * Measurement control values
 *****************************/
struct MeasurementControlStruct MeasurementControl;

/*
 * Display control
 * while running switch between upper info line on/off
 * while stopped switch between chart / t+info line and gui
 */

DisplayControlStruct DisplayControl;

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

// definitions from <wiring_private.h>
#if !defined(cbi)
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#if !defined(sbi)
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

/***************
 * Debug stuff
 ***************/
#if defined(DEBUG)
void printDebugData(void);

uint16_t DebugValue1;
uint16_t DebugValue2;
uint16_t DebugValue3;
uint16_t DebugValue4;
#endif

/***********************************************************************************
 * Put those variables at the end of data section (.data + .bss + .noinit)
 * adjacent to stack in order to have stack overflows running into DataBuffer array
 * *********************************************************************************/
/*
 * a string buffer for any purpose...
 */
char sStringBuffer[SIZEOF_STRINGBUFFER] __attribute__((section(".noinit")));
// safety net - array overflow will overwrite only DisplayBuffer
DataBufferStruct DataBufferControl __attribute__((section(".noinit")));

/*******************************************************************************************
 * Function declaration section
 *******************************************************************************************/

void acquireDataFast(void);

// Measurement auto control stuff (trigger, range + offset)
void computeAutoTrigger(void);
void setInputRange(uint8_t aShiftValue, uint8_t aActiveAttenuatorValue);
bool checkRAWValuesForClippingAndChangeRange(void);
void computeAutoRange(void);
void computeAutoOffset(void);

// Attenuator support stuff
void setAttenuator(uint8_t aNewValue);
uint16_t getAttenuatorFactor(void);

// GUI initialization and drawing stuff
void initDSOGUI(void);
void activateChartGui(void);

// BUTTON handler section

// Graphical output section
void clearDisplayedChart(uint8_t *aDisplayBufferPtr);
void drawRemainingDataBufferValues(void);

//Hardware support section
float getTemperature(void);
void setVCCValue(void);
inline void setPrescaleFactor(uint8_t aFactor);
void setADCReference(uint8_t aReference);
void setTimer2FastPWMOutput();
void initTimer2(void);

/*******************************************************************************************
 * Program code starts here
 * Setup section
 *******************************************************************************************/

void initDisplay(void) {
    BlueDisplay1.setFlagsAndSize(
            BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE | BD_FLAG_LONG_TOUCH_ENABLE | BD_FLAG_ONLY_TOUCH_MOVE_DISABLE,
            REMOTE_DISPLAY_WIDTH, REMOTE_DISPLAY_HEIGHT);
    BlueDisplay1.setCharacterMapping(0xD1, 0x21D1); // Ascending in UTF16 - for printInfo()
    BlueDisplay1.setCharacterMapping(0xD2, 0x21D3); // Descending in UTF16 - for printInfo()
    BlueDisplay1.setCharacterMapping(0xD4, 0x2227); // UP (logical AND) in UTF16
    BlueDisplay1.setCharacterMapping(0xD5, 0x2228); // Down (logical OR) in UTF16
    //\xB0 is degree character
    //BlueDisplay1.setButtonsTouchTone(TONE_PROP_BEEP_OK, 80);
    initDSOGUI();
#if !defined(__AVR_ATmega32U4__)
    // no frequency page in order to save space for leonardo
    initFrequencyGeneratorPage();
#endif
}

void setup() {
    // Set outputs at port D
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__) || defined(ARDUINO_AVR_LEONARDO) || defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__)
    pinMode(ATTENUATOR_0_PIN, OUTPUT);
    pinMode(ATTENUATOR_1_PIN, OUTPUT);
    pinMode(AC_DC_RELAY_PIN, OUTPUT);
#else
    DDRD = (DDRD & ~OUTPUT_MASK_PORTD) | OUTPUT_MASK_PORTD;
#endif
    // Set outputs at port B
    //    pinMode(TIMER_1_OUTPUT_PIN, OUTPUT);
    //    pinMode(VEE_PIN, OUTPUT);
    //    pinMode(DEBUG_PIN, OUTPUT);
    // 3 pinMode replaced by:
    DDRB = (DDRB & ~OUTPUT_MASK_PORTB) | OUTPUT_MASK_PORTB;
    pinMode(ATTENUATOR_DETECT_PIN_0, INPUT_PULLUP);
    pinMode(ATTENUATOR_DETECT_PIN_1, INPUT_PULLUP);

    // Set outputs at port C
    // For DC mode, change AC_DC_BIAS_PIN pin to output and set bias to 0 volt
    DDRC = OUTPUT_MASK_PORTC;
    digitalWriteFast(AC_DC_BIAS_PIN, LOW);

    // Shutdown SPI and TWI, enable all timers, USART and ADC
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__) || defined(ARDUINO_AVR_LEONARDO) || defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__)
    PRR0 = _BV(PRTWI) | _BV(PRTWI);
#else
    PRR = _BV(PRTWI) | _BV(PRTWI);
#endif
    // Disable  digital input on all ADC channel pins to reduce power consumption for levels near half VCC
    DIDR0 = ADC0D | ADC1D | ADC2D | ADC3D | ADC4D | ADC5D;

    // Must be simple serial for the DSO!
    initSerial();

    // initialize values
    MeasurementControl.isRunning = false;
    MeasurementControl.TriggerSlopeRising = true;
    MeasurementControl.TriggerMode = TRIGGER_MODE_AUTOMATIC;
    MeasurementControl.isSingleShotMode = false;

    /*
     * Read input pins to determine attenuator type
     */
    MeasurementControl.ChannelHasActiveAttenuator = false;
    MeasurementControl.AttenuatorValue = 0; // set direct input as default
    // read type of attached attenuator from 2 external connected pins
    uint8_t tAttenuatorType = !digitalReadFast(ATTENUATOR_DETECT_PIN_0);
    tAttenuatorType |= (!digitalReadFast(ATTENUATOR_DETECT_PIN_1)) << 1;
    MeasurementControl.AttenuatorType = tAttenuatorType;

    uint8_t tStartChannel = 0;
    if (tAttenuatorType == ATTENUATOR_TYPE_FIXED_ATTENUATOR) {
        tStartChannel = 1;
    }

    /*
     * disable Timer0 and start Timer2 as replacement to maintain millis()
     */
    TIMSK0 = _BV(OCIE0A); // enable timer0 Compare match A interrupt, since we need timer0 for timebase
    initTimer2(); // start timer2 to maintain millis() (and for generating VEE - negative voltage for external hardware)
    if (tAttenuatorType >= ATTENUATOR_TYPE_ACTIVE_ATTENUATOR) {
        setTimer2FastPWMOutput(); // enable timer2 for output 1 kHz at Pin11 generating VEE (negative voltage for external hardware)
    }

    MeasurementControl.TimebaseIndex = 0;
    changeTimeBaseValue(TIMEBASE_INDEX_START_VALUE);
    /*
     * changeTimeBaseValue() needs:
     * TimebaseIndex
     * TriggerMode
     * AttenuatorValue
     *
     * changeTimeBaseValue() sets:
     * DrawWhileAcquire
     * TimebaseDelay
     * TimebaseDelayRemaining
     * TriggerTimeoutSampleCount
     * XScale
     * Timer0
     */
    MeasurementControl.RawDSOReadingACZero = 0x200;

    delay(100);
    setVCCValue();

    setChannel(tStartChannel); // outputs button caption!
    /*
     * setChannel() needs:
     * AttenuatorType
     * isACMode
     * VCC
     *
     * setChannel() sets:
     * ShiftValue = 2
     * ADCInputMUXChannel
     * ADCInputMUXChannelChar
     *
     * And if AttenuatorDetected == true also:
     * ChannelHasAttenuator
     * isACModeANDChannelHasAttenuator
     * AttenuatorValue
     * ADCReference
     *
     * setChannel calls setInputRange(2,2) and this sets:
     * OffsetValue
     * HorizontalGridSizeShift8
     * HorizontalGridVoltage
     */

    MeasurementControl.RangeAutomatic = true;
    MeasurementControl.OffsetMode = OFFSET_MODE_0_VOLT;

    DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[0];

    DisplayControl.EraseColor = COLOR_BACKGROUND_DSO;
    DisplayControl.showHistory = false;
    DisplayControl.DisplayPage = DISPLAY_PAGE_START;
    DisplayControl.showInfoMode = INFO_MODE_SHORT_INFO;

    //setACMode(!digitalReadFast(AC_DC_PIN));
    clearDataBuffer();

    // first synchronize. Since a complete chart data can be missing, send minimum 320 byte
    for (int i = 0; i < 16; ++i) {
        BlueDisplay1.sendSync();
    }

    // Register callback handler and check for connection
    BlueDisplay1.initCommunication(&initDisplay, &redrawDisplay);
    registerSwipeEndCallback(&doSwipeEndDSO);
    registerTouchUpCallback(&doSwitchInfoModeOnTouchUp);
    registerLongTouchDownCallback(&doLongTouchDownDSO, 900);

    BlueDisplay1.playFeedbackTone(FEEDBACK_TONE_OK);
    delay(400);
    setVCCValue();
    BlueDisplay1.playFeedbackTone(FEEDBACK_TONE_OK);

    initStackFreeMeasurement();

}

/************************************************************************
 * main loop - 32 microseconds
 ************************************************************************/
// noreturn saves 56 byte program memory!
void __attribute__((noreturn)) loop(void) {
    uint32_t sMillisOfLastInfoOutput;

    bool sDoInfoOutput = true; // Output info at least every second for settings page, single shot or trigger not found

    for (;;) {
        checkAndHandleEvents();
        if (BlueDisplay1.mBlueDisplayConnectionEstablished) {

            /*
             * Check for cyclic info output
             */
            if (millis() - sMillisOfLastInfoOutput > MILLIS_BETWEEN_INFO_OUTPUT) {
                sMillisOfLastInfoOutput = millis();
                sDoInfoOutput = true;
            }

            if (MeasurementControl.isRunning) {
                if (MeasurementControl.AcquisitionFastMode) {
                    /*
                     * Fast mode here  <= 201us/div
                     */
                    acquireDataFast();
                }
                if (DataBufferControl.DataBufferFull) {
                    /*
                     * Data (from InterruptServiceRoutine) is ready
                     */

                    /*
                     * Enable Timer0 overflow interrupt again
                     * and compensate for missing ticks because timer was disabled not to disturb acquisition.
                     * 320.0 / 31.0 = divs per screen
                     * 4 * 256 = micro seconds per interrupt
                     */
                    uint32_t tCompensation = ((320.0 / 31.0) / (4 * 256))
                            * pgm_read_float(&TimebaseExactDivValuesMicros[MeasurementControl.TimebaseIndex]);
                    timer0_millis += tCompensation;
                    TIMSK2 = _BV(TOIE2); // Enable overflow interrupts which replaces the Arduino millis() interrupt

                    /*
                     * Handle cyclicly print info or refresh buttons
                     */
                    if (sDoInfoOutput) {
                        sDoInfoOutput = false;

                        if (DisplayControl.DisplayPage == DISPLAY_PAGE_CHART) {
                            if (!DisplayControl.showHistory) {
                                // This enables slow display devices to skip frames
                                BlueDisplay1.clearDisplayOptional(COLOR_BACKGROUND_DSO);
                            }
                            drawGridLinesWithHorizLabelsAndTriggerLine();
                            if (DisplayControl.showInfoMode != INFO_MODE_NO_INFO) {
                                printInfo();
                            }
                        } else if (DisplayControl.DisplayPage == DISPLAY_PAGE_SETTINGS) {
                            // refresh buttons
                            drawDSOSettingsPage();
                        } else if (DisplayControl.DisplayPage == DISPLAY_PAGE_FREQUENCY) {
                            // refresh buttons
                            drawFrequencyGeneratorPage();
#if !defined(AVR)
                        } else if (DisplayControl.DisplayPage == DISPLAY_PAGE_MORE_SETTINGS) {
                            // refresh buttons
                            drawDSOMoreSettingsPage();
#endif
                        }
                    }

                    // Compute Average first
                    MeasurementControl.ValueAverage = (MeasurementControl.IntegrateValueForAverage
                            + (DataBufferControl.AcquisitionSize / 2)) / DataBufferControl.AcquisitionSize;

                    if (MeasurementControl.StopRequested) {
                        /*
                         * Handle stop
                         */
                        MeasurementControl.StopRequested = false;
                        MeasurementControl.isRunning = false;
                        if (MeasurementControl.ADCReference != DEFAULT) {
                            // get new VCC Value
                            setVCCValue();
                        }
                        if (MeasurementControl.isSingleShotMode) {
                            MeasurementControl.isSingleShotMode = false;
                            // Clear single shot character
                            clearSingleshotMarker();
                        }
                        redrawDisplay();
                    } else {
                        /*
                         * Normal loop -> process data, draw new chart, and start next acquisition
                         */
                        uint8_t tLastTriggerDisplayValue = DisplayControl.TriggerLevelDisplayValue;
                        computeAutoTrigger();
                        computeAutoRange();
                        computeAutoOffset();
                        // handle trigger line
                        DisplayControl.TriggerLevelDisplayValue = getDisplayFromRawInputValue(MeasurementControl.RawTriggerLevel);
                        if (tLastTriggerDisplayValue
                                != DisplayControl.TriggerLevelDisplayValue&& DisplayControl.DisplayPage == DISPLAY_PAGE_CHART) {
                            clearTriggerLine(tLastTriggerDisplayValue);
                            drawTriggerLine();
                        }

                        if (!DisplayControl.DrawWhileAcquire) {
                            /*
                             * Clear old chart and draw new data
                             */
                            drawDataBuffer(&DataBufferControl.DataBuffer[0], COLOR_DATA_RUN, DisplayControl.EraseColor);
                        }
                        startAcquisition();
                    }
                } else {
                    /*
                     * Here data buffer is NOT full and acquisition is still ongoing
                     */

                    /*
                     * Handle slow modes (draw while acquire)
                     */
                    if (DisplayControl.DrawWhileAcquire) {
                        drawRemainingDataBufferValues();
                        MeasurementControl.RawValueMin = MeasurementControl.ValueMinForISR;
                        MeasurementControl.RawValueMax = MeasurementControl.ValueMaxForISR;
                        if (checkRAWValuesForClippingAndChangeRange()) {
                            // range changed set new values for ISR to avoid another range change by the old values
                            MeasurementControl.ValueMinForISR = 42;
                            MeasurementControl.ValueMaxForISR = 42;
                        }
                    }
                }
#if defined(DEBUG)
                //            DebugValue1 = MeasurementControl.ShiftValue;
                //            DebugValue2 = MeasurementControl.RawValueMin;
                //            DebugValue3 = MeasurementControl.RawValueMax;
                //            printDebugData();
#endif

                /*
                 * Handle single shot, or trigger not yet found -> output actual values cyclicly
                 */
                if (MeasurementControl.isSingleShotMode && MeasurementControl.TriggerStatus != TRIGGER_STATUS_FOUND
                        && sDoInfoOutput) {
                    sDoInfoOutput = false;
                    MeasurementControl.ValueAverage = MeasurementControl.ValueBeforeTrigger;
                    MeasurementControl.PeriodMicros = 0;
                    MeasurementControl.PeriodFirst = 0;
                    MeasurementControl.PeriodSecond = 0;
                    printInfo(false);
                }

                /*
                 * Handle milliseconds delay - not timer overflow proof
                 */
                if (MeasurementControl.TriggerStatus == TRIGGER_STATUS_FOUND_AND_WAIT_FOR_DELAY) {
                    if (MeasurementControl.TriggerDelayMillisEnd == 0) {
                        // initialize value since this can not be efficiently done in ISR
                        MeasurementControl.TriggerDelayMillisEnd = millis() + MeasurementControl.TriggerDelayMillisOrMicros;
                    } else if (millis() > MeasurementControl.TriggerDelayMillisEnd) {
                        /*
                         * Start acquisition
                         */
                        MeasurementControl.TriggerStatus = TRIGGER_STATUS_FOUND;
                        if (MeasurementControl.AcquisitionFastMode) {
                            // NO Interrupt in FastMode
                            ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADIF) | MeasurementControl.TimebaseHWValue;
                        } else {
                            //  enable ADC interrupt, start with free running mode,
                            ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADIF) | ADC_PRESCALE_FOR_TRIGGER_SEARCH | _BV(ADIE);
                        }
                    }
                }
            } else {

                /*
                 * Analyze mode here
                 */
                if (sDoInfoOutput && DisplayControl.DisplayPage == DISPLAY_PAGE_SETTINGS) {
                    sDoInfoOutput = false;
                    /*
                     * show VCC and Temp and stack
                     */
                    printVCCAndTemperature();
                    printFreeStack();
                }
            }

            /*
             * Handle Sub-Pages
             */
            if (DisplayControl.DisplayPage == DISPLAY_PAGE_SETTINGS) {
                if (sBackButtonPressed) {
                    sBackButtonPressed = false;
                    DisplayControl.DisplayPage = DISPLAY_PAGE_CHART;
                    redrawDisplay();
                }
            } else if (DisplayControl.DisplayPage == DISPLAY_PAGE_FREQUENCY) {
                if (sBackButtonPressed) {
                    sBackButtonPressed = false;
                    stopFrequencyGeneratorPage();
                    DisplayControl.DisplayPage = DISPLAY_PAGE_SETTINGS;
                    redrawDisplay();
                } else {
                    //not required here, because is contains only checkAndHandleEvents()
                    // loopFrequencyGeneratorPage();
                }
            }
        } // BlueDisplay1.mBlueDisplayConnectionEstablished

    } // for(;;)
}
/* Main loop end */

/************************************************************************
 * Measurement section
 ************************************************************************/
/*
 * prepares all variables for new acquisition
 * switches between fast an interrupt mode depending on TIMEBASE_FAST_MODES
 * sets ADC status register including prescaler
 */
void startAcquisition(void) {
    DataBufferControl.AcquisitionSize = REMOTE_DISPLAY_WIDTH;
    DataBufferControl.DataBufferEndPointer = &DataBufferControl.DataBuffer[REMOTE_DISPLAY_WIDTH - 1];
    if (MeasurementControl.StopRequested) {
        DataBufferControl.AcquisitionSize = DATABUFFER_SIZE;
        DataBufferControl.DataBufferEndPointer = &DataBufferControl.DataBuffer[DATABUFFER_SIZE - 1];
    }
    /*
     * setup new interrupt cycle only if not to be stopped
     */
    DataBufferControl.DataBufferNextInPointer = &DataBufferControl.DataBuffer[0];
    DataBufferControl.DataBufferNextDrawPointer = &DataBufferControl.DataBuffer[0];
    DataBufferControl.DataBufferNextDrawIndex = 0;
    MeasurementControl.IntegrateValueForAverage = 0;
    DataBufferControl.DataBufferFull = false;
    /*
     * Timebase
     */
    uint8_t tTimebaseIndex = MeasurementControl.TimebaseIndex;
    if (tTimebaseIndex < TIMEBASE_NUMBER_OF_FAST_MODES) {
        MeasurementControl.AcquisitionFastMode = true;
    } else {
        MeasurementControl.AcquisitionFastMode = false;
    }
    /*
     * get hardware prescale value
     */
    if (tTimebaseIndex < TIMEBASE_NUMBER_OF_FAST_PRESCALE) {
        MeasurementControl.TimebaseHWValue = ADCPrescaleValueforTimebase[tTimebaseIndex];
    } else {
        MeasurementControl.TimebaseHWValue = ADC_PRESCALE_MAX_VALUE;
    }

    MeasurementControl.TriggerStatus = TRIGGER_STATUS_START;
    // fast mode checks the INT1 pin directly and need no interrupt
    if (MeasurementControl.TriggerMode == TRIGGER_MODE_EXTERN && !MeasurementControl.AcquisitionFastMode) {
        /*
         * wait for external trigger with INT1 pin change interrupt - NO timeout
         */
        if (MeasurementControl.TriggerSlopeRising) {
            EICRA = _BV(ISC01) | _BV(ISC00);
        } else {
            EICRA = _BV(ISC01);
        }

        // clear interrupt bit
        EIFR = _BV(INTF0);
        // enable interrupt on next change
        EIMSK = _BV(INT0);
        return;
    } else {
        // start with waiting for triggering condition
        MeasurementControl.TriggerSampleCountDividedBy256 = 0;
    }

    /*
     * Start acquisition in free running mode for trigger detection
     */
    ADCSRB = 0; // free running mode
    if (MeasurementControl.AcquisitionFastMode) {
        // NO Interrupt in FastMode
        ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADIF) | MeasurementControl.TimebaseHWValue;
    } else {
        //  enable ADC interrupt, start with fast free running mode for trigger search.
        ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADIF) | ADC_PRESCALE_FOR_TRIGGER_SEARCH | _BV(ADIE);
    }
}

/*
 * ISR for external trigger input
 * not used if MeasurementControl.AcquisitionFastMode == true
 */
ISR(INT0_vect) {
    /*
     * Disable interrupt on trigger pin
     */
    EIMSK = 0;

    if (MeasurementControl.TriggerDelayMode != TRIGGER_DELAY_NONE) {
        /*
         * Delay
         */
        if (MeasurementControl.TriggerDelayMode == TRIGGER_DELAY_MICROS) {
            // It is not possible to access using internal delay handling of ISR, so duplicate code here
            // delayMicroseconds(MeasurementControl.TriggerDelayMillisOrMicros - TRIGGER_DELAY_MICROS_ISR_ADJUST_COUNT); substituted by code below, to enable full 16 bit range
            uint16_t tDelayMicros = MeasurementControl.TriggerDelayMillisOrMicros - TRIGGER_DELAY_MICROS_ISR_ADJUST_COUNT;
            asm volatile (
                    "1: sbiw %0,1" "\n\t" // 2 cycles
                    "1: adiw %0,1" "\n\t"// 2 cycles
                    "1: sbiw %0,1" "\n\t"// 2 cycles
                    "1: adiw %0,1" "\n\t"// 2 cycles
                    "1: sbiw %0,1" "\n\t"// 2 cycles
                    "1: adiw %0,1" "\n\t"// 2 cycles
                    "1: sbiw %0,1" "\n\t"// 2 cycles
                    "brne .-16" : : "w" (tDelayMicros)// 16 cycles
            );
        } else {
            MeasurementControl.TriggerStatus = TRIGGER_STATUS_FOUND_AND_WAIT_FOR_DELAY;
            MeasurementControl.TriggerDelayMillisEnd = millis() + MeasurementControl.TriggerDelayMillisOrMicros;
            return;
        }
    }

    /*
     * Start acquisition in free running mode as for trigger detection
     */
    MeasurementControl.TriggerStatus = TRIGGER_STATUS_FOUND;
    //  enable ADC interrupt
    ADCSRA = _BV(ADEN) | _BV(ADSC) | _BV(ADATE) | _BV(ADIF) | ADC_PRESCALE_FOR_TRIGGER_SEARCH | _BV(ADIE);
}

/*
 * Fast ADC read routine for timebase 101-201us and ultra fast for 10-50us
 * Value, which mets trigger condition, is taken as first data.
 *
 * Only TRIGGER_MODE_MANUAL_TIMEOUT and TRIGGER_MODE_EXTERN is supported yet.
 * But TRIGGER_MODE_EXTERN has timeout here!
 */
//#define DEBUG_ADC_TIMING
void acquireDataFast(void) {
    /**********************************
     * wait for triggering condition
     **********************************/
    Myword tUValue;
    uint8_t tTriggerStatus = TRIGGER_STATUS_START;
    uint16_t i;
    uint16_t tValueOffset = MeasurementControl.OffsetValue;

    if (MeasurementControl.TriggerMode == TRIGGER_MODE_EXTERN) {
        if (MeasurementControl.TriggerSlopeRising) {
            EICRA = _BV(ISC01) | _BV(ISC00);
        } else {
            EICRA = _BV(ISC01);
        }
        EIFR = _BV(INTF0); // Do not enable interrupt since we do polling below
        uint16_t tTimeoutCounter = 0;
        /*
         * Wait 65536 loops for external trigger to happen (for interrupt bit to be set)
         */
        do {
            tTimeoutCounter--;
        } while (bit_is_clear(EIFR, INTF0) && tTimeoutCounter != 0);
        // get first value after trigger
        tUValue.byte.LowByte = ADCL;
        tUValue.byte.HighByte = ADCH;

    } else {
        // start the first conversion and clear bit to recognize next conversion has finished
        ADCSRA |= _BV(ADIF) | _BV(ADSC);

        TIMSK2 = 0; // disable timer2 (millis()) interrupt to avoid jitter and signal dropouts

        /*
         * Wait for trigger for max. 10 screens e.g. < 20 ms
         * if trigger condition not met it will run forever in single shot mode
         */
        for (i = TRIGGER_WAIT_NUMBER_OF_SAMPLES; i != 0 || MeasurementControl.isSingleShotMode; --i) {
#if defined(DEBUG_ADC_TIMING)
            digitalWriteFast(DEBUG_PIN, HIGH); // debug pulse is 1 us for (ultra fast) PRESSCALER4 and 4 us for (fast) PRESSCALER8
#endif
            // wait for free running conversion to finish
            loop_until_bit_is_set(ADCSRA, ADIF);
#if defined(DEBUG_ADC_TIMING)
            digitalWriteFast(DEBUG_PIN, LOW);
#endif
            // Get value
            tUValue.byte.LowByte = ADCL;
            tUValue.byte.HighByte = ADCH;
            ADCSRA |= _BV(ADIF); // clear bit to recognize next conversion has finished

            /*
             * detect trigger slope
             */
            if (MeasurementControl.TriggerSlopeRising) {
                if (tTriggerStatus == TRIGGER_STATUS_START) {
                    // rising slope - wait for value below hysteresis level
                    if (tUValue.Word < MeasurementControl.RawTriggerLevelHysteresis) {
                        tTriggerStatus = TRIGGER_STATUS_AFTER_HYSTERESIS;
                    }
                } else {
                    // rising slope - wait for value to rise above trigger level
                    if (tUValue.Word > MeasurementControl.RawTriggerLevel) {
                        break;
                    }
                }
            } else {
                if (tTriggerStatus == TRIGGER_STATUS_START) {
                    // falling slope - wait for value above hysteresis level
                    if (tUValue.Word > MeasurementControl.RawTriggerLevelHysteresis) {
                        tTriggerStatus = TRIGGER_STATUS_AFTER_HYSTERESIS;
                    }
                } else {
                    // falling slope - wait for value to go below trigger level
                    if (tUValue.Word < MeasurementControl.RawTriggerLevel) {
                        break;
                    }
                }
            }
        }
    }

    /*
     * Only microseconds delay makes sense here
     */
    if (MeasurementControl.TriggerDelayMode == TRIGGER_DELAY_MICROS) {
        delayMicroseconds(MeasurementControl.TriggerDelayMillisOrMicros - TRIGGER_DELAY_MICROS_POLLING_ADJUST_COUNT);
        ADCSRA |= _BV(ADIF);
        loop_until_bit_is_set(ADCSRA, ADIF);

        // get first value after delay
        tUValue.byte.LowByte = ADCL;
        tUValue.byte.HighByte = ADCH;
        ADCSRA |= _BV(ADIF); // clear bit to recognize next conversion has finished
    }

    MeasurementControl.TriggerStatus = TRIGGER_STATUS_FOUND; // for single shot mode

    /********************************
     * read a buffer of data here
     ********************************/
// setup for min, max, average
    uint16_t tValueMax = tUValue.Word;
    uint16_t tValueMin = tUValue.Word;

    uint8_t tIndex = MeasurementControl.TimebaseIndex;
    uint8_t *DataPointerFast = &DataBufferControl.DataBuffer[0];
    uint16_t tLoopCount = DataBufferControl.AcquisitionSize;

    if (tIndex <= TIMEBASE_INDEX_ULTRAFAST_MODES) {
        /*
         * 10-50us range. Data is stored directly as 16 bit value and not processed.
         * => we have only half number of samples (tLoopCount)
         */
        if (DATABUFFER_SIZE < REMOTE_DISPLAY_WIDTH * 2) {
            // In this configuration (for testing etc.) we have not enough space
            // for DISPLAY_WIDTH 16 bit-values which need (REMOTE_DISPLAY_WIDTH * 2) bytes.
            // The compiler will remove the code if the configuration does not hold ;-)
            tLoopCount = DATABUFFER_SIZE / 2;
        } else if (tLoopCount > REMOTE_DISPLAY_WIDTH) {
            // Last measurement before stop, fill the whole buffer with 16 bit values
            // During running measurements we know here (see if above) that we can get REMOTE_DISPLAY_WIDTH 16 bit values
            tLoopCount = tLoopCount / 2;
        }
        *DataPointerFast++ = tUValue.byte.LowByte;
        *DataPointerFast++ = tUValue.byte.HighByte;
        /*
         * do very fast reading without processing
         */
        for (i = tLoopCount; i > 1; --i) {
            uint8_t tLow, tHigh;
#if defined(DEBUG_ADC_TIMING)
            digitalWriteFast(DEBUG_PIN, HIGH); // debug pulse is 1.6 us
#endif
            loop_until_bit_is_set(ADCSRA, ADIF);
#if defined(DEBUG_ADC_TIMING)
            digitalWriteFast(DEBUG_PIN, LOW);
#endif
            tLow = ADCL;
            tHigh = ADCH;
            ADCSRA |= _BV(ADIF);
            *DataPointerFast++ = tLow;
            *DataPointerFast++ = tHigh;
        }
        interrupts();
        DataPointerFast = &DataBufferControl.DataBuffer[0];
        tUValue.byte.LowByte = *DataPointerFast++;
        tUValue.byte.HighByte = *DataPointerFast++;
    } // (tIndex <= TIMEBASE_INDEX_ULTRAFAST_MODES)

    /*
     * Data is processed here.
     * For fast mode new data is read from ADC,
     * for ultra fast mode data is read from buffer.
     */
    uint32_t tIntegrateValue = 0;
    uint8_t *DataPointer = &DataBufferControl.DataBuffer[0]; // ca. 1064 / 0x428
    for (i = tLoopCount; i > 0; --i) {
        /*
         * process (first) value
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
        tUValue.Word = tUValue.Word >> MeasurementControl.ShiftValue;
        // byte overflow? This can happen if autorange is disabled.
        if (tUValue.Word >= 0X100) {
            tUValue.Word = 0xFF;
        }
        // now value is a byte and fits to screen
        *DataPointer++ = DISPLAY_VALUE_FOR_ZERO - tUValue.byte.LowByte;

        /*
         * get next value. Only for fast mode!
         */
        if (tIndex > TIMEBASE_INDEX_ULTRAFAST_MODES) {
            // 101-201us range 13 us conversion time
            // get values from ADC
            // wait for free running conversion to finish
            // ADCSRA here is E5
#if defined(DEBUG_ADC_TIMING)
            digitalWriteFast(DEBUG_PIN, HIGH); // debug pulse is 2.2 us
#endif
            loop_until_bit_is_set(ADCSRA, ADIF);
#if defined(DEBUG_ADC_TIMING)
            digitalWriteFast(DEBUG_PIN, LOW);
#endif
            // ADCSRA here is F5
            // duration: get Value included min 1,2 micros
            tUValue.byte.LowByte = ADCL;
            tUValue.byte.HighByte = ADCH;
            ADCSRA |= _BV(ADIF); // clear bit to recognize next conversion has finished
            //ADCSRA here is E5
        } else {
            // get values from ultra fast buffer
            tUValue.byte.LowByte = *DataPointerFast++;
            tUValue.byte.HighByte = *DataPointerFast++;
        }
    }

    ADCSRA &= ~_BV(ADATE); // Disable auto-triggering
    MeasurementControl.RawValueMax = tValueMax;
    MeasurementControl.RawValueMin = tValueMin;
    if (tIndex <= TIMEBASE_INDEX_ULTRAFAST_MODES && tLoopCount > REMOTE_DISPLAY_WIDTH) {
        // compensate for half sample count in last measurement in ultra fast mode
        tIntegrateValue *= 2;
        // set remaining of buffer to zero
        while (DataPointer < &DataBufferControl.DataBuffer[DATABUFFER_SIZE]) {
            *DataPointer++ = 0;
        }
    }
    MeasurementControl.IntegrateValueForAverage = tIntegrateValue;
    DataBufferControl.DataBufferFull = true;
}

// Dummy ISR - only to reset Interrupt flag
ISR(TIMER0_COMPA_vect) {
}

/*
 * Interrupt service routine for adc interrupt
 * used only for "slow" mode >=496us/div because ISR overhead is to much for fast mode
 * app. 7 microseconds + 2 for push + 2 for pop
 * app. 2 microseconds for trigger search + 2 for push + 2 for pop
 * 7 cycles before entering
 * 4 cycles RETI
 * ADC is free running for trigger phase, where ADC runs with PRESCALE16 (as for 496 us range)
 * First value, which mets trigger condition, is taken as first data.
 */
//#define DEBUG_ISR_TIMING
ISR(ADC_vect) {
// 7++ for jump to ISR
    // 3 + 10 pushes (r18 - r31) + in + eor = 28 cycles
// 35++ cycles to get here
#if defined(DEBUG_ISR_TIMING)
    digitalWriteFast(DEBUG_PIN, HIGH);
#endif

    Myword tUValue;
    tUValue.byte.LowByte = ADCL;
    tUValue.byte.HighByte = ADCH;

    if (MeasurementControl.TriggerStatus != TRIGGER_STATUS_FOUND) {
        bool tTriggerFound = false;
        /*
         * Trigger detection here
         */
        uint8_t tTriggerStatus = MeasurementControl.TriggerStatus;

        if (MeasurementControl.TriggerSlopeRising) {
            if (tTriggerStatus == TRIGGER_STATUS_START) {
                // rising slope - wait for value below hysteresis level
                if (tUValue.Word < MeasurementControl.RawTriggerLevelHysteresis) {
                    MeasurementControl.TriggerStatus = TRIGGER_STATUS_AFTER_HYSTERESIS;
                }
            } else {
                // rising slope - wait for value to rise above trigger level
                if (tUValue.Word > MeasurementControl.RawTriggerLevel) {
                    // start reading into buffer
                    tTriggerFound = true;
                }
            }
        } else {
            if (tTriggerStatus == TRIGGER_STATUS_START) {
                // falling slope - wait for value above hysteresis level
                if (tUValue.Word > MeasurementControl.RawTriggerLevelHysteresis) {
                    MeasurementControl.TriggerStatus = TRIGGER_STATUS_AFTER_HYSTERESIS;
                }
            } else {
                // falling slope - wait for value to go below trigger level
                if (tUValue.Word < MeasurementControl.RawTriggerLevel) {
                    // start reading into buffer
                    tTriggerFound = true;
                }
            }
        }

        if (!tTriggerFound) {
#if defined(DEBUG_ISR_TIMING)
            digitalWriteFast(DEBUG_PIN, LOW);
#endif
            if (MeasurementControl.TriggerMode == TRIGGER_MODE_MANUAL || MeasurementControl.isSingleShotMode
                    || MeasurementControl.TriggerDelayMode != TRIGGER_DELAY_NONE) {
                // no timeout for Manual Trigger or SingleShotMode or trigger delay mode -> return
                MeasurementControl.ValueBeforeTrigger = tUValue.Word;
                return;
            }
            /*
             * Trigger timeout handling. Always timeout in free running mode
             */
            if (MeasurementControl.TriggerMode != TRIGGER_MODE_FREE) {
                MeasurementControl.TriggerSampleCountPrecaler++;
                if (MeasurementControl.TriggerSampleCountPrecaler != 0) {
                    return;
                } else {
                    MeasurementControl.TriggerSampleCountDividedBy256++;
                    if (MeasurementControl.TriggerSampleCountDividedBy256 < MeasurementControl.TriggerTimeoutSampleCount) {
                        /*
                         * Trigger condition not met and timeout not reached
                         */
                        return;
                    }
                }
            }
        } else {
            /*
             * Trigger found (or timeout reached) , check for delay
             */
            if (MeasurementControl.TriggerDelayMode != TRIGGER_DELAY_NONE) {
                if (MeasurementControl.TriggerDelayMode == TRIGGER_DELAY_MICROS) {
                    TIMSK2 = 0; // disable timer2 (millis()) interrupt to avoid jitter
                    // enable RX interrupt for GUI, otherwise we could miss input bytes at long delays (eg.50000us)
                    uint16_t tDelayMicros = MeasurementControl.TriggerDelayMillisOrMicros;
                    // delayMicroseconds(tDelayMicros); substituted by code below, to avoid additional register pushes
                    asm volatile (
                            "1: sbiw %0,1" "\n\t" // 2 cycles
                            "1: adiw %0,1" "\n\t"// 2 cycles
                            "1: sbiw %0,1" "\n\t"// 2 cycles
                            "1: adiw %0,1" "\n\t"// 2 cycles
                            "1: sbiw %0,1" "\n\t"// 2 cycles
                            "1: adiw %0,1" "\n\t"// 2 cycles
                            "1: sbiw %0,1" "\n\t"// 2 cycles
                            "brne .-16" : : "w" (tDelayMicros)// 16 cycles
                    );

                    TIMSK2 = _BV(TOIE2); // Enable overflow interrupts which replaces the Arduino millis() interrupt
                    // get a new value since ADC is already free running here
                    tUValue.byte.LowByte = ADCL;
                    tUValue.byte.HighByte = ADCH;
                } else {
                    // needs additional register pushes
                    // MeasurementControl.TriggerDelayMillisEnd = millis() + MeasurementControl.TriggerDelayMillisOrMicros;
                    MeasurementControl.TriggerDelayMillisEnd = 0; // signal main loop to initialize value
                    MeasurementControl.TriggerStatus = TRIGGER_STATUS_FOUND_AND_WAIT_FOR_DELAY;
                    ADCSRA &= ~_BV(ADIE); // disable ADC interrupt -> Main loop will handle delay
                    return;
                }
            }
        }

        /*
         * External Trigger or trigger found and delay passed
         */

        /*
         * Problem is: next conversion has already started with old fast prescaler.
         * Changing prescaler has only effect of the last 4 to 5 conversion cycles since 5 to 6 are already done :-(
         * So just speed up ongoing conversion by setting prescaler to 2, disable interrupt and auto trigger and wait for end.
         *
         * 4 us ISR delay + code until here plus max 2 fast conversions (13 us)
         * or plus minimal 1 fast conversion (6 us) since real trigger event
         * => 10 to 17 us from real trigger condition to here.
         * Ideally it would be one timer0 clock e.g. 16us for 496us range, 32us for 1ms range etc.
         */
        ADCSRA = _BV(ADEN) | _BV(ADIF); // => 10 to 12 clocks until conversion finishes

        /*
         * disable millis() interrupt and initialize max and min
         */
        // 2 clock cycles
        TIMSK2 = 0; // disable timer2 (millis() interrupts to avoid jitter. Enable at main loop on buffer full

        // 11 clock cycles
        MeasurementControl.TriggerStatus = TRIGGER_STATUS_FOUND;
        MeasurementControl.ValueMaxForISR = tUValue.Word;
        MeasurementControl.ValueMinForISR = tUValue.Word;

        ADCSRB = _BV(ADTS0) | _BV(ADTS1); // Trigger source Timer/Counter0 Compare Match A. 3 cycles
        // set timer to initial state
        TCNT0 = 0; // 1 cycle
        TIFR0 = _BV(OCF0A); // reset int flag. 1 cycle

        // No need to wait for last self triggered conversion to end
//        while (bit_is_clear(ADCSRA, ADIF)) {
//            ;
//        }

        /*
         * variable delay
         */
        if (MeasurementControl.TimebaseIndex < 6) {
            // start new conversion
            ADCSRA = _BV(ADEN) | _BV(ADATE) | _BV(ADSC) | _BV(ADIF) | MeasurementControl.TimebaseHWValue | _BV(ADIE);
            // proceed and take trigger value as first data, since the interrupt request of conversion above is cancelled.
        } else if (MeasurementControl.TimebaseIndex < 8) {
            uint16_t a4Microseconds = 15 * 4;
            /*
             * wait 15 micros for TimebaseIndex == 6 and 47 for TimebaseIndex == 7
             */
            if (MeasurementControl.TimebaseIndex == 7) {
                a4Microseconds = 47 * 4;
            }
            // the following loop takes 4 cycles (1/4 microseconds  at 16 MHz) per iteration
            __asm__ __volatile__ (
                    "1: sbiw %0,1" "\n\t"    // 2 cycles
                    "brne .-4" : "=w" (a4Microseconds) : "0" (a4Microseconds)// 2 cycles
            );
            TCNT0 = 0; // 1 cycle
            TIFR0 = _BV(OCF0A); // reset int flag. 1 cycle
            // start new conversion
            ADCSRA = _BV(ADEN) | _BV(ADATE) | _BV(ADSC) | _BV(ADIF) | MeasurementControl.TimebaseHWValue | _BV(ADIE);
        } else {
            // delay of greater (160 - (10 to 17)) was required so just ignore this ADC value and take next value as first one
            ADCSRA = _BV(ADEN) | _BV(ADATE) | _BV(ADSC) | _BV(ADIF) | MeasurementControl.TimebaseHWValue | _BV(ADIE);
#if defined(DEBUG_ISR_TIMING)
            digitalWriteFast(DEBUG_PIN, LOW);
#endif
            return;
        }

        /*
         * Start new conversion of ADC with new timing. Change ADC prescaler and change trigger source for free running mode triggered by timer0.
         */

        ADCSRA = _BV(ADEN) | _BV(ADATE) | _BV(ADSC) | _BV(ADIF) | MeasurementControl.TimebaseHWValue | _BV(ADIE);
        // proceed and take trigger value as first data, since the interrupt request of conversion above is cancelled.
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

    uint8_t *tDataBufferPointer = DataBufferControl.DataBufferNextInPointer;
    /*
     * c code (see line below) needs 5 register more (to push and pop)#
     * so do it with assembler and 1 additional register (for high byte :-()
     */
//  MeasurementControl.IntegrateValueForAverage += tUValue.Word;
    __asm__ (
            /* could use __tmp_reg__ as synonym for r24 */
            /* add low byte */
            "ld     r24, Z  ; \n"
            "add    r24, %[lowbyte] ; \n"
            "st     Z+, r24 ; \n"
            /* add high byte with carry */
            "ld     r24, Z  ; \n"
            "adc    r24, %[highbyte] ; \n"
            "st     Z+, r24 ; \n"
            /* add carry */
            "ld     r24, Z  ; \n"
            "adc    r24, __zero_reg__ ; \n"
            "st     Z+, r24 ; \n"
            /* add carry */
            "ld     r24, Z  ; \n"
            "adc    r24, __zero_reg__ ; \n"
            "st     Z+, r24 ; \n"
            :/*no output*/
            : [lowbyte] "r" (tUValue.byte.LowByte), [highbyte] "r" (tUValue.byte.HighByte), "z" (&MeasurementControl.IntegrateValueForAverage)
            : "r24"
    );
    /***************************************************
     * transform 10 bit value in order to fit on screen
     ***************************************************/
    if (tUValue.Word < MeasurementControl.OffsetValue) {
        tUValue.Word = 0;
    } else {
        tUValue.Word = tUValue.Word - MeasurementControl.OffsetValue;
    }
    tUValue.Word = tUValue.Word >> MeasurementControl.ShiftValue;
    // byte overflow? This can happen if autorange is disabled.
    if (tUValue.byte.HighByte > 0) {
        tUValue.byte.LowByte = 0xFF;
    }
    // store display byte value
    *tDataBufferPointer++ = DISPLAY_VALUE_FOR_ZERO - tUValue.byte.LowByte;
    // detect end of buffer
    if (tDataBufferPointer > DataBufferControl.DataBufferEndPointer) {
        // stop acquisition
        ADCSRA &= ~_BV(ADIE); // disable ADC interrupt
// copy max and min values of measurement for display purposes
        MeasurementControl.RawValueMin = MeasurementControl.ValueMinForISR;
        MeasurementControl.RawValueMax = MeasurementControl.ValueMaxForISR;

        /*
         * signal to main loop that acquisition ended
         * Main loop is responsible to start a new acquisition via call of startAcquisition();
         */
        DataBufferControl.DataBufferFull = true;
    }
    DataBufferControl.DataBufferNextInPointer = tDataBufferPointer;

#if defined(DEBUG_ISR_TIMING)
    digitalWriteFast(DEBUG_PIN, LOW);
#endif
}

/***********************************************************************
 * Measurement auto control stuff (trigger, range + offset)
 ***********************************************************************/
/*
 * Clears and sets grid and label too.
 * sets:
 * OffsetValue
 * HorizontalGridSizeShift8
 * HorizontalGridVoltage
 * needs:
 * VCC
 */
void setInputRange(uint8_t aShiftValue, uint8_t aActiveAttenuatorValue) {
    MeasurementControl.ShiftValue = aShiftValue;
    if (MeasurementControl.ChannelHasActiveAttenuator) {
        setAttenuator(aActiveAttenuatorValue);
    }
    resetOffset();

    if (MeasurementControl.isRunning && DisplayControl.DisplayPage == DISPLAY_PAGE_CHART) {
        //clear old grid, since it will be changed
        BlueDisplay1.clearDisplay();
    }
    float tNewGridVoltage;
    uint16_t tHorizontalGridSizeShift8;

    if (MeasurementControl.ADCReference == DEFAULT) {
        /*
         * 5 volt reference
         */
        if (aShiftValue == 0) {
            // formula for 5 volt and 0.2 volt / div and shift 0 is: ((1023/5 volt)*0.2) * 256/2^shift = 52377.6 / 5
            tHorizontalGridSizeShift8 = (52377.6 / MeasurementControl.VCC);
            tNewGridVoltage = 0.2;
        } else {
            // formula for 5 volt and 0.5 volt / div and shift 1 is: ((1023/5 volt)*0.5) * 256/2^shift = (130944/2) / 5
            tHorizontalGridSizeShift8 = (65472.0 / MeasurementControl.VCC);
            tNewGridVoltage = 0.5 * aShiftValue;
        }
    } else {
        /*
         * 1.1 volt reference
         */
        if (MeasurementControl.ChannelHasActiveAttenuator) {
            tHorizontalGridSizeShift8 = HORIZONTAL_GRID_HEIGHT_2V_SHIFT8;
        } else {
            tHorizontalGridSizeShift8 = HORIZONTAL_GRID_HEIGHT_1_1V_SHIFT8;
        }
        uint8_t tFactor = 1 << aShiftValue;
        tNewGridVoltage = 0.05 * tFactor;
    }
    MeasurementControl.HorizontalGridSizeShift8 = tHorizontalGridSizeShift8;
    MeasurementControl.HorizontalGridVoltage = tNewGridVoltage * getAttenuatorFactor();

    if (MeasurementControl.isRunning) {
        drawGridLinesWithHorizLabelsAndTriggerLine();
    }
}

/*
 * used by swipe handler
 * input Range is ShiftValue + 3 * AttenuatorValue
 */
uint8_t changeRange(int8_t aChangeAmount) {
    uint8_t tFeedbackType = FEEDBACK_TONE_OK;
    int8_t tNewValue = 0;
    if (MeasurementControl.AttenuatorType >= ATTENUATOR_TYPE_ACTIVE_ATTENUATOR) {
        tNewValue = MeasurementControl.AttenuatorValue * 3;
    }
    tNewValue += MeasurementControl.ShiftValue + aChangeAmount;
    if (tNewValue < 0) {
        tNewValue = 0;
        tFeedbackType = FEEDBACK_TONE_ERROR;
    }
    if (MeasurementControl.ChannelHasActiveAttenuator) {
        if (tNewValue > 8) {
            tNewValue = 8;
            tFeedbackType = FEEDBACK_TONE_ERROR;
        }
    } else {
        if (tNewValue > 2) {
            tNewValue = 2;
            tFeedbackType = FEEDBACK_TONE_ERROR;
        }
    }
    setInputRange(tNewValue % 3, tNewValue / 3);
    return tFeedbackType;
}

/*
 * makes only sense if external attenuator attached
 * returns true, if range was changed
 */
bool checkRAWValuesForClippingAndChangeRange(void) {
    if (MeasurementControl.ChannelHasActiveAttenuator) {
        if (MeasurementControl.RangeAutomatic) {
            // Check for clipping (check ADC_MAX_CONVERSION_VALUE and also 0 if AC mode)
            if ((MeasurementControl.RawValueMax == ADC_MAX_CONVERSION_VALUE
                    || (MeasurementControl.ChannelIsACMode && MeasurementControl.RawValueMin == 0))
                    && MeasurementControl.AttenuatorValue < 2) {
                setInputRange(0, MeasurementControl.AttenuatorValue + 1);
                return true;
            }
        }
    }
    return false;
}

void computeAutoRange(void) {
    if (!MeasurementControl.RangeAutomatic) {
        return;
    }
//First check for clipping - check ADC_MAX_CONVERSION_VALUE and also zero if AC mode
    if (checkRAWValuesForClippingAndChangeRange()) {
        return;
    }

// get relevant peak2peak value
    int16_t tPeakToPeak = MeasurementControl.RawValueMax;
    if (MeasurementControl.ChannelIsACMode) {
        tPeakToPeak -= MeasurementControl.RawDSOReadingACZero;
        // check for zero offset error with tPeakToPeak < 0
        if (tPeakToPeak < 0
                || ((int16_t) MeasurementControl.RawDSOReadingACZero - (int16_t) MeasurementControl.RawValueMin) > tPeakToPeak) {
            //difference between zero and min is greater than difference between zero and max => min determines the range
            tPeakToPeak = MeasurementControl.RawDSOReadingACZero - MeasurementControl.RawValueMin;
        }
        // since tPeakToPeak must fit in half of display
        tPeakToPeak *= 2;
    } else if (MeasurementControl.OffsetMode == OFFSET_MODE_AUTOMATIC) {
        // Value min(display) is NOT fixed at zero
        tPeakToPeak -= MeasurementControl.RawValueMin;
    }

    /*
     * set automatic range
     */
    uint8_t tOldValueShift = MeasurementControl.ShiftValue;
    uint8_t tNewValueShift = 0;
    uint8_t tNewAttenuatorValue = MeasurementControl.AttenuatorValue;

    bool tAttChanged = false; // no change
// ignore warnings since we know that now tPeakToPeak is positive
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
    if (tPeakToPeak >= REMOTE_DISPLAY_HEIGHT * 2) {
        tNewValueShift = 2;
    } else if (tPeakToPeak >= REMOTE_DISPLAY_HEIGHT) {
#pragma GCC diagnostic pop
        tNewValueShift = 1;
    } else if (MeasurementControl.AttenuatorValue > 0 && MeasurementControl.AttenuatorType >= ATTENUATOR_TYPE_ACTIVE_ATTENUATOR) {
        /*
         * only max value is relevant for attenuator switching!
         */
        if (MeasurementControl.RawValueMax < ((REMOTE_DISPLAY_HEIGHT - (REMOTE_DISPLAY_HEIGHT / 10)) * 4) / ATTENUATOR_FACTOR) {
            // more than 10 percent below theoretical threshold -> switch attenuator to higher resolution
            tNewAttenuatorValue--;
            tNewValueShift = 2;
            tAttChanged = true;
        }
    }

    if (tNewValueShift < tOldValueShift || tAttChanged) {
        /*
         * wait n-milliseconds before switch to higher resolution (lower index)
         */
        uint32_t tActualMillis = millis();
        if (tActualMillis - MeasurementControl.TimestampLastRangeChange > SCALE_CHANGE_DELAY_MILLIS) {
            MeasurementControl.TimestampLastRangeChange = tActualMillis;
            setInputRange(tNewValueShift, tNewAttenuatorValue);
        }
    } else if (tNewValueShift > tOldValueShift) {
        setInputRange(tNewValueShift, tNewAttenuatorValue);
    } else {
        // reset "delay"
        MeasurementControl.TimestampLastRangeChange = millis();
    }
}

/**
 * compute offset based on center value in order display values at center of screen
 */
void computeAutoOffset(void) {
    if (MeasurementControl.OffsetMode != OFFSET_MODE_AUTOMATIC) {
        return;
    }
    uint16_t tValueMiddleY = MeasurementControl.RawValueMin
            + ((MeasurementControl.RawValueMax - MeasurementControl.RawValueMin) / 2);

    uint16_t tRawValuePerGrid = MeasurementControl.HorizontalGridSizeShift8 >> (8 - MeasurementControl.ShiftValue);
    uint16_t tLinesPerHalfDisplay = 4;
    if (MeasurementControl.HorizontalGridSizeShift8 > 10000) {
        tLinesPerHalfDisplay = 2;
    }
// take next multiple of HorizontalGridSize which is smaller than tValueMiddleY
    int16_t tNumberOfGridLinesToSkip = (tValueMiddleY / tRawValuePerGrid) - tLinesPerHalfDisplay; // adjust to bottom of display (minus 3 lines)
    if (tNumberOfGridLinesToSkip < 0) {
        tNumberOfGridLinesToSkip = 0;
    }
    if (abs(MeasurementControl.OffsetGridCount - tNumberOfGridLinesToSkip) > 1) {
        // avoid jitter by not changing number if its delta is only 1
        BlueDisplay1.clearDisplay();
        MeasurementControl.OffsetValue = tNumberOfGridLinesToSkip * tRawValuePerGrid;
        MeasurementControl.OffsetGridCount = tNumberOfGridLinesToSkip;
        drawGridLinesWithHorizLabelsAndTriggerLine();
    } else if (tNumberOfGridLinesToSkip == 0) {
        MeasurementControl.OffsetValue = 0;
        MeasurementControl.OffsetGridCount = 0;
    }
}

/*
 * sets offset to right value for AC or DC mode
 */
void resetOffset(void) {
    if (MeasurementControl.ChannelIsACMode) {
        // Adjust zero offset for small display ranges
        uint16_t tNewValueOffsetForACMode = MeasurementControl.RawDSOReadingACZero / 2 + MeasurementControl.RawDSOReadingACZero / 4;
        if (MeasurementControl.ShiftValue == 1) {
            tNewValueOffsetForACMode = MeasurementControl.RawDSOReadingACZero / 2;
        } else if (MeasurementControl.ShiftValue == 2) {
            tNewValueOffsetForACMode = 0;
        }
        MeasurementControl.OffsetValue = tNewValueOffsetForACMode;
    } else if (MeasurementControl.OffsetMode != OFFSET_MODE_AUTOMATIC) {
        MeasurementControl.OffsetValue = 0;
    }
}

/***********************************************************************
 * Attenuator support stuff
 ***********************************************************************/

void setAttenuator(uint8_t aNewValue) {
    MeasurementControl.AttenuatorValue = aNewValue;
    uint8_t tPortValue = CONTROL_PORT;  //
    tPortValue &= ~ATTENUATOR_MASK;
    tPortValue |= ((aNewValue << ATTENUATOR_SHIFT) & ATTENUATOR_MASK);
    CONTROL_PORT = tPortValue;
}

uint16_t getAttenuatorFactor(void) {
    uint16_t tRetValue = 1;
//    if (MeasurementControl.ChannelHasAC_DCSwitch) {
    for (int i = 0; i < MeasurementControl.AttenuatorValue; ++i) {
        tRetValue *= ATTENUATOR_FACTOR;
    }
//    }
    return tRetValue;
}

/************************************************************************
 * BUTTON handler section
 ************************************************************************/
/*
 * toggle between DC and AC mode
 */
void doAcDcMode(__attribute__((unused))     BDButton *aTheTouchedButton, __attribute__((unused))     int16_t aValue) {
    setACMode(!MeasurementControl.ChannelIsACMode);
}

/*
 * Handler for number receive event - set delay to value
 */
void doSetTriggerDelay(float aValue) {
    uint8_t tTriggerDelayMode = TRIGGER_DELAY_NONE;
    if (aValue != NUMBER_INITIAL_VALUE_DO_NOT_SHOW) {
        uint32_t tTriggerDelay = aValue;
        if (tTriggerDelay != 0) {
            if (tTriggerDelay > UINT16_MAX) {
                tTriggerDelayMode = TRIGGER_DELAY_MILLIS;
                MeasurementControl.TriggerDelayMillisOrMicros = tTriggerDelay / 1000;
            } else {
                tTriggerDelayMode = TRIGGER_DELAY_MICROS;
                uint16_t tTriggerDelayMicros = tTriggerDelay;
                if (tTriggerDelayMicros > TRIGGER_DELAY_MICROS_ISR_ADJUST_COUNT) {
                    MeasurementControl.TriggerDelayMillisOrMicros = tTriggerDelayMicros;
                } else {
                    // delay to small -> disable delay
                    tTriggerDelayMode = TRIGGER_DELAY_NONE;
                }
            }
        }
    }
    MeasurementControl.TriggerDelayMode = tTriggerDelayMode;
    setTriggerDelayCaption();
}

/*
 * toggle between 5 and 1.1 volt reference
 */
void doADCReference(__attribute__((unused))     BDButton *aTheTouchedButton, __attribute__((unused))     int16_t aValue) {
    uint8_t tNewReference = MeasurementControl.ADCReference;
    if (MeasurementControl.ADCReference == DEFAULT) {
        tNewReference = INTERNAL;
    } else {
        tNewReference = DEFAULT;
    }
    setADCReference(tNewReference); // switch hardware
    setReferenceButtonCaption();
    if (!MeasurementControl.RangeAutomatic) {
        // set new grid values
        setInputRange(MeasurementControl.ShiftValue, MeasurementControl.AttenuatorValue);
    }
}

void doStartStopDSO(__attribute__((unused))     BDButton *aTheTouchedButton, __attribute__((unused))     int16_t aValue) {
    if (MeasurementControl.isRunning) {
        /*
         * Stop here
         * for the last measurement read full buffer size
         * Do this asynchronously to the interrupt routine by "StopRequested" in order to extend a running or started acquisition.
         * Stopping does not need to release the trigger condition except for TRIGGER_MODE_EXTERN since trigger always has a timeout.
         * Stop single shot mode by switching to regular mode (and then waiting for timeout)
         */
        DataBufferControl.DataBufferEndPointer = &DataBufferControl.DataBuffer[DATABUFFER_SIZE - 1];

        // - use noInterrupts() to avoid race conditions
        noInterrupts();
        if (MeasurementControl.TriggerStatus != TRIGGER_STATUS_FOUND) {
            if (MeasurementControl.TriggerMode == TRIGGER_MODE_EXTERN) {
                // Call the external trigger event routine by software for TRIGGER_MODE_EXTERN to start reading data.
                INT0_vect();
            } else if (MeasurementControl.TriggerMode == TRIGGER_MODE_MANUAL || MeasurementControl.isSingleShotMode) {
                // set status to trigger found to start reading data.
                MeasurementControl.TriggerStatus = TRIGGER_STATUS_FOUND;
            }
        }
        interrupts();

        if (MeasurementControl.StopRequested) {
            /*
             * Stop requested 2 times -> stop immediately
             */
            uint8_t *tEndPointer = DataBufferControl.DataBufferNextInPointer;
            DataBufferControl.DataBufferEndPointer = tEndPointer;
            // clear trailing buffer space not used
            memset(tEndPointer, 0xFF, ((uint8_t*) &DataBufferControl.DataBuffer[DATABUFFER_SIZE]) - ((uint8_t*) tEndPointer));
        }
        // return to continuous mode with stop requested
        MeasurementControl.StopRequested = true;
        // AcquisitionSize is used in synchronous fast loop so we can set it here
        DataBufferControl.AcquisitionSize = DATABUFFER_SIZE;
        DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[0];
        // no feedback tone, it kills the timing!
    } else {
        /*
         * Start here
         */
        BlueDisplay1.clearDisplay();
        DisplayControl.DisplayPage = DISPLAY_PAGE_CHART;
        //DisplayControl.showInfoMode = true;
        activateChartGui();
        drawGridLinesWithHorizLabelsAndTriggerLine();
        MeasurementControl.isSingleShotMode = false;
        startAcquisition();
        MeasurementControl.isRunning = true;
    }
}

/************************************************************************
 * Graphical output section
 ************************************************************************/
/*
 * returns false if display was scrolled
 */
uint8_t scrollChart(int aScrollAmount) {
    if (DisplayControl.DisplayPage != DISPLAY_PAGE_CHART) {
        return true;
    }
    bool isError = false;
    /*
     * set start of display in data buffer
     */
    DataBufferControl.DataBufferDisplayStart += aScrollAmount / DisplayControl.XScale;
// check against begin of buffer
    if (DataBufferControl.DataBufferDisplayStart < &DataBufferControl.DataBuffer[0]) {
        DataBufferControl.DataBufferDisplayStart = &DataBufferControl.DataBuffer[0];
        isError = true;
    } else {
        uint8_t *tMaxAddress = &DataBufferControl.DataBuffer[DATABUFFER_SIZE];
        if (MeasurementControl.TimebaseIndex < TIMEBASE_NUMBER_OF_FAST_MODES) {
            // Only half of data buffer is filled
            tMaxAddress = &DataBufferControl.DataBuffer[DATABUFFER_SIZE / 2];
        }
        tMaxAddress = tMaxAddress - (REMOTE_DISPLAY_WIDTH / DisplayControl.XScale);
        if (DataBufferControl.DataBufferDisplayStart > tMaxAddress) {
            DataBufferControl.DataBufferDisplayStart = tMaxAddress;
            isError = true;
        }
    }
    drawDataBuffer(DataBufferControl.DataBufferDisplayStart, COLOR_DATA_HOLD, COLOR_BACKGROUND_DSO);

    return isError;
}

void drawDataBuffer(uint8_t *aByteBuffer, uint16_t aColor, uint16_t aClearBeforeColor) {
    uint8_t tXScale = DisplayControl.XScale;
    uint8_t *tBufferPtr = aByteBuffer;
    if (tXScale > 1) {
        // expand - show value several times
        uint8_t *tDisplayBufferPtr = &DataBufferControl.DisplayBuffer[0];
        uint8_t tXScaleCounter = tXScale;
        uint8_t tValue = *tBufferPtr++;
        for (unsigned int i = 0; i < sizeof(DataBufferControl.DisplayBuffer); ++i) {
            if (tXScaleCounter == 0) {
                tValue = *tBufferPtr++;
                tXScaleCounter = tXScale;
            }
            tXScaleCounter--;
            *tDisplayBufferPtr++ = tValue;
        }
        tBufferPtr = &DataBufferControl.DisplayBuffer[0];
    }
    BlueDisplay1.drawChartByteBuffer(0, 0, aColor, aClearBeforeColor, tBufferPtr, sizeof(DataBufferControl.DisplayBuffer));
}

void clearDisplayedChart(uint8_t *aDisplayBufferPtr) {
    BlueDisplay1.drawChartByteBuffer(0, 0, COLOR_BACKGROUND_DSO, COLOR16_NO_BACKGROUND, aDisplayBufferPtr,
            sizeof(DataBufferControl.DisplayBuffer));
}

void clearDataBuffer() {
    memset(DataBufferControl.DataBuffer, 0, sizeof(DataBufferControl.DataBuffer));
}
/*
 * Draws only one chart value - used for drawing while sampling
 */
void drawRemainingDataBufferValues(void) {
// check if end of display buffer reached - required for last acquisition which uses the whole data buffer
    uint8_t tLastValueByte;
    uint8_t tNextValueByte;
    uint8_t tValueByte;
    uint16_t tBufferIndex = DataBufferControl.DataBufferNextDrawIndex;

    while (DataBufferControl.DataBufferNextDrawPointer < DataBufferControl.DataBufferNextInPointer
            && tBufferIndex < REMOTE_DISPLAY_WIDTH) {
        /*
         * clear old line
         */
        if (tBufferIndex < REMOTE_DISPLAY_WIDTH - 1) {
            // fetch next value and clear line in advance
            tValueByte = DataBufferControl.DisplayBuffer[tBufferIndex];
            tNextValueByte = DataBufferControl.DisplayBuffer[tBufferIndex + 1];
            BlueDisplay1.drawLineFastOneX(tBufferIndex, tValueByte, tNextValueByte, DisplayControl.EraseColor);
        }

        /*
         * get new value
         */
        tValueByte = *DataBufferControl.DataBufferNextDrawPointer++;
        DataBufferControl.DisplayBuffer[tBufferIndex] = tValueByte;

        if (tBufferIndex != 0 && tBufferIndex <= REMOTE_DISPLAY_WIDTH - 1) {
            // get last value and draw line
            tLastValueByte = DataBufferControl.DisplayBuffer[tBufferIndex - 1];
            BlueDisplay1.drawLineFastOneX(tBufferIndex - 1, tLastValueByte, tValueByte, COLOR_DATA_RUN);
        }
        tBufferIndex++;
    }
    DataBufferControl.DataBufferNextDrawIndex = tBufferIndex;
}

/************************************************************************
 * Text output section
 ************************************************************************/
/*
 * Move 2 digits to front and set thousand separator
 */
void formatThousandSeparator(char *aThousandPosition) {
    char tNewChar;
    char tOldChar = *aThousandPosition;
//set separator for thousands
    *aThousandPosition-- = THOUSANDS_SEPARATOR;
    for (uint_fast8_t i = 2; i > 0; i--) {
        tNewChar = *aThousandPosition;
        *aThousandPosition-- = tOldChar;
        tOldChar = tNewChar;
    }
}

/*
 * prints a period to a 9 character string and sets the thousand separator
 */
void printfMicrosPeriod(char *aDataBufferPtr, uint32_t aPeriod) {
    char tPeriodUnitChar;
    uint16_t tPeriod;
    if (aPeriod >= 50000l) {
        tPeriod = aPeriod / 1000;
        tPeriodUnitChar = 'm'; // milli
    } else {
        tPeriod = aPeriod;
        tPeriodUnitChar = '\xB5'; // micro
    }
    sprintf_P(aDataBufferPtr, PSTR("  %5u%cs"), tPeriod, tPeriodUnitChar);
    if (tPeriod >= 1000) {
        formatThousandSeparator(aDataBufferPtr + 3);
    }
}

/*
 * prints TriggerDelayMillisOrMicros with right unit according to TriggerDelayMode to a character string and sets the thousand separator
 */
void printfTriggerDelay(char *aDataBufferPtr, uint16_t aTriggerDelayMillisOrMicros) {
    char tPeriodUnitChar;
    if (MeasurementControl.TriggerDelayMode == TRIGGER_DELAY_MILLIS) {
        tPeriodUnitChar = 'm'; // milli
    } else {
        tPeriodUnitChar = '\xB5'; // micro
    }
    sprintf_P(aDataBufferPtr, PSTR(" %5u%cs"), aTriggerDelayMillisOrMicros, tPeriodUnitChar);
    if (aTriggerDelayMillisOrMicros >= 1000) {
        formatThousandSeparator(aDataBufferPtr + 2);
    }
}

void printSingleshotMarker() {
// draw an S to indicate running single shot trigger
    BlueDisplay1.drawChar(SINGLESHOT_PPRINT_VALUE_X, FONT_SIZE_INFO_LONG_ASC, 'S', FONT_SIZE_INFO_LONG, COLOR16_BLACK,
    COLOR_INFO_BACKGROUND);
}

void clearSingleshotMarker() {
    BlueDisplay1.drawChar(SINGLESHOT_PPRINT_VALUE_X, FONT_SIZE_INFO_LONG_ASC, ' ', FONT_SIZE_INFO_LONG, COLOR16_BLACK,
    COLOR_BACKGROUND_DSO);
}

/*
 * Output info line
 * for documentation see start of this file
 */
void printInfo(bool aRecomputeValues) {
    if (DisplayControl.showInfoMode == INFO_MODE_NO_INFO) {
        return;
    }

    if (aRecomputeValues) {
        computePeriodFrequency(); // is only called here :-)
    }

    char tSlopeChar;
    char tTimebaseUnitChar;
    char tReferenceChar;
    char tMinStringBuffer[6];
    char tAverageStringBuffer[6];
    char tMaxStringBuffer[5];
    char tP2PStringBuffer[5];
    char tTriggerStringBuffer[6];
    float tRefMultiplier;

    float tVoltage;

    if (MeasurementControl.TriggerSlopeRising) {
        tSlopeChar = 0xD1; //ascending
    } else {
        tSlopeChar = 0xD2; //Descending
    }

    if (MeasurementControl.ChannelHasActiveAttenuator) {
        tRefMultiplier = 2.0 / 1.1;
    } else {
        tRefMultiplier = 1;
    }
    if (MeasurementControl.ADCReference == DEFAULT) {
        tReferenceChar = '5';
        tRefMultiplier *= MeasurementControl.VCC / 1024.0;
        /*
         * Use 1023 to get 5 volt display for full scale reading
         * Better would be 5.0 / 1024.0; since the reading for value for 5 volt- 1 LSB is also 1023,
         * but this implies that the maximum displayed value is 4.99(51171875) :-(
         */
    } else {
        tReferenceChar = '1';
        tRefMultiplier *= 1.1 / 1024.0;
    }
    tRefMultiplier *= getAttenuatorFactor();

    uint8_t tPrecision = 2 - MeasurementControl.AttenuatorValue;
    int16_t tACOffset = 0;
    if (MeasurementControl.ChannelIsACMode) {
        tACOffset = MeasurementControl.RawDSOReadingACZero;
    }

// 2 kByte code size
    tVoltage = tRefMultiplier * ((int16_t) MeasurementControl.RawValueMin - tACOffset);
    dtostrf(tVoltage, 5, tPrecision, tMinStringBuffer);
    tVoltage = tRefMultiplier * ((int16_t) MeasurementControl.ValueAverage - tACOffset);
    dtostrf(tVoltage, 5, tPrecision, tAverageStringBuffer);
    tVoltage = tRefMultiplier * ((int16_t) MeasurementControl.RawValueMax - tACOffset);
// 4 since we need no sign
    dtostrf(tVoltage, 4, tPrecision, tMaxStringBuffer);
    tVoltage = tRefMultiplier * (MeasurementControl.RawValueMax - MeasurementControl.RawValueMin);
// 4 since we need no sign
    dtostrf(tVoltage, 4, tPrecision, tP2PStringBuffer);

    tVoltage = ((int16_t) MeasurementControl.RawTriggerLevel - tACOffset);

    tVoltage = tRefMultiplier * tVoltage;
    dtostrf(tVoltage, 5, tPrecision, tTriggerStringBuffer);

    uint16_t tTimebaseUnitsPerGrid;
    if (MeasurementControl.TimebaseIndex >= TIMEBASE_INDEX_MILLIS) {
        tTimebaseUnitChar = 'm';
    } else {
        tTimebaseUnitChar = '\xB5'; // micro
    }
    tTimebaseUnitsPerGrid = pgm_read_word(&TimebaseDivPrintValues[MeasurementControl.TimebaseIndex]);

    uint32_t tHertz = MeasurementControl.FrequencyHertz;

    if (DisplayControl.showInfoMode == INFO_MODE_LONG_INFO) {
        /*
         * Long version 1. line Timebase, Channel, (min, average, max, peak to peak) voltage, Trigger, Reference.
         */
        sprintf_P(sStringBuffer, PSTR("%3u%cs %c      %s %s %s P2P%sV %sV %c"), tTimebaseUnitsPerGrid, tTimebaseUnitChar,
                tSlopeChar, tMinStringBuffer, tAverageStringBuffer, tMaxStringBuffer, tP2PStringBuffer, tTriggerStringBuffer,
                tReferenceChar);
        memcpy_P(&sStringBuffer[8], ADCInputMUXChannelStrings[MeasurementControl.ADCInputMUXChannelIndex], 4);
        BlueDisplay1.drawText(INFO_LEFT_MARGIN, FONT_SIZE_INFO_LONG_ASC, sStringBuffer, FONT_SIZE_INFO_LONG, COLOR16_BLACK,
        COLOR_INFO_BACKGROUND);

        /*
         * 2. line - timing, period, 1st interval, 2nd interval
         */
        // 8 Character
        sprintf_P(sStringBuffer, PSTR(" %5luHz"), tHertz);
        if (tHertz >= 1000) {
            formatThousandSeparator(&sStringBuffer[2]);
        }
        /*
         * period and pulse values
         * Just write the fixed format strings to buffer, so that they are concatenated
         */
        printfMicrosPeriod(&sStringBuffer[8], MeasurementControl.PeriodMicros);
        printfMicrosPeriod(&sStringBuffer[17], MeasurementControl.PeriodFirst);
        printfMicrosPeriod(&sStringBuffer[26], MeasurementControl.PeriodSecond);

        /*
         * Delay - 14 character including leading space
         */
        if (MeasurementControl.TriggerDelayMode != TRIGGER_DELAY_NONE) {
            strcpy_P(&sStringBuffer[35], PSTR(" del "));
            printfTriggerDelay(&sStringBuffer[40], MeasurementControl.TriggerDelayMillisOrMicros);
        }

        BlueDisplay1.drawText(INFO_LEFT_MARGIN, FONT_SIZE_INFO_LONG_ASC + FONT_SIZE_INFO_LONG, sStringBuffer, FONT_SIZE_INFO_LONG,
        COLOR16_BLACK, COLOR_INFO_BACKGROUND);

    } else {
        /*
         * Short version
         */
#if defined(LOCAL_DISPLAY_EXISTS)
        snprintf(sStringBuffer, sizeof sStringBuffer, "%6.*fV %6.*fV%s%4u%cs", tPrecision,
                getFloatFromRawValue(MeasurementControl.RawValueAverage), tPrecision,
                getFloatFromRawValue(tValueDiff), tBufferForPeriodAndFrequency, tUnitsPerGrid, tTimebaseUnitChar);
#else
#if defined(AVR)

        sprintf_P(sStringBuffer, PSTR("%sV %sV  %5luHz %3u%cs"), tAverageStringBuffer, tP2PStringBuffer, tHertz,
                tTimebaseUnitsPerGrid, tTimebaseUnitChar);
        if (tHertz >= 1000) {
            formatThousandSeparator(&sStringBuffer[15]);
        }

#else
        snprintf(sStringBuffer, sizeof sStringBuffer, "%6.*fV %6.*fV  %6luHz %4u%cs", tPrecision,
                getFloatFromRawValue(MeasurementControl.RawValueAverage), tPrecision,
                getFloatFromRawValue(tValueDiff), MeasurementControl.FrequencyHertz, tUnitsPerGrid,
                tTimebaseUnitChar);
        if (MeasurementControl.FrequencyHertz >= 1000) {
            // set separator for thousands
            formatThousandSeparator(&sStringBuffer[16], &sStringBuffer[19]);
        }
#endif
#endif
        BlueDisplay1.drawText(INFO_LEFT_MARGIN, FONT_SIZE_INFO_SHORT_ASC, sStringBuffer, FONT_SIZE_INFO_SHORT, COLOR16_BLACK,
        COLOR_INFO_BACKGROUND);
    }
}

// TODO we have a lot of duplicate code here and in printInfo()
void printTriggerInfo(void) {
    float tVoltage;
    float tRefMultiplier;
    if (MeasurementControl.ChannelHasActiveAttenuator) {
        tRefMultiplier = 2.0 / 1.1;
    } else {
        tRefMultiplier = 1;
    }
    if (MeasurementControl.ADCReference == DEFAULT) {
        tRefMultiplier *= MeasurementControl.VCC / 1024.0;
        /*
         * Use 1023 to get 5 volt display for full scale reading
         * Better would be 5.0 / 1024.0; since the reading for value for 5 volt- 1 LSB is also 1023,
         * but this implies that the maximum displayed value is 4.99(51171875) :-(
         */
    } else {
        tRefMultiplier *= 1.1 / 1024.0;
    }
    tRefMultiplier *= getAttenuatorFactor();

    int16_t tACOffset = 0;
    if (MeasurementControl.ChannelIsACMode) {
        tACOffset = MeasurementControl.RawDSOReadingACZero;
    }
    tVoltage = ((int16_t) MeasurementControl.RawTriggerLevel - tACOffset);

    tVoltage = tRefMultiplier * tVoltage;
    uint8_t tPrecision = 2 - MeasurementControl.AttenuatorValue;
    dtostrf(tVoltage, 5, tPrecision, sStringBuffer);
    sStringBuffer[5] = 'V';
    sStringBuffer[6] = '\0';

    uint8_t tYPos = TRIGGER_LEVEL_INFO_SHORT_Y;
    uint16_t tXPos = TRIGGER_LEVEL_INFO_SHORT_X;
    uint8_t tFontsize = FONT_SIZE_INFO_SHORT;
    if (DisplayControl.showInfoMode == INFO_MODE_NO_INFO) {
        tYPos = FONT_SIZE_INFO_SHORT_ASC;
    } else if (DisplayControl.showInfoMode == INFO_MODE_LONG_INFO) {
        tXPos = TRIGGER_LEVEL_INFO_LONG_X;
        tYPos = TRIGGER_LEVEL_INFO_LONG_Y;
        tFontsize = FONT_SIZE_INFO_LONG;
    }
    BlueDisplay1.drawText(tXPos, tYPos, sStringBuffer, tFontsize, COLOR16_BLACK, COLOR_INFO_BACKGROUND);
}

/*
 * Show temperature and VCC voltage
 */
void printVCCAndTemperature(void) {
    if (!MeasurementControl.isRunning) {
        float tTemp = getTemperature();
        dtostrf(tTemp, 4, 1, &sStringBuffer[40]);

        setVCCValue();
        dtostrf(MeasurementControl.VCC, 4, 2, &sStringBuffer[30]);

        sprintf_P(sStringBuffer, PSTR("%s volt %s\xB0" "C"), &sStringBuffer[30], &sStringBuffer[40]); // \xB0 is degree character
        BlueDisplay1.drawText(BUTTON_WIDTH_3_POS_2, SETTINGS_PAGE_INFO_Y, sStringBuffer,
        TEXT_SIZE_11, COLOR16_BLACK, COLOR_BACKGROUND_DSO);
    }
}

/************************************************************************
 * Utility section
 ************************************************************************/

/*
 * initialize RAM between actual stack and actual heap start (__brkval) with pattern 0x5A
 */
#define HEAP_STACK_UNTOUCHED_VALUE 0x5A
void initStackFreeMeasurement(void) {
    extern unsigned int __heap_start;
    extern void *__brkval;
    uint8_t v;

    uint8_t *tHeapPtr = (uint8_t*) __brkval;
    if (tHeapPtr == 0) {
        tHeapPtr = (uint8_t*) &__heap_start;
    }

// Fill memory
    do {
        *tHeapPtr++ = HEAP_STACK_UNTOUCHED_VALUE;
    } while (tHeapPtr < &v);
}

/*
 * Returns the amount of stack not touched since the last call to initStackFreeMeasurement()
 * by check for first touched pattern on the stack, starting the search at heap start.
 */
uint16_t getStackUnusedBytes() {
    extern unsigned int __heap_start;
    extern void *__brkval;
    uint8_t tDummyVariableOnStack;

    uint8_t *tHeapPtr = (uint8_t*) __brkval;
    if (tHeapPtr == 0) {
        tHeapPtr = (uint8_t*) &__heap_start;
    }

// first search for first match after current begin of heap, because malloc() and free() may be happened in between and overwrite low memory
    while (*tHeapPtr != HEAP_STACK_UNTOUCHED_VALUE && tHeapPtr < &tDummyVariableOnStack) {
        tHeapPtr++;
    }
// then count untouched patterns
    uint16_t tStackUnused = 0;
    while (*tHeapPtr == HEAP_STACK_UNTOUCHED_VALUE && tHeapPtr < &tDummyVariableOnStack) {
        tHeapPtr++;
        tStackUnused++;
    }
// word -> bytes
    return tStackUnused;
}

/*
 * Show minimum free space on stack
 * Needs 260 byte of FLASH
 */
void printFreeStack(void) {
    uint16_t tUntouchesBytesOnStack = getStackUnusedBytes();
    sprintf_P(sStringBuffer, PSTR("%4u Stack[bytes]"), tUntouchesBytesOnStack);
    BlueDisplay1.drawText(0, SETTINGS_PAGE_INFO_Y, sStringBuffer,
    TEXT_SIZE_11, COLOR16_BLACK, COLOR_BACKGROUND_DSO);
}

/*
 * Change actual timebase index by value
 */
uint8_t changeTimeBaseValue(int8_t aChangeIndexValue) {
    bool IsError = false;
    uint8_t tOldIndex = MeasurementControl.TimebaseIndex;

// positive value means increment timebase index!
    int8_t tNewIndex = tOldIndex + aChangeIndexValue;
    if (tNewIndex < 0) {
        tNewIndex = 0;
        IsError = true;
    } else if (tNewIndex > TIMEBASE_NUMBER_OF_ENTRIES - 1) {
        tNewIndex = TIMEBASE_NUMBER_OF_ENTRIES - 1;
        IsError = true;
    }

    bool tStartNewAcquisition = false;
    DisplayControl.DrawWhileAcquire = (tNewIndex >= TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE);

    if (tOldIndex >= TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE && tNewIndex < TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE) {
        // from draw while acquire to normal mode -> stop acquisition, clear old chart, and start a new one
        ADCSRA &= ~_BV(ADIE); // stop acquisition - disable ADC interrupt
        clearDisplayedChart(&DataBufferControl.DisplayBuffer[0]);
        tStartNewAcquisition = true;
    }

    if (tOldIndex < TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE && tNewIndex >= TIMEBASE_INDEX_DRAW_WHILE_ACQUIRE) {
        // from normal to draw while acquire mode
        ADCSRA &= ~_BV(ADIE); // stop acquisition - disable ADC interrupt
        clearDisplayedChart(&DataBufferControl.DataBuffer[0]);
        tStartNewAcquisition = true;
    }

    MeasurementControl.TimebaseIndex = tNewIndex;

    /*
     * Set trigger timeout. Used only for trigger modes with timeout.
     */
    uint16_t tTriggerTimeoutSampleCount;
    /*
     * Try to have the time for showing one display as trigger timeout
     * Don't go below 1/10 of a second (30 * 256 samples) and above 3 seconds (900)
     * 10 ms Range => timeout = 100 millisecond
     * 20 ms => 200 ms, 50 ms => 500 ms, 100 ms => 1s, 200 ms => 2s, 500 ms => 3s
     * Trigger is always running with ADC at free running mode at 1 us clock => 13 us per conversion
     * which gives 77k samples for 1 second
     */
    if (tNewIndex <= 9) {
        tTriggerTimeoutSampleCount = 30;
    } else if (tNewIndex >= 14) {
        tTriggerTimeoutSampleCount = 900;
    } else {
        tTriggerTimeoutSampleCount = pgm_read_word(&TimebaseDivPrintValues[tNewIndex]) * 3;
    }

    MeasurementControl.TriggerTimeoutSampleCount = tTriggerTimeoutSampleCount;

// reset xScale to regular value
    if (MeasurementControl.TimebaseIndex < TIMEBASE_NUMBER_OF_XSCALE_CORRECTION) {
        DisplayControl.XScale = xScaleForTimebase[tNewIndex];
    } else {
        DisplayControl.XScale = 1;
    }
    if (tStartNewAcquisition) {
        startAcquisition();
    }

    if (tNewIndex >= TIMEBASE_NUMBER_OF_FAST_MODES) {
        /*
         * set timer 0
         */
        if (MeasurementControl.AttenuatorType < ATTENUATOR_TYPE_ACTIVE_ATTENUATOR) {
            // output half the timebase at pin 6
            TCCR0A = _BV(COM0A0) | _BV(WGM01); // CTC mode, toggle OC0A on Compare Match
        } else {
            TCCR0A = _BV(WGM01); // CTC mode
        }
        uint8_t tTimer0HWPrescaler;
        if (tNewIndex < 8) {
            tTimer0HWPrescaler = TIMER0_PRESCALE8;
        } else if (tNewIndex < 11) {
            tTimer0HWPrescaler = TIMER0_PRESCALE64;
        } else if (tNewIndex < 13) {
            tTimer0HWPrescaler = TIMER0_PRESCALE256;
        } else {
            tTimer0HWPrescaler = TIMER0_PRESCALE1024;
        }
        TCCR0B = tTimer0HWPrescaler;
        OCR0A = CTCValueforTimebase[tNewIndex - TIMEBASE_NUMBER_OF_FAST_MODES] - 1;
    }
    return IsError;
}

uint8_t getDisplayFromRawInputValue(uint16_t aRawValue) {
    aRawValue -= MeasurementControl.OffsetValue;
    aRawValue >>= MeasurementControl.ShiftValue;
    if (DISPLAY_VALUE_FOR_ZERO > aRawValue) {
        return (DISPLAY_VALUE_FOR_ZERO - aRawValue);
    } else {
        return 0;
    }
}

uint16_t getInputRawFromDisplayValue(uint8_t aDisplayValue) {
    aDisplayValue = DISPLAY_VALUE_FOR_ZERO - aDisplayValue;
    uint16_t aValue16 = aDisplayValue << MeasurementControl.ShiftValue;
    aValue16 = aValue16 + MeasurementControl.OffsetValue;
    return aValue16;
}
/*
 * computes corresponding voltage from display y position (DISPLAY_HEIGHT - 1 -> 0 volt)
 */
float getFloatFromDisplayValue(uint8_t aDisplayValue) {
    float tFactor;
    if (MeasurementControl.ChannelHasActiveAttenuator) {
        tFactor = 2.0 / 1.1;
    } else {
        tFactor = 1;
    }
    if (MeasurementControl.ADCReference == DEFAULT) {
        tFactor *= MeasurementControl.VCC / 1024.0;
    } else {
        tFactor *= 1.1 / 1024.0;
    }
    int16_t tRaw = getInputRawFromDisplayValue(aDisplayValue);
    if (MeasurementControl.ChannelIsACMode) {
        tRaw -= MeasurementControl.RawDSOReadingACZero;
    }
// cannot multiply tRaw with getAttenuatorFactor() before since it can lead to 16 bit overflow
    tFactor *= tRaw;
    tFactor *= getAttenuatorFactor();
    return tFactor;
}

/************************************************************************
 * Hardware support section
 ************************************************************************/

void setVCCValue(void) {
    MeasurementControl.VCC = getVCCVoltage();
}

/*
 * setChannel() sets:
 * ShiftValue
 * ADCInputMUXChannel
 * ADCInputMUXChannelChar
 * ADCReference
 * tHasACDC
 * isACMode
 *
 * And by setInputRange():
 * OffsetValue
 * HorizontalGridSizeShift8
 * HorizontalGridVoltage
 * needs:
 * VCC
 *
 * And if attenuator detected also:
 * AttenuatorValue
 *
 * For active attenuator also:
 * ChannelHasAttenuator
 *  */
void setChannel(uint8_t aChannel) {
    MeasurementControl.ADCInputMUXChannelIndex = aChannel;
    /*
     * Set default values for plain inputs without attenuator but potential AC/DC capabilities
     */
    MeasurementControl.ShiftValue = 2;
    bool tIsACMode = false;
    MeasurementControl.AttenuatorValue = 0; // no attenuator attached at channel
    uint8_t tHasAC_DC = true;
    uint8_t tReference = DEFAULT; // DEFAULT/1 -> VCC   INTERNAL/3 -> 1.1 volt

    if (MeasurementControl.AttenuatorType >= ATTENUATOR_TYPE_ACTIVE_ATTENUATOR) {
        if (aChannel < NUMBER_OF_CHANNEL_WITH_ACTIVE_ATTENUATOR) {
            MeasurementControl.ChannelHasActiveAttenuator = true;
            // restore AC mode for this channels
            tIsACMode = MeasurementControl.isACMode;
            // use internal reference if attenuator is available
            tReference = INTERNAL;
        } else {
            tHasAC_DC = false;
            MeasurementControl.ChannelHasActiveAttenuator = false;
            // Set to high attenuation to protect input. Since ChannelHasActiveAttenuator = false it will not be changed by setInputRange()
            setAttenuator(3);
        }
    } else if (MeasurementControl.AttenuatorType == ATTENUATOR_TYPE_FIXED_ATTENUATOR) {
        if (aChannel < NUMBER_OF_CHANNELS_WITH_FIXED_ATTENUATOR) {
            MeasurementControl.AttenuatorValue = aChannel; // channel 0 has 10^0 attenuation factor etc.
            // restore AC mode for this channels
            tIsACMode = MeasurementControl.isACMode;
            tReference = INTERNAL;
        }
    }

    /*
     * Map channel index to special channel numbers
     */
    if (aChannel == MAX_ADC_EXTERNAL_CHANNEL + 1) {
        aChannel = ADC_TEMPERATURE_CHANNEL; // Temperature
        tHasAC_DC = false;
    }
    if (aChannel == MAX_ADC_EXTERNAL_CHANNEL + 2) {
        aChannel = ADC_1_1_VOLT_CHANNEL; // 1.1 Reference
        tHasAC_DC = false;
    }
    ADMUX = aChannel | (tReference << REFS0);

    MeasurementControl.ChannelIsACMode = tIsACMode;
    MeasurementControl.ChannelHasAC_DCSwitch = tHasAC_DC;
    MeasurementControl.ADCReference = tReference;

//the second parameter for active attenuator is only required if ChannelHasActiveAttenuator == true
    setInputRange(2, 2);
}

inline void setPrescaleFactor(uint8_t aFactor) {
    ADCSRA = (ADCSRA & ~0x07) | aFactor;
}

// DEFAULT/1 -> VCC   INTERNAL/3 -> 1.1  volt
void setADCReference(uint8_t aReference) {
    MeasurementControl.ADCReference = aReference;
    ADMUX = (ADMUX & ~0xC0) | (aReference << REFS0);
}

void setTimer2FastPWMOutput() {
#if defined(TCCR2A)
    OCR2A = 125 - 1; // set compare match register for 50% duty cycle
    TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20); // Clear OC2A/PB3/D11 on compare match, set at 00 / Fast PWM mode with 0xFF as TOP
#else
            OCR3A = 125 - 1; // set compare match register for 50% duty cycle
            TCCR3A = 0;// set entire TCCR3A register to 0 - Normal mode
            TCCR3A = _BV(WGM30);// Fast PWM, 8-bit
            TCCR3B = 0;
            TCCR3B = _BV(WGM32) | _BV(CS31) | _BV(CS30);// Clock/64 => 4 us. Fast PWM, 8-bit
#endif
}

/*
 * Square wave for VEE (-5 volt) generation and interrupts for Arduino millis()
 */
void initTimer2(void) {
#if defined(TCCR2A)
    // initialization with 0 is essential otherwise timer will not work correctly!!!
    TCCR2A = 0; // set entire TCCR2A register to 0 - Normal mode
    TCCR2B = 0; // same for TCCR2B ???
    TCCR2B = _BV(CS22); // Clock/64 => 4 us
#else
            // ???initialization with 0 is essential otherwise timer will not work correctly???
            TCCR3A = 0;// set entire TCCR3A register to 0 - Normal mode
            TCCR3A = _BV(WGM30);// Fast PWM, 8-bit
            TCCR3B = 0;
            TCCR3B = _BV(WGM32) | _BV(CS31) | _BV(CS30);// Clock/64 => 4 us. Fast PWM, 8-bit
#endif
    TIMSK2 = _BV(TOIE2); // Enable overflow interrupts which replaces the Arduino millis() interrupt
}

/*
 * Redirect interrupts for TIMER2 Overflow to existent arduino TIMER0 Overflow ISR used for millis()
 */
#if defined(TCCR2A)
ISR_ALIAS(TIMER2_OVF_vect, TIMER0_OVF_vect);
#else
ISR_ALIAS(TIMER3_OVF_vect, TIMER0_OVF_vect);

#endif

#if defined(DEBUG)
void printDebugData(void) {
    sprintf_P(sStringBuffer, PSTR("%5d, 0x%04X, 0x%04X, 0x%04X"), DebugValue1, DebugValue2, DebugValue3, DebugValue4);
    BlueDisplay1.drawText(INFO_LEFT_MARGIN, INFO_UPPER_MARGIN + 2 * TEXT_SIZE_11_HEIGHT, sStringBuffer, 11, COLOR16_BLACK,
            COLOR_BACKGROUND_DSO);
}
#endif

/*
 * For the common parts of AVR and ARM development
 */
//#include "TouchDSOCommon.cpp"
