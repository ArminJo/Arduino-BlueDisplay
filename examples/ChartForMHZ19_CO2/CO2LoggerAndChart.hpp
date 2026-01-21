/*
 *  CO2LoggerAndChart.hpp
 *
 *  Stores CO2 values in an array and show them on a Android mobile or tablet running the BlueFisplay app.
 *
 *  Values are stored as 8 bits with 0 = 400 ppm, 1=405, 2=410, 20=500, 60=700, 80=800, 100=900, 120=1000, 200=1400,
 *  Each 5 minutes the minimum of the snooped values is stored. I.e. 1 hour has 12 values, 72-> 6 hours, 288->1 day, 1152->4 days, 1440->5 days
 *
 *  DISPLAY_WIDTH is defined as 3.3/2 of CHART_WIDTH and CHART_WIDTH is POWER_ARRAY_SIZE / 2.
 *
 *  Copyright (C) 2024-2026  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of Arduino-BlueDisplay https://github.com/ArminJo/Arduino-BlueDisplay.
 *
 *  BlueDisplay is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 */

#ifndef _CO2_LOGGER_AND_CHART_HPP
#define _CO2_LOGGER_AND_CHART_HPP

//#define DO_NOT_NEED_BASIC_TOUCH_EVENTS    // Disables basic touch events down, move and up. Saves 620 bytes program memory and 36 bytes RAM
#define DO_NOT_NEED_LONG_TOUCH_DOWN_AND_SWIPE_EVENTS    // Disables LongTouchDown and SwipeEnd events.
#define DO_NOT_NEED_SPEAK_EVENTS            // Disables SpeakingDone event handling. Saves up to 54 bytes program memory and 18 bytes RAM.
#define ONLY_CONNECT_EVENT_REQUIRED         // Disables reorientation, redraw and SensorChange events
#include "Chart.hpp"

void getTimeEventCallbackForLogger(uint8_t aSubcommand, uint8_t aByteInfo, uint16_t aShortInfo, ByteShortLongFloatUnion aLongInfo);
#define TIME_EVENTCALLBACK_FUNCTION     getTimeEventCallbackForLogger // use this function, which will also set sNextStorageMillis
#include "BDTimeHelper.hpp"

//#define LOCAL_DEBUG
//#define ENABLE_STACK_ANALYSIS
#if defined(ENABLE_STACK_ANALYSIS)
#include "AVRUtils.h" // include for initStackFreeMeasurement() and printRAMInfo()
#include "AVRTracing.h"
#endif

/*
 * DISPLAY_WIDTH is defined as 3.3/2 of CHART_WIDTH
 * Scale the screen such, that this fit horizontally.
 * Border - YLabels - Chart with CO2_ARRAY_SIZE / 2 - Border - Buttons for 6 big characters - Border
 * Values for CO2_ARRAY_SIZE = 1152
 */
#define CHART_WIDTH     (CO2_ARRAY_SIZE / 2)            // 576
#define DISPLAY_WIDTH   ((CO2_ARRAY_SIZE * 33) / 40)    // 950,4
#define BASE_TEXT_SIZE  (CO2_ARRAY_SIZE / 40)           // 1152 / 40 = 28.8
#define BASE_TEXT_SIZE_2  (CO2_ARRAY_SIZE / 20)         // 1152 / 20 = 57.6
#define BASE_TEXT_SIZE_1_5  ((CO2_ARRAY_SIZE * 3) / 80) // 43.2
#define BASE_TEXT_SIZE_HALF  ((CO2_ARRAY_SIZE) / 80)    // 14.4
#define SMALL_BUTTON_WIDTH  (BASE_TEXT_SIZE * 4)        // 57
#define BUTTON_WIDTH    ((SMALL_BUTTON_WIDTH * 2) + BASE_TEXT_SIZE_HALF)     // 236
#define CHART_START_X   (3 * BASE_TEXT_SIZE)            // 84 Origin of chart
#define CHART_START_Y   (BlueDisplay1.getRequestedDisplayHeight() - BASE_TEXT_SIZE_1_5)
#define CHART_AXES_SIZE (BASE_TEXT_SIZE / 8)            // 3
#define BUTTONS_START_X ( CHART_WIDTH + (4 * BASE_TEXT_SIZE))
#define TIME_MARKER_START_Y (BlueDisplay1.getRequestedDisplayHeight() - BASE_TEXT_SIZE)
#define CHART_Y_LABEL_INCREMENT 200L

#define CHART_BACKGROUND_COLOR  COLOR16_WHITE
#define CHART_DATA_COLOR        COLOR16_RED
#define CHART_AXES_COLOR        COLOR16_BLUE
#define CHART_GRID_COLOR        COLOR16_GREEN
#define CHART_DATA_COLOR        COLOR16_RED
#define CHART_TEXT_COLOR        COLOR16_BLACK
#define DAY_BUTTONS_COLOR       COLOR16_GREEN

Chart CO2Chart;
BDButton TouchButton4days;
BDButton TouchButton2days;
BDButton TouchButton1day;
BDButton TouchButton12Hour;
BDButton TouchButtonBrightness;
BDButton TouchButtonStoreToEEPROM;
BDButton TouchButtonShowTimeToNextStorage; // overlay for time text, not drawn, only activated

void initDisplay(void);
void drawDisplay();

void initCO2Chart();
void drawCO2Chart();

bool sDoInitDisplay = false; // used to call initDisplay on main loop which does save stack space.
bool sDoRefreshOrChangeBrightness = false; // used to call changeBrightness on main loop which does save stack space.
unsigned long sMillisOfLastRefreshOrChangeBrightness;
#define TIMEOUT_FOR_BRIGHTNESS_MILLIS      4000 // Before 4 seconds the next touch is interpreted as brightness change request

#define BRIGHTNESS_LOW      2
#define BRIGHTNESS_MIDDLE   1
#define BRIGHTNESS_HIGH     0
#define START_BRIGHTNESS    BRIGHTNESS_HIGH
uint8_t sCurrentBrightness = START_BRIGHTNESS;
color16_t sBackgroundColor = CHART_BACKGROUND_COLOR;
color16_t sTextColor = CHART_TEXT_COLOR;

/*
 * CO2 storage in RAM
 */
#define CO2_BASE_VALUE          400L
#define CO2_COMPRESSION_FACTOR    5L // value 1 -> 405, 2 -> 410 etc.
/*
 * Layout values for fullscreen GUI
 */
uint8_t sChartMaxValue = ((1400L - CO2_BASE_VALUE) / CO2_COMPRESSION_FACTOR); // For clipping, and initialized, if BT is not available at startup

/*
 * Even with initDisplay() called from main loop we only have 1140 bytes available for application
 */
#define NUMBER_OF_DAYS_IN_BUFFER    4
// At 5 minutes / sample: 576 -> 2 days, 864 -> 3 days, 1152-> 4 days, 1440->5 days
#define CO2_ARRAY_SIZE (NUMBER_OF_DAYS_IN_BUFFER * HOURS_IN_ONE_DAY * 12UL) // 1152 - UL is essential!
uint16_t sCO2ArrayValuesChecksum __attribute__((section(".noinit")));  // must be in noinit, otherwise it is set to 0 at boot
uint8_t sCO2Array[CO2_ARRAY_SIZE] __attribute__((section(".noinit"))); // values are (CO2[ppm] - 400) / 5
uint16_t sChartHoursToDisplay __attribute__((section(".noinit")));  // is initialized to 96 if checksum is wrong
uint8_t *sCO2ArrayDisplayStart = sCO2Array; // Start of displayed values determined by doDays()
uint16_t sCO2MinimumOfCurrentReadings; // the minimum of all values received during one period

/*
 * CO2 storage in EEPROM
 */
#if defined(E2END) // 1023 for Atmega328
#  if CO2_ARRAY_SIZE < E2END
// If CO2_ARRAY_SIZE is smaller than E2END, than EEPROM Array can overwrite more than sCO2Array (and loop forever due to lazy programming ;-)
#define EEPROM_CO2_ARRAY_SIZE   CO2_ARRAY_SIZE
#else
#  if defined(EEPROM_REQUIRED_FOR_APPLICATION_BYTES)
#define EEPROM_CO2_ARRAY_SIZE   ((E2END + 1) - EEPROM_REQUIRED_FOR_APPLICATION_BYTES)
#  else
#define EEPROM_CO2_ARRAY_SIZE   (E2END + 1)
#  endif
#endif
EEMEM uint8_t sCO2ArrayInEEPROM[EEPROM_CO2_ARRAY_SIZE];
void doStoreInEEPROM(BDButton *aTheTouchedButton, int16_t aValue);
#endif

void initializeCO2Array();
void writeToCO2Array(uint8_t aCO2Value);

void signalInitDisplay(void);
void doDays(BDButton *aTheTouchedButton, int16_t aChartHoursToDisplay);
void doSignalChangeBrightness(BDButton *aTheTouchedButton, int16_t aValue);
void doShowTimeToNextStorage(BDButton *aTheTouchedButton, int16_t aValue);

bool handleEventAndFlags();
void checkAndHandleCO2LoggerEventFlags();

void printCO2Value();
void changeBrightness();

/*
 * Date and time handling for X scale
 */
#define STORAGE_INTERVAL_SECONDS (5 * SECONDS_IN_ONE_MINUTE)
// sNextStorageMillis is required by CO2LoggerAndChart.hpp for toast at connection startup
//uint32_t sNextStorageMillis = SECONDS_IN_ONE_MINUTE * MILLIS_IN_ONE_SECOND; // If not connected first storage in 1 after boot.
uint32_t sNextStorageSeconds = SECONDS_IN_ONE_MINUTE; // If not connected first storage in 1 after boot.

void printTimeToNextStorage();

/*
 * Code starts here
 */

/*
 * Is intended to be called by setup()
 */
void InitCo2LoggerAndChart() {
#if defined(ENABLE_STACK_ANALYSIS)
    initStackFreeMeasurement();
#endif

    /*
     * Register callback handler and wait for 1500 ms (CONNECTIOM_TIMEOUT_MILLIS) if Bluetooth connection is still active.
     * For ESP32 and after power on of the Bluetooth module (HC-05) at other platforms, Bluetooth connection is most likely not active here.
     *
     * If active, mCurrentDisplaySize and mHostUnixTimestamp are set and callback handler (initDisplay() and drawGui()) functions are called.
     * If not active, the periodic call of checkAndHandleEvents() in the main loop waits for the (re)connection and then performs the same actions.
     *
     * Here the initDisplay() is delayed and called by the loop().
     */
    BlueDisplay1.initCommunication(&Serial, &signalInitDisplay); // introduces up to 1.5 seconds delay
    initializeCO2Array();
    sCO2MinimumOfCurrentReadings = __UINT16_MAX__;

// Simulate a connection for testing
//    BlueDisplay1.mHostDisplaySize.XWidth = 1280;
//    BlueDisplay1.mHostDisplaySize.YHeight = 800;
//    BlueDisplay1.mHostUnixTimestamp = 1733191057;
//    BlueDisplay1.mBlueDisplayConnectionEstablished = true;
//    sDoInitDisplay = true;

#if defined(STANDALONE_TEST)
    uint8_t *tArrayFillPointer = sCO2Array;
    for (int i = 0; i < 200; ++i) {
        *tArrayFillPointer++ = i;
        *tArrayFillPointer++ = i;
        *tArrayFillPointer++ = i;
        *tArrayFillPointer++ = i;
    }
    for (int i = 200; i > 120; --i) {
        *tArrayFillPointer++ = i;
        *tArrayFillPointer++ = i;
        *tArrayFillPointer++ = i;
        *tArrayFillPointer++ = i;
    }
    while (tArrayFillPointer < &sCO2Array[CO2_ARRAY_SIZE]) {
        *tArrayFillPointer++ = 80;
    }
#endif
}

/*
 * Is intended to be called by main loop
 */
bool storeCO2ValuePeriodically(uint16_t aCO2Value, const uint32_t aStoragePeriodSeconds) {
    /*
     * Compute the minimum of the values
     */
    if (sCO2MinimumOfCurrentReadings > aCO2Value) {
        sCO2MinimumOfCurrentReadings = aCO2Value; // Take minimum
    }

    if (now() < sNextStorageSeconds) {
        return false;
    } else {

        /*
         * One complete measurement period ended -> store data in array
         */
#if defined(LOCAL_DEBUG)
        Serial.print(F("CO2 (ppm)="));
        Serial.println(sCO2MinimumOfCurrentReadings);
#endif
        /*
         * convert to compressed 8 bit value and clip it at chart top
         */
        uint16_t tCO2Value = (sCO2MinimumOfCurrentReadings - CO2_BASE_VALUE) / CO2_COMPRESSION_FACTOR;

        /*
         * Write to array, clipping is done there
         */
        writeToCO2Array(tCO2Value);

        if (BlueDisplay1.isConnectionEstablished()) {
            handleEventAndFlags(); // Is required, if we are called first time after boot and handleEventAndFlags() was not called before
            drawCO2Chart();
        }

        // Prepare for next storage period
#if defined(LOCAL_DEBUG)
        Serial.print(F("sNextStorageMillis="));
        Serial.print(sNextStorageMillis);
        Serial.print(F(" aStoragePeriodMillis="));
        Serial.println(aStoragePeriodSeconds);
#endif
        sNextStorageSeconds += aStoragePeriodSeconds;
        sCO2MinimumOfCurrentReadings = __UINT16_MAX__;
        return true;
    }
}

/*
 * Array is in section .noinit
 * Therefore we can check the checksum and keep existing data before initializing it after reboot
 */
void initializeCO2Array() {
    uint16_t tCO2ArrayChecksum = 0;

    for (uint16_t i = 0; i < CO2_ARRAY_SIZE; ++i) {
        tCO2ArrayChecksum += sCO2Array[i];
    }
    if (tCO2ArrayChecksum == sCO2ArrayValuesChecksum) {
#if !defined(BD_USE_SIMPLE_SERIAL)
        Serial.println(F("Checksum match -> keep array"));
#else
        BlueDisplay1.debug(PSTR("Checksum match -> keep array"));
#endif
    } else {

        // checksum does not match -> initialize array
#if !defined(BD_USE_SIMPLE_SERIAL)
        Serial.print(F("Computed checksum "));
        Serial.print(tCO2ArrayChecksum);
        Serial.print(F(" does not match "));
        Serial.print(sCO2ArrayValuesChecksum);
        Serial.println(F(", assume power up -> initialize array and copy from EEPROM"));
#else
        BlueDisplay1.debug(PSTR("Checksum mismatch -> read from EEPROM"));
#endif

#if defined(E2END)
        uint16_t tSourceOffset = CO2_ARRAY_SIZE - sizeof(sCO2ArrayInEEPROM); // store only the most recent values
        // Fill the start of array with zero
        for (uint16_t i = 0; i < tSourceOffset; ++i) {
            sCO2Array[i] = 0;
        }
        /*
         * Now copy EEPROM content to remainder of array
         */
        if (eeprom_read_byte(&sCO2ArrayInEEPROM[sizeof(sCO2ArrayInEEPROM) - 1]) == 0xFF) {
            /*
             * Found an unused EEPROM (at first running this program on this CPU) -> clear it to zero
             */
//            Serial.println(F("Clear EEPROM to 0")); // this can hardly be observed
            for (unsigned int i = 0; i < CO2_ARRAY_SIZE; ++i) {
                eeprom_update_byte((uint8_t*) &sCO2ArrayInEEPROM[i], 0x00);
            }
        }
        eeprom_read_block(&sCO2Array[tSourceOffset], sCO2ArrayInEEPROM, sizeof(sCO2ArrayInEEPROM));

        tCO2ArrayChecksum = 0;
        for (uint16_t i = 0; i < CO2_ARRAY_SIZE; ++i) {
            tCO2ArrayChecksum += sCO2Array[i];
        }
#else
        // Fill the start of array with zero
        for (uint16_t i = 0; i < CO2_ARRAY_SIZE; ++i) {
            sCO2Array[i] = 0;
        }
        tCO2ArrayChecksum = 0;
        for (uint16_t i = 0; i < CO2_ARRAY_SIZE; ++i) {
            tCO2ArrayChecksum += sCO2Array[i];
        }
#endif
#if defined(ENABLE_STACK_ANALYSIS)
        printRAMInfo(&Serial);
#  if !defined(BD_USE_SIMPLE_SERIAL)
        Serial.flush();
#  endif
#endif
        sCO2ArrayValuesChecksum = tCO2ArrayChecksum;
    }
}

/*
 * aCO2Value is (CO2[ppm] - 400) / 5
 */
void writeToCO2Array(uint8_t aCO2Value) {

    // Clip at chart top
    if (aCO2Value > sChartMaxValue) {
        aCO2Value = sChartMaxValue;
    }
    sCO2ArrayValuesChecksum -= sCO2Array[0];  // adjust checksum with removed element
// Shift array to front
    for (uint16_t i = 0; i < CO2_ARRAY_SIZE - 1; ++i) {
        sCO2Array[i] = sCO2Array[i + 1];
    }
    sCO2Array[CO2_ARRAY_SIZE - 1] = aCO2Value;
    sCO2ArrayValuesChecksum += aCO2Value; // adjust checksum with new element
#if defined(LOCAL_DEBUG)
    Serial.print(F("Write "));
    Serial.print(aCO2Value);
#  if defined(LOCAL_TRACE)
    Serial.print(F(" Checksum="));
    Serial.print(sCO2ArrayValuesChecksum);
#  endif
    Serial.println();
#endif
}

/**************************************
 * BlueDisplay GUI related functions
 **************************************/
/*
 * This function is not called by event callback, it is called from setup or main loop
 * signalInitDisplay() is called by event callback, which only sets a flag for the main loop.
 * This helps reducing stack usage,
 * !!!but BlueDisplay1.isConnectionEstablished() cannot be used as indicator for initialized BD data,
 * without further handling, because initialized BD data may be delayed!!!
 */
void initDisplay(void) {
#if defined(LOCAL_DEBUG)
    Serial.println(F("InitDisplay"));
#endif

    uint16_t tDisplayHeight = (DISPLAY_WIDTH * BlueDisplay1.getHostDisplayHeight()) / BlueDisplay1.getHostDisplayWidth();
    if (tDisplayHeight < ((float) DISPLAY_WIDTH / 1.75)) {
        tDisplayHeight = (float) DISPLAY_WIDTH / 1.75;
    }
    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE, DISPLAY_WIDTH, tDisplayHeight);

// set layout variables;
    int tDisplayHeightEighth = tDisplayHeight / 8;

// here we have received a new local timestamp
    initLocalTimeHandling();

    BDButton::BDButtonPGMTextParameterStruct tBDButtonPGMParameterStruct; // Saves 480 Bytes for all 5 buttons

    BDButton::setInitParameters(&tBDButtonPGMParameterStruct, BUTTONS_START_X, BASE_TEXT_SIZE, (BASE_TEXT_SIZE * 4),
            tDisplayHeightEighth, DAY_BUTTONS_COLOR, F("4"), BASE_TEXT_SIZE_2, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 4 * HOURS_IN_ONE_DAY,
            &doDays);
    TouchButton4days.init(&tBDButtonPGMParameterStruct);

    /*
     * Draw 2 buttons right
     */
    tBDButtonPGMParameterStruct.aPositionX += (BASE_TEXT_SIZE * 4) + BASE_TEXT_SIZE_HALF; // gap of (BASE_TEXT_SIZE / 2)
    tBDButtonPGMParameterStruct.aValue = 2 * HOURS_IN_ONE_DAY; // 48
#if defined(__AVR__)
    tBDButtonPGMParameterStruct.aPGMText = F("2");
#else
    tBDButtonParameterStruct.aText = "2";
#endif
    TouchButton2days.init(&tBDButtonPGMParameterStruct);

    tBDButtonPGMParameterStruct.aPositionY += (tDisplayHeightEighth * 2);
    tBDButtonPGMParameterStruct.aValue = HOURS_IN_ONE_DAY / 2; // 12
    tBDButtonPGMParameterStruct.aPGMText = F("1/2");
    TouchButton12Hour.init(&tBDButtonPGMParameterStruct); // 14 bytes

    tBDButtonPGMParameterStruct.aPositionX = BUTTONS_START_X;
    tBDButtonPGMParameterStruct.aValue = HOURS_IN_ONE_DAY; // 24
    tBDButtonPGMParameterStruct.aPGMText = F("1");
    TouchButton1day.init(&tBDButtonPGMParameterStruct); // 14 bytes

    int tButtonYSpacing = tDisplayHeightEighth + BASE_TEXT_SIZE_HALF;

    tBDButtonPGMParameterStruct.aWidthX = BUTTON_WIDTH;
    tBDButtonPGMParameterStruct.aTextSize = BASE_TEXT_SIZE;
#if defined(E2END)
    tBDButtonPGMParameterStruct.aButtonColor = COLOR16_YELLOW;
    tBDButtonPGMParameterStruct.aPositionY += tButtonYSpacing;
    tBDButtonPGMParameterStruct.aOnTouchHandler = &doStoreInEEPROM;
    tBDButtonPGMParameterStruct.aPGMText = F("Store in\nEEPROM");
    TouchButtonStoreToEEPROM.init(&tBDButtonPGMParameterStruct);
#endif

    tBDButtonPGMParameterStruct.aButtonColor = COLOR16_LIGHT_GREY;
    tBDButtonPGMParameterStruct.aPositionY += tButtonYSpacing;
    tBDButtonPGMParameterStruct.aOnTouchHandler = &doSignalChangeBrightness;
    tBDButtonPGMParameterStruct.aPGMText = F("Refresh/\nBrightness");
    TouchButtonBrightness.init(&tBDButtonPGMParameterStruct);
    TouchButtonBrightness.setButtonTextColor(COLOR16_WHITE);

    /*
     * Button as invisible overlay to the time text.
     * Shows next time to storage as toast
     */
    tBDButtonPGMParameterStruct.aPositionY = BlueDisplay1.getRequestedDisplayHeight() - (4 * BASE_TEXT_SIZE);
    tBDButtonPGMParameterStruct.aOnTouchHandler = &doShowTimeToNextStorage;
    tBDButtonPGMParameterStruct.aPGMText = F("Next"); // not shown, just for debugging
    TouchButtonShowTimeToNextStorage.init(&tBDButtonPGMParameterStruct);

    sCurrentBrightness = BRIGHTNESS_LOW;
    changeBrightness(); // from low to high :-)
}

void drawDisplay() {
    BlueDisplay1.clearDisplay(sBackgroundColor);
    CO2Chart.drawYAxisTitle(-(int) BASE_TEXT_SIZE_2, CHART_START_X);
    drawCO2Chart();
    TouchButton4days.drawButton();
    TouchButton2days.drawButton();
    TouchButton1day.drawButton();
    TouchButton12Hour.drawButton();
    BlueDisplay1.drawText(BUTTONS_START_X + BASE_TEXT_SIZE_2, (BlueDisplay1.getRequestedDisplayHeight() / 4) - BASE_TEXT_SIZE,
            F("Day(s)"), BASE_TEXT_SIZE_1_5, DAY_BUTTONS_COLOR, sBackgroundColor);

#if defined(E2END)
    TouchButtonStoreToEEPROM.drawButton();
#endif
    TouchButtonBrightness.drawButton();
    TouchButtonShowTimeToNextStorage.activate(); // Just listen for touches
}

void setChartHoursToDisplay(int16_t aChartHoursToDisplay) {
    sChartHoursToDisplay = aChartHoursToDisplay;

    // defaults for every scale except 1/2 day
    uint8_t tXPixelSpacing = CHART_WIDTH / 8;  // 8 grid lines per chart
    uint8_t tXLabelDistance = 2; // draw label at every 2. grid line
    int8_t tXLabelScaleFactor;
    int8_t tDataFactor;
    if (aChartHoursToDisplay == (4 * HOURS_IN_ONE_DAY)) { //
        // Data compressed by 2
        sCO2ArrayDisplayStart = &sCO2Array[0];
        tDataFactor = CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_2;
        /*
         * 96 hours | 4 days -> with 8 grids at X axis => 1 grid line each 12 hours, label each 24 hours
         */
        tXLabelScaleFactor = CHART_X_AXIS_SCALE_FACTOR_1; // X label increment is 1/2 day at X label scale factor 1
    } else if (aChartHoursToDisplay == (2 * HOURS_IN_ONE_DAY)) {
        // Data direct
        sCO2ArrayDisplayStart = &sCO2Array[CO2_ARRAY_SIZE / 2];
        tDataFactor = CHART_X_AXIS_SCALE_FACTOR_1;
        /*
         * 48 hours | 2 days -> with 8 grids at X axis => 1 grid line each 6 hours, label each 12 hours => X scale expanded by 2
         */
        tXLabelScaleFactor = CHART_X_AXIS_SCALE_FACTOR_EXPANSION_2;
    } else if (aChartHoursToDisplay == HOURS_IN_ONE_DAY) {
        // data expanded by 2
        sCO2ArrayDisplayStart = &sCO2Array[(CO2_ARRAY_SIZE / 2) + (CO2_ARRAY_SIZE / 4)];
        tDataFactor = CHART_X_AXIS_SCALE_FACTOR_EXPANSION_2;
        /*
         * 24 hours |  day -> with 8 grids at X axis => 1 grid line each 3 hours, label each 6 hours => X scale expanded by 4
         */
        tXLabelScaleFactor = CHART_X_AXIS_SCALE_FACTOR_EXPANSION_4;
    } else {
        // Data expanded by 4
        sCO2ArrayDisplayStart = &sCO2Array[(CO2_ARRAY_SIZE / 2) + (CO2_ARRAY_SIZE / 4) + (CO2_ARRAY_SIZE / 8)];
        tDataFactor = CHART_X_AXIS_SCALE_FACTOR_EXPANSION_4;
        /*
         * 12 hour. Here the regular expansion would be 8, which leads to 1.5 hour per grid
         * So change the grid to 1 hour per grid and draw labels every 3 hours
         */
        tXPixelSpacing = CHART_WIDTH / 12; // 12 grid lines per chart
        tXLabelDistance = 3; // draw label at every 3. grid line, but draw big label still each 2x12 hours by keeping mXBigLabelDistance at 2
        tXLabelScaleFactor = CHART_X_AXIS_SCALE_FACTOR_EXPANSION_12; // 1 hour per grid
    }
    CO2Chart.setXLabelDistance(tXLabelDistance);
    CO2Chart.setXGridOrLabelPixelSpacing(tXPixelSpacing);
    CO2Chart.setXLabelScaleFactor(tXLabelScaleFactor);
    CO2Chart.setXDataScaleFactor(tDataFactor);
}

void doDays(BDButton *aTheTouchedButton, int16_t aChartHoursToDisplay) {
    (void) aTheTouchedButton;
    setChartHoursToDisplay(aChartHoursToDisplay);
    drawCO2Chart(); // this sets XLabelAndGridOffset
}

/*
 * Current time is at pixel position CHART_START_X + CHART_WIDTH
 * Touched time is at current time - (pixel_difference * X_data_scale * 5 min)
 */
void doShowTimeAtTouchPosition(struct TouchEvent *const aTouchPosition) {
    static struct XYPosition sLastPosition = { 0, 0 };
    uint16_t tPositionX = aTouchPosition->TouchPosition.PositionX;
    if (tPositionX >= CHART_START_X && tPositionX <= (CHART_START_X + CHART_WIDTH)) {
        uint16_t tPixelDifference = CO2Chart.reduceLongWithIntegerScaleFactor((CHART_START_X + CHART_WIDTH) - tPositionX,
                CO2Chart.getXDataScaleFactor());
        time_float_union tTimeOfTouchPosition;

#if defined(USE_C_TIME)
        tTimeOfTouchPosition.TimeValue = (BlueDisplay1.getHostUnixTimestamp()
                - (BlueDisplay1.getHostUnixTimestamp() % STORAGE_INTERVAL_SECONDS)) - (tPixelDifference * STORAGE_INTERVAL_SECONDS);
#else
        tTimeOfTouchPosition.TimeValue = (now() - (now() % STORAGE_INTERVAL_SECONDS))
                - (tPixelDifference * STORAGE_INTERVAL_SECONDS);
#endif

        char tTimeString[6];
        convertUnixTimestampToHourAndMinuteString(tTimeString, tTimeOfTouchPosition);
        BlueDisplay1.drawText(BUTTONS_START_X, TIME_MARKER_START_Y, tTimeString, BASE_TEXT_SIZE, CHART_DATA_COLOR,
                sBackgroundColor);
        // Clear last indicator
        if (sLastPosition.PositionX != 0) {
            BlueDisplay1.drawLineRel(sLastPosition.PositionX, sLastPosition.PositionY - 48, 0, 32, sBackgroundColor);
        }
        // Draw an indicator
        if (aTouchPosition->TouchPosition.PositionY > 48) {
            BlueDisplay1.drawLineRel(tPositionX, aTouchPosition->TouchPosition.PositionY - 48, 0, 32, CHART_DATA_COLOR);
            sLastPosition = aTouchPosition->TouchPosition;
        }
    }
}

void initCO2Chart() {
    /*
     * For CHART_X_AXIS_SCALE_FACTOR_1             and at 5 minutes / pixel we have 12 hours labels each 144 pixel.
     * For CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_2 and at 5 minutes / pixel we have 24 hours labels each 288 pixel.
     * For CHART_X_AXIS_SCALE_FACTOR_EXPANSION_2   and at 5 minutes / pixel we have  6 hours labels each  72 pixel.
     * SECONDS_IN_ONE_DAY/2 increment for label value and label distance is 2
     * 8 (12 for 1/2 day) grid lines per chart is set by doDays()
     */
    sCO2ArrayDisplayStart = sCO2Array;
    CO2Chart.initXLabelTimestampForLabelScaleIdentity(0, SECONDS_IN_ONE_DAY / 2, 5); // Increment is 1/2 day at X label scale factor 1
    // PowerChart.setXLabelDistance(2); // Draw label at every 2. grid line except for 1/2 day => Actual label distance is set by doDays()
    CO2Chart.setXBigLabelDistance(2);   // Show day date label bigger
    CO2Chart.setLabelStringFunction(convertUnixTimestampToHourString);

    CO2Chart.initYLabel(CO2_BASE_VALUE, CHART_Y_LABEL_INCREMENT, CO2_COMPRESSION_FACTOR, 4, 0); // 200 ppm per grid, 5 for input 1 per 5 ppm , 4 is minimum label string width

    uint16_t tChartHeight = BlueDisplay1.getRequestedDisplayHeight() - (BlueDisplay1.getRequestedDisplayHeight() / 4); // 3/4 display height
    uint16_t tYGridSpacing = (BlueDisplay1.getRequestedDisplayHeight() / 7); // 7 lines per screen
    sChartMaxValue = ((tChartHeight * (CHART_Y_LABEL_INCREMENT / CO2_COMPRESSION_FACTOR)) / (tYGridSpacing)) - 1;
    CO2Chart.initChart(CHART_START_X, CHART_START_Y, CHART_WIDTH, tChartHeight, CHART_AXES_SIZE, BASE_TEXT_SIZE, CHART_DISPLAY_GRID,
            0, tYGridSpacing); // GridOrLabelXPixelSpacing is set by doDays()

    CO2Chart.initChartColors(CHART_DATA_COLOR, CHART_AXES_COLOR, CHART_GRID_COLOR, sTextColor, sTextColor, sBackgroundColor);
    CO2Chart.setYTitleTextAndSize("ppm CO2", BASE_TEXT_SIZE_1_5);

    /*
     * Set values initially for whole buffer
     */
    setChartHoursToDisplay(NUMBER_OF_DAYS_IN_BUFFER * HOURS_IN_ONE_DAY);

    registerTouchDownCallback(doShowTimeAtTouchPosition);
    registerTouchMoveCallback(doShowTimeAtTouchPosition);
}

void drawCO2Chart() {
    printTimeAtTwoLines(BUTTONS_START_X, BlueDisplay1.getRequestedDisplayHeight() - ((7 * BASE_TEXT_SIZE) / 2), BASE_TEXT_SIZE,
            sTextColor, sBackgroundColor); // gets now()
    printCO2Value();
    /*
     * Print time and clear time marker area see doShowTimeAtTouchPosition()
     */
    BlueDisplay1.clearTextArea(BUTTONS_START_X, TIME_MARKER_START_Y, 5, BASE_TEXT_SIZE, sBackgroundColor);
    CO2Chart.clear();

    /*
     * Offset to origin for first main label (midnight) in seconds
     */
#if defined(USE_C_TIME)
    long tDifferenceToLastMidnightInSeconds = sTimeInfo->tm_sec + ((sTimeInfo->tm_min + (MINUTES_IN_ONE_HOUR * sTimeInfo->tm_hour)) * (long)SECONDS_IN_ONE_MINUTE);
#else
    long tDifferenceToLastMidnightInSeconds = second() + ((minute() + (MINUTES_IN_ONE_HOUR * hour())) * (long) SECONDS_IN_ONE_MINUTE);
#endif
    /*
     * We must always start with a midnight value to mark the start of the big labels
     * (+ SECONDS_PER_STORAGE) if we have midnight, the offset of the first label must be -1,
     * such that the midnight label is drawn at rightmost X axis position.
     * Because for data compression 2 one chart step is 2 SECONDS_PER_STORAGE,
     * we must expand it to 2 * SECONDS_PER_STORAGE for the effect.
     */
    long tCorrectedSecondsForStorage = CO2Chart.reduceLongWithIntegerScaleFactor(SECONDS_PER_STORAGE, CO2Chart.mXDataScaleFactor);
    if (sChartHoursToDisplay == (HOURS_IN_ONE_DAY / 2)) {
        // We want to see the labels of the last half day here, so midnight grid line is shifted left by 12 hours
        CO2Chart.setXLabelAndGridOffset(
                tDifferenceToLastMidnightInSeconds + (sChartHoursToDisplay * SECONDS_IN_ONE_HOUR) + tCorrectedSecondsForStorage);
    } else {
        CO2Chart.setXLabelAndGridOffset(tDifferenceToLastMidnightInSeconds + tCorrectedSecondsForStorage);
    }

    // Use 1,2, or 4 days back as start value
    // Timestamp of the midnight, which is left by tDifferenceToLastMidnightInSeconds of Y axis.
    uint8_t tFullDays = ((sChartHoursToDisplay - 1) / HOURS_IN_ONE_DAY) + 1; // Results in 1 for values <= 24
#if defined(USE_C_TIME)
    CO2Chart.drawXAxisAndDateLabels(
            BlueDisplay1.getHostUnixTimestamp() - ((tFullDays * SECONDS_IN_ONE_DAY) + tDifferenceToLastMidnightInSeconds),
            convertUnixTimestampToHourString);
#else
    CO2Chart.drawXAxisAndDateLabels(now() - ((tFullDays * SECONDS_IN_ONE_DAY) + tDifferenceToLastMidnightInSeconds),
            convertUnixTimestampToDateString);
#endif

    CO2Chart.drawYAxisAndLabels(); // this will restore the overwritten 400 label
    CO2Chart.drawGrid();
    CO2Chart.drawChartDataWithYOffset(sCO2ArrayDisplayStart, CO2_ARRAY_SIZE, CHART_MODE_LINE);
//        CO2Chart.drawChartDataWithYOffset(sCO2ArrayDisplayStart, CO2_ARRAY_SIZE, CHART_MODE_PIXEL);
}

/*
 * These function are called from main loop instead of delay()
 */
void delayMillisWithHandleEventAndFlags(unsigned long aDelayMillis) {
    unsigned long tStartMillis = millis();
    while (millis() - tStartMillis < aDelayMillis) {
        handleEventAndFlags();
    }
}

/*
 * End prematurely, if event received
 */
bool delayMillisWithCheckForEventAndFlags(unsigned long aDelayMillis) {
    if (delayMillisAndCheckForEvent(aDelayMillis)) {
        checkAndHandleCO2LoggerEventFlags();
        return true;
    }
    return false;
}

void checkAndHandleCO2LoggerEventFlags() {
    if (sDoInitDisplay) {
        sDoInitDisplay = false; // do it once
        setTime(BlueDisplay1.mHostUnixTimestamp); // set it before drawDisplay() -> drawCO2Chart() -> printTimeAtTwoLines()
        initDisplay();
        initCO2Chart();
        drawDisplay();
    }
    if (sDoRefreshOrChangeBrightness) {
        sDoRefreshOrChangeBrightness = false; // do it once
        if (millis() - sMillisOfLastRefreshOrChangeBrightness < TIMEOUT_FOR_BRIGHTNESS_MILLIS) {
            changeBrightness();
        }
        drawDisplay();
        sMillisOfLastRefreshOrChangeBrightness = millis();
    }
}

/*
 * To be called by setup or main loop
 * Calls delayed initDisplay() and changeBrightness() functions to save stack
 * Checks for BlueDisplay events and returns true if event happened,
 * which may introduce an unknown delay the program might not be able to handle
 */
bool handleEventAndFlags() {
    sBDEventJustReceived = false;
    checkAndHandleEvents(); // BlueDisplay function
    checkAndHandleCO2LoggerEventFlags(); // check
    return sBDEventJustReceived;
}

/*
 * GUI event handler section
 */

//This handler is called after boot or reconnect
void signalInitDisplay(void) {
    sDoInitDisplay = true;
}

void doSignalChangeBrightness(BDButton *aTheTouchedButton, int16_t aValue) {
    (void) aTheTouchedButton;
    (void) aValue;
    sDoRefreshOrChangeBrightness = true;
}

#if defined(E2END)
void doStoreInEEPROM(BDButton *aTheTouchedButton, int16_t aValue) {
    (void) aTheTouchedButton;
    (void) aValue;
// store only the most recent values
    eeprom_update_block(&sCO2Array[CO2_ARRAY_SIZE - sizeof(sCO2ArrayInEEPROM)], sCO2ArrayInEEPROM, sizeof(sCO2ArrayInEEPROM));
}
#endif

void doShowTimeToNextStorage(BDButton *aTheTouchedButton, int16_t aValue) {
    (void) aTheTouchedButton;
    (void) aValue;
#if defined(USE_C_TIME)
    BlueDisplay1.getInfo(SUBFUNCTION_GET_INFO_LOCAL_TIME, getTimeEventCallbackForLogger); // set
#else
    now(); // update sysTime and prevMillis to current time
#endif
    printTimeToNextStorage();
    printTimeAtTwoLines(BUTTONS_START_X, BlueDisplay1.getRequestedDisplayHeight() - ((7 * BASE_TEXT_SIZE) / 2), BASE_TEXT_SIZE,
            sTextColor, sBackgroundColor);
}

void printCO2Value() {
    char tStringBuffer[5];
    uint16_t tCO2Value = (sCO2Array[CO2_ARRAY_SIZE - 1] * CO2_COMPRESSION_FACTOR) + CO2_BASE_VALUE;
    snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("%4d"), tCO2Value);
    BlueDisplay1.drawText(CHART_START_X + (CHART_WIDTH / 2), BASE_TEXT_SIZE_HALF, tStringBuffer, 3 * BASE_TEXT_SIZE,
    CHART_DATA_COLOR, sBackgroundColor);
}

void changeBrightness() {
    if (sCurrentBrightness == BRIGHTNESS_HIGH) {
        // Set to dimmed background
        BlueDisplay1.setScreenBrightness(BD_SCREEN_BRIGHTNESS_MIN);
        sCurrentBrightness = BRIGHTNESS_MIDDLE;
    } else if (sCurrentBrightness == BRIGHTNESS_MIDDLE) {
        // Set to dark mode
        sBackgroundColor = COLOR16_LIGHT_GREY;
        sTextColor = COLOR16_WHITE;
        CO2Chart.setLabelColor(COLOR16_WHITE);
        CO2Chart.setBackgroundColor(COLOR16_LIGHT_GREY);
        sCurrentBrightness = BRIGHTNESS_LOW;
    } else {
        // Back to user brightness
        sBackgroundColor = COLOR16_WHITE;
        sTextColor = COLOR16_BLACK;
        BlueDisplay1.setScreenBrightness(BD_SCREEN_BRIGHTNESS_USER);
        CO2Chart.setLabelColor(COLOR16_BLACK);
        CO2Chart.setBackgroundColor(COLOR16_WHITE);
        sCurrentBrightness = BRIGHTNESS_HIGH;
    }
}

/*
 * Time handling
 */
/*
 * Print difference between next storage time and now(), which is set by caller :-)
 * Serial output is displayed as an Android toast
 */
void printTimeToNextStorage() {
#if defined(USE_C_TIME)
#warning printTimeToNextStorage() is not yet supported for usage of plain C time library
#else
//    int16_t tSecondsToNext = (sNextStorageMillis - millis()) / MILLIS_IN_ONE_SECOND;
    int16_t tSecondsToNext = sNextStorageSeconds - sysTime;

    char tStringBuffer[23];

    // give 10 seconds for internal delay / waiting for CO2 value
    if (tSecondsToNext < -10) {
        BlueDisplay1.debug(F("Timeout")); // signal, that seconds to next has an unexpected high negative value
    } else {
        if (tSecondsToNext < 0) {
            tSecondsToNext = 0;
        }

        uint8_t tMinutesToGo = tSecondsToNext / SECS_PER_MIN;
        uint8_t tSecondsToGo = tSecondsToNext % SECS_PER_MIN;
        snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("Next in %d min %02d sec"), tMinutesToGo, tSecondsToGo);
        // !!!!! What a bug! This gives x min 00 sec with 00 constant because of wrong type of SECS_PER_MIN !!!!!
//        snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("Next in %d min %02d sec"), tSecondsToNext / SECS_PER_MIN,
//                tSecondsToNext % SECS_PER_MIN);
        BlueDisplay1.debug(tStringBuffer); // Show as toast
//        Serial.print(F("Next in "));
//        if (tSecondsToNext / SECS_PER_MIN > 0) {
//            Serial.print(tSecondsToNext / SECS_PER_MIN);
//            Serial.print(F(" min "));
//        }
//        Serial.print(tSecondsToNext % SECS_PER_MIN);
//        Serial.println(F(" sec"));
    }
#endif
}

/*
 * It is called at startup and then every 24 hours.
 */
void getTimeEventCallbackForLogger(uint8_t aSubcommand, uint8_t aByteInfo, uint16_t aShortInfo, ByteShortLongFloatUnion aLongInfo) {
    (void) aSubcommand;
    (void) aByteInfo;
    (void) aShortInfo;

#if defined(USE_C_TIME)
        BlueDisplay1.setHostUnixTimestamp(aLongInfo.uint32Value);
        sTimeInfoWasJustUpdated = true;
        sTimeInfo = localtime((const time_t*) &aLongInfo.uint32Value); // Compute minutes, hours, day, month etc. for later use in printTime
#else

    setTime(aLongInfo.uint32Value);
    time_t tTimestamp = now(); // use this timestamp for display etc.

    /*
     * Now set sNextStorageMillis, so it is synchronized with clock
     */
#  if defined(STANDALONE_TEST) || defined(TEST)
        // start at 20, 40 seconds
    sNextStorageSeconds = tTimestamp + (20 - (second(tTimestamp) % 20));
#  else
    // tweak sNextStorageMillis, so that our next storage is at the next full 5 minute
    uint16_t tSecondsUntilNextFull5Minutes = (STORAGE_INTERVAL_SECONDS
            - ((minute(tTimestamp) * SECS_PER_MIN) + second(tTimestamp)) % STORAGE_INTERVAL_SECONDS);
    sNextStorageSeconds = tTimestamp + tSecondsUntilNextFull5Minutes;
#  endif
#endif

    delay(400); // wait for the scale factor toast to vanish
    printTimeToNextStorage();
#if defined(LOCAL_TRACE)
    Serial.print(sTimestamp);
    Serial.print('|');
    Serial.println(sNextStorageSeconds);
#endif
}

#if defined(LOCAL_DEBUG)
#undef LOCAL_DEBUG
#endif
#endif // _CO2_LOGGER_AND_CHART_HPP
