/*
 *  CO2LoggerAndChart.hpp
 *  Stores CO2 values in an array and show them on a Android mobile or tablet running the BlueFisplay app.
 *
 *  Values are stored as 8 bits with 0 = 400 ppm, 1=405, 2=410, 20=500, 60=700, 80=800, 100=900, 120=1000, 200=1400,
 *  Each 5 minutes the minimum of the snooped values is stored. I.e. 1 hour has 12 values, 72-> 6 hours, 288->1 day, 1152->4 days, 1440->5 days
 *
 *
 *  Copyright (C) 2024-2025  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#ifndef _CO2_LOGGER_AND_CHART_HPP
#define _CO2_LOGGER_AND_CHART_HPP

//#define DO_NOT_NEED_BASIC_TOUCH_EVENTS    // Disables basic touch events down, move and up. Saves 620 bytes program memory and 36 bytes RAM
#define DO_NOT_NEED_LONG_TOUCH_DOWN_AND_SWIPE_EVENTS    // Disables LongTouchDown and SwipeEnd events.
#define ONLY_CONNECT_EVENT_REQUIRED                     // Disables reorientation, redraw and SensorChange events
#include "Chart.hpp"

//#define LOCAL_DEBUG
//#define ENABLE_STACK_ANALYSIS
#if defined(ENABLE_STACK_ANALYSIS)
#include "AVRUtils.h" // include for initStackFreeMeasurement() and printRAMInfo()
#include "AVRTracing.h"
#endif

/*
 * Scale the screen such, that this fit horizontally.
 * Border - YLabels - Chart with CO2_ARRAY_SIZE / 2 - Border - Buttons for 6 big characters - Border
 * Values for CO2_ARRAY_SIZE = 1152
 */
#define CHART_WIDTH     ((CO2_ARRAY_SIZE) / 2)          // 576
#define DISPLAY_WIDTH   ((CO2_ARRAY_SIZE * 33) / 40)    // 950,4
#define BASE_TEXT_SIZE  (CO2_ARRAY_SIZE / 40)           // 1152 / 40 = 28.8
#define BUTTON_WIDTH    ((CO2_ARRAY_SIZE * 4) / 20)     // 230,4
#define CHART_START_X   (BASE_TEXT_SIZE * 3)            // 84
#define CHART_AXES_SIZE (BASE_TEXT_SIZE / 8)            // 10,5
#define BUTTONS_START_X ((BASE_TEXT_SIZE * 4) + CHART_WIDTH)
#define CHART_Y_LABEL_INCREMENT 200L

#define CHART_BACKGROUND_COLOR  COLOR16_WHITE
#define CHART_DATA_COLOR        COLOR16_RED
#define CHART_AXES_COLOR        COLOR16_BLUE
#define CHART_GRID_COLOR        COLOR16_GREEN
#define CHART_DATA_COLOR        COLOR16_RED
#define CHART_TEXT_COLOR        COLOR16_BLACK

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
bool sDoChangeBrightness = false; // used to call changeBrightness on main loop which does save stack space.
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
 * Even with initDisplay() called from main loop we only have 1140 bytes available for application
 */
#define CO2_ARRAY_SIZE  1152L // 1152-> 4 days, 1440->5 days at 5 minutes / sample
//#define CO2_ARRAY_SIZE  1440 // 1152-> 4 days, 1440->5 days at 5 minutes / sample
#define NUMBER_OF_DAYS_IN_BUFFER    4
uint16_t sCO2ArrayValuesChecksum __attribute__((section(".noinit")));  // must be in noinit, otherwise it is set to 0 at boot
uint8_t sCO2Array[CO2_ARRAY_SIZE] __attribute__((section(".noinit"))); // values are (CO2[ppm] - 400) / 5
uint16_t sHoursPerChartToDisplay __attribute__((section(".noinit")));  // is initialized to 96 if checksum is wrong
uint16_t sCO2MinimumOfCurrentReadings; // the minimum of all values received during one period
/*
 * Layout values for fullscreen GUI
 */
uint8_t sChartMaxValue; // For clipping
/*
 * We do not have enough resolution/pixel for 1152 pixel of chart + label.
 * So we use 576 pixel chart with compressed display of 10 minutes per pixel for 4 days display
 * and 5 minutes per pixel for 2 days (1:1 resolution).
 */
uint8_t *sCO2ArrayDisplayStart = sCO2Array;

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
void doDays(BDButton *aTheTouchedButton, int16_t aHoursPerChartToDisplay);
void doSignalChangeBrightness(BDButton *aTheTouchedButton, int16_t aValue);
void doShowTimeToNextStorage(BDButton *aTheTouchedButton, int16_t aValue);

bool handleEventAndFlags();
void checkAndHandleCO2LoggerEventFlags();

void printCO2Value();
void changeBrightness();

/*
 * Date and time handling for X scale
 */
#if defined(ESP32) || defined(ESP8266)
#define USE_C_TIME
#define WAIT_FOR_TIME_SYNC_MAX_MILLIS   150
#endif

#if defined(USE_C_TIME)
#include "time.h"
struct tm *sTimeInfo;
uint32_t prevMillis = 0; // To be compatible with TimeLib.h
bool sTimeInfoWasJustUpdated = false;
uint16_t waitUntilTimeWasUpdated(uint16_t aMaxWaitMillis);
/* Useful Constants */
#define SECS_PER_MIN  ((time_t)(60UL))
#define SECS_PER_HOUR ((time_t)(3600UL))
#define SECS_PER_DAY  ((time_t)(SECS_PER_HOUR * 24UL))
//#define DAYS_PER_WEEK ((time_t)(7UL))
//#define SECS_PER_WEEK ((time_t)(SECS_PER_DAY * DAYS_PER_WEEK))
//#define SECS_PER_YEAR ((time_t)(SECS_PER_DAY * 365UL)) // TODO: ought to handle leap years
//#define SECS_YR_2000  ((time_t)(946684800UL)) // the time at the start of y2k
#else
#include "TimeLib.h"
#endif
#if ! defined(USE_C_TIME)
time_t requestHostUnixTimestamp();
#endif
#define TIME_ADJUSTMENT 0
//#define TIME_ADJUSTMENT SECS_PER_HOUR // To be subtracted from received timestamp
#define MILLIS_IN_ONE_SECOND 1000L
#define STORAGE_INTERVAL_SECONDS (5 * SECS_PER_MIN)
// sNextStorageMillis is required by CO2LoggerAndChart.hpp for toast at connection startup
uint32_t sNextStorageMillis = 60 * MILLIS_IN_ONE_SECOND; // If not connected first storage in 1 after boot.

void printTime();
void getTimeEventCallback(uint8_t aSubcommand, uint8_t aByteInfo, uint16_t aShortInfo, ByteShortLongFloatUnion aLongInfo);
int convertUnixTimestampToDateString(char *aLabelStringBuffer, time_float_union aXvalue);
int convertUnixTimestampToHourString(char *aLabelStringBuffer, time_float_union aXvalue);
int convertUnixTimestampToHourAndMinuteString(char *aLabelStringBuffer, time_float_union aXvalue);
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
     * Register callback handler and wait for 300 ms if Bluetooth connection is still active.
     * For ESP32 and after power on of the Bluetooth module (HC-05) at other platforms, Bluetooth connection is most likely not active here.
     *
     * If active, mCurrentDisplaySize and mHostUnixTimestamp are set and initDisplay() and drawGui() functions are called.
     * If not active, the periodic call of checkAndHandleEvents() in the main loop waits for the (re)connection and then performs the same actions.
     */
#if defined(BD_USE_SIMPLE_SERIAL)
    BlueDisplay1.initCommunication(&signalInitDisplay); // introduces up to 1.5 seconds delay
#else
    uint16_t tConnectDurationMillis = BlueDisplay1.initCommunication(&signalInitDisplay); // introduces up to 1.5 seconds delay
    if (tConnectDurationMillis > 0) {
        Serial.print("Connection established after ");
        Serial.print(tConnectDurationMillis);
        Serial.println(" ms");
    } else {
        Serial.println(F("No connection after " STR(CONNECTIOM_TIMEOUT_MILLIS) " ms"));
    }
#endif
    handleEventAndFlags(); // To process the (delayed) time event and flags set by the events received at init

// Simulate a connection for testing
//    BlueDisplay1.mHostDisplaySize.XWidth = 1280;
//    BlueDisplay1.mHostDisplaySize.YHeight = 800;
//    BlueDisplay1.mHostUnixTimestamp = 1733191057;
//    BlueDisplay1.mBlueDisplayConnectionEstablished = true;
//    sDoInitDisplay = true;

    initializeCO2Array(); // uses BlueDisplay1.debug();

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

    sCO2MinimumOfCurrentReadings = __UINT16_MAX__;
}

/*
 * Is intended to be called by main loop
 */
bool storeCO2ValuePeriodically(uint16_t aCO2Value, const uint32_t aStoragePeriodMillis) {
    /*
     * Compute the minimum of the values
     */
    if (sCO2MinimumOfCurrentReadings > aCO2Value) {
        sCO2MinimumOfCurrentReadings = aCO2Value; // Take minimum
    }

    if (millis() < sNextStorageMillis) {
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
        Serial.println(aStoragePeriodMillis);
#endif
        sNextStorageMillis += aStoragePeriodMillis;
        sCO2MinimumOfCurrentReadings = __UINT16_MAX__;
        return true;
    }
}

/*
 * Array is in section .noinit
 * Therefore check checksum before initializing it after reboot
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

        // Must be initialized here
        sHoursPerChartToDisplay = 96;

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
    sCO2ArrayValuesChecksum -= sCO2Array[0]; // adjust checksum
// Shift array to front
    for (int i = 0; i < CO2_ARRAY_SIZE - 1; ++i) {
        sCO2Array[i] = sCO2Array[i + 1];
    }
    sCO2Array[CO2_ARRAY_SIZE - 1] = aCO2Value;
    sCO2ArrayValuesChecksum += aCO2Value; // adjust checksum
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
 * This function is not called by event callback, it is called from main loop
 * signalInitDisplay() is called by event callback, which only sets a flag for the main loop.
 * This helps reducing stack usage,
 * !!!but BlueDisplay1.isConnectionEstablished() cannot be used as indicator for initialized BD data, without further handling!!!
 */
void initDisplay(void) {
#if defined(LOCAL_DEBUG)
    Serial.println(F("InitDisplay"));
#endif

    uint16_t tDisplayHeight = (DISPLAY_WIDTH * BlueDisplay1.getHostDisplayHeight()) / BlueDisplay1.getHostDisplayWidth();
    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE, DISPLAY_WIDTH, tDisplayHeight);

// set layout variables;
    int tDisplayHeightEighth = tDisplayHeight / 8;

// here we have received a new local timestamp
#if defined(USE_C_TIME)
// initialize sTimeInfo
    sTimeInfo = localtime((const time_t*) BlueDisplay1.getHostUnixTimestamp());
#else
    setSyncProvider(requestHostUnixTimestamp); // This always calls getInfo() immediately, which will set time again.
// Set function to call when time sync required (default: after 300 seconds)
#  if defined(STANDALONE_TEST)
    setSyncInterval(SECS_PER_WEEK); // sync with host once per week
#  else
    setSyncInterval(SECS_PER_DAY); // sync with host once per day
#  endif
    setTime(BlueDisplay1.getHostUnixTimestamp() - TIME_ADJUSTMENT); // safety net, because we need a reasonable timestamp for initCO2Chart().
#endif
    delayMillisAndCheckForEvent(150); // Wait for requested time event but terminate at least after 150 ms

    BDButton::BDButtonPGMParameterStruct tBDButtonPGMParameterStruct; // Saves 480 Bytes for all 5 buttons

    BDButton::setInitParameters(&tBDButtonPGMParameterStruct, BUTTONS_START_X, BASE_TEXT_SIZE, (BASE_TEXT_SIZE * 4),
            tDisplayHeightEighth, COLOR16_GREEN, F("4"), BASE_TEXT_SIZE * 2, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 96, &doDays);
    TouchButton4days.init(&tBDButtonPGMParameterStruct);

    /*
     * Draw 2 buttons right
     */
    tBDButtonPGMParameterStruct.aPositionX += (BASE_TEXT_SIZE * 4) + (BASE_TEXT_SIZE / 2); // gap of (BASE_TEXT_SIZE / 2)
    tBDButtonPGMParameterStruct.aValue = 48;
    tBDButtonPGMParameterStruct.aPGMText = F("2");
    TouchButton2days.init(&tBDButtonPGMParameterStruct);

    tBDButtonPGMParameterStruct.aPositionY += (tDisplayHeightEighth * 2);
    tBDButtonPGMParameterStruct.aValue = 12;
    tBDButtonPGMParameterStruct.aPGMText = F("1/2");
    TouchButton12Hour.init(&tBDButtonPGMParameterStruct); // 14 bytes

    tBDButtonPGMParameterStruct.aPositionX = BUTTONS_START_X;
    tBDButtonPGMParameterStruct.aValue = 24;
    tBDButtonPGMParameterStruct.aPGMText = F("1");
    TouchButton1day.init(&tBDButtonPGMParameterStruct); // 14 bytes

    int tButtonYSpacing = tDisplayHeightEighth + (BASE_TEXT_SIZE / 2);

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
    tBDButtonPGMParameterStruct.aPGMText = F("Brightness");
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

void doDays(BDButton *aTheTouchedButton, int16_t aHoursPerChartToDisplay) {
    (void) aTheTouchedButton;
    sHoursPerChartToDisplay = aHoursPerChartToDisplay;
    /*
     * X increment is 1/2 day (for scale factor 1)
     * Chart: 4 days -> CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_2 (-2), 2 days -> CHART_X_AXIS_SCALE_FACTOR_1 (0), 1 day -> CHART_X_AXIS_SCALE_FACTOR_EXPANSION_2 (2)
     * 4->-2,2->0, 1->2, 1/2->4
     * Normal chart: 4->0, 2->2, 1->4, 1/2->8
     */
    int8_t tXLabelScaleFactor = CHART_X_AXIS_SCALE_FACTOR_1; // X increment is 1/2 day (for scale factor 1)
    int8_t tDataFactor = CHART_X_AXIS_SCALE_FACTOR_1;
    uint8_t tXPixelSpacing = CHART_WIDTH / 8;  // 8 grid lines per chart
    uint8_t tXLabelDistance = 2; // // draw label at every 2. grid line
    if (aHoursPerChartToDisplay == 96) {
        // 4 days. Grid each 12 hours, label each 24 hours, data compressed by 2
        sCO2ArrayDisplayStart = sCO2Array;
        tDataFactor = CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_2;
    } else if (aHoursPerChartToDisplay == 48) {
        // 2 days. Grid each 6 hours, label each 12 hours => scale expanded by 2, data direct
        sCO2ArrayDisplayStart = &sCO2Array[CO2_ARRAY_SIZE / 2];
        tXLabelScaleFactor = CHART_X_AXIS_SCALE_FACTOR_EXPANSION_2;
    } else if (aHoursPerChartToDisplay == 24) {
        // 1 day. Grid each 3 hours, label each 6 hours => scale expanded by 4, data expanded by 2
        sCO2ArrayDisplayStart = &sCO2Array[(CO2_ARRAY_SIZE / 2) + (CO2_ARRAY_SIZE / 4)];
        tDataFactor = CHART_X_AXIS_SCALE_FACTOR_EXPANSION_2;
        tXLabelScaleFactor = CHART_X_AXIS_SCALE_FACTOR_EXPANSION_4;
    } else {
        // 12 hour. Grid each hour, label each 3 hours => grid is narrower and scale expanded by 12, data expanded by 4
        sCO2ArrayDisplayStart = &sCO2Array[(CO2_ARRAY_SIZE / 2) + (CO2_ARRAY_SIZE / 4) + (CO2_ARRAY_SIZE / 8)];
        tDataFactor = CHART_X_AXIS_SCALE_FACTOR_EXPANSION_4;
        /*
         * Here the regular expansion would be 8, which leads to 1.5 hour per grid
         * So change the grid to 1 hour per grid and draw labels every 3 hours
         */
        tXLabelScaleFactor = CHART_X_AXIS_SCALE_FACTOR_EXPANSION_12; // 1 hour per grid
        tXPixelSpacing = CHART_WIDTH / 12; // 12 grid lines per chart
        tXLabelDistance = 3; // draw label at every 3. grid line, but draw big label still each 2x12 hours by keeping mXBigLabelDistance at 2
    }
    CO2Chart.setXLabelDistance(tXLabelDistance);
    CO2Chart.setGridOrLabelXPixelSpacing(tXPixelSpacing);
    CO2Chart.setXLabelScaleFactor(tXLabelScaleFactor);
    CO2Chart.setXDataScaleFactor(tDataFactor);
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
        tTimeOfTouchPosition.TimeValue = (now() - (now() % STORAGE_INTERVAL_SECONDS))
                - (tPixelDifference * STORAGE_INTERVAL_SECONDS);
        char tTimeString[6];
        convertUnixTimestampToHourAndMinuteString(tTimeString, tTimeOfTouchPosition);
        BlueDisplay1.drawText(BUTTONS_START_X, BlueDisplay1.getRequestedDisplayHeight() - BASE_TEXT_SIZE / 2, tTimeString,
        BASE_TEXT_SIZE, CHART_DATA_COLOR, sBackgroundColor);
        // Clear last indicator
        if (sLastPosition.PositionX != 0) {
            BlueDisplay1.drawLineRel(sLastPosition.PositionX, sLastPosition.PositionY + 32, 0, 32, sBackgroundColor);
        }
        // Draw an indicator
        BlueDisplay1.drawLineRel(tPositionX, aTouchPosition->TouchPosition.PositionY + 32, 0, 32, CHART_DATA_COLOR);
        sLastPosition = aTouchPosition->TouchPosition;
    }
}

void initCO2Chart() {
    /*
     * For CHART_X_AXIS_SCALE_FACTOR_1             and at 5 minutes / pixel we have 12 hours labels each 144 pixel -> 8 grid lines.
     * For CHART_X_AXIS_SCALE_FACTOR_COMPRESSION_2 and at 5 minutes / pixel we have 24 hours labels each 288 pixel.
     * For CHART_X_AXIS_SCALE_FACTOR_EXPANSION_2   and at 5 minutes / pixel we have  6 hours labels each  72 pixel.
     * SECS_PER_DAY/2 increment for label value and label distance is 2
     */
    sCO2ArrayDisplayStart = sCO2Array;
// Increment is 1/2 day for scale factor 1
    CO2Chart.initXLabelTimestamp(0, SECS_PER_DAY / 2, CHART_X_AXIS_SCALE_FACTOR_1, 5);
    CO2Chart.setXBigLabelDistance(2); // regular label distance is set by doDays() below
    CO2Chart.setLabelStringFunction(convertUnixTimestampToHourString);

    CO2Chart.initYLabel(CO2_BASE_VALUE, CHART_Y_LABEL_INCREMENT, CO2_COMPRESSION_FACTOR, 4, 0); // 200 ppm per grid, 5 for input 1 per 5 ppm , 4 is minimum label string width

    uint16_t tChartHeight = BlueDisplay1.getRequestedDisplayHeight() - (BlueDisplay1.getRequestedDisplayHeight() / 4); // 3/4 display height
    uint16_t tYGridSize = (BlueDisplay1.getRequestedDisplayHeight() / 7);
    sChartMaxValue = ((tChartHeight * (CHART_Y_LABEL_INCREMENT / CO2_COMPRESSION_FACTOR)) / (tYGridSize));
    // Grid spacing is CHART_WIDTH / 8 -> 8 columns and height / 6 for 5 lines from 400 to 1400
    CO2Chart.initChart(CHART_START_X, BlueDisplay1.getRequestedDisplayHeight() - (BASE_TEXT_SIZE + (BASE_TEXT_SIZE / 2)),
    CHART_WIDTH, tChartHeight, CHART_AXES_SIZE, BASE_TEXT_SIZE, CHART_DISPLAY_GRID, 0, tYGridSize); // GridOrLabelXPixelSpacing is set by doDays() below

    CO2Chart.initChartColors(CHART_DATA_COLOR, CHART_AXES_COLOR, CHART_GRID_COLOR, sTextColor, sTextColor, sBackgroundColor);
    CO2Chart.setYTitleTextAndSize("ppm CO2", BASE_TEXT_SIZE + (BASE_TEXT_SIZE / 2));

    /*
     * Set values depending on sHoursPerChartToDisplay
     */
    doDays(nullptr, sHoursPerChartToDisplay);

    registerTouchDownCallback(doShowTimeAtTouchPosition);
    registerTouchMoveCallback(doShowTimeAtTouchPosition);
}

void drawDisplay() {
    BlueDisplay1.clearDisplay(sBackgroundColor);
    CO2Chart.drawYAxisTitle(-(2 * BASE_TEXT_SIZE));
    drawCO2Chart();
    TouchButton4days.drawButton();
    TouchButton2days.drawButton();
    TouchButton1day.drawButton();
    TouchButton12Hour.drawButton();
    BlueDisplay1.drawText(BUTTONS_START_X + (2 * BASE_TEXT_SIZE),
            (BlueDisplay1.getRequestedDisplayHeight() / 4) + (BASE_TEXT_SIZE / 8), F("Day(s)"),
            BASE_TEXT_SIZE + (BASE_TEXT_SIZE / 2), COLOR16_GREEN, sBackgroundColor);

#if defined(E2END)
    TouchButtonStoreToEEPROM.drawButton();
#endif
    TouchButtonBrightness.drawButton();
    TouchButtonShowTimeToNextStorage.activate(); // Just listen for touches
}

void drawCO2Chart() {
    printTime(); // gets now()
    printCO2Value();
    // Clear time of marker area
    BlueDisplay1.clearTextArea(BUTTONS_START_X, BlueDisplay1.getRequestedDisplayHeight() - BASE_TEXT_SIZE / 2, 5, BASE_TEXT_SIZE,
            sBackgroundColor);
    CO2Chart.clear();

    /*
     * Offset to Y axis for first main label (midnight) in seconds
     */
#if defined(USE_C_TIME)
    long tDifferenceToLastMidnightInSeconds = (sTimeInfo->tm_min + (60 * sTimeInfo->tm_hour)) * SECS_PER_MIN;
#else
    long tDifferenceToLastMidnightInSeconds = (minute() + (60 * hour())) * SECS_PER_MIN;
#endif
    /*
     * We must always start with a midnight value to mark the start of the big labels
     */
    if (sHoursPerChartToDisplay == 12) {
        // We want to see the labels of the last half day here, so midnight grid line is shifted left by 12 hours
        CO2Chart.setXLabelAndGridOffset(tDifferenceToLastMidnightInSeconds + (sHoursPerChartToDisplay * SECS_PER_HOUR));
    } else {
        CO2Chart.setXLabelAndGridOffset(tDifferenceToLastMidnightInSeconds);
    }

    // Use 1,2, or 4 days back as start value
    // Timestamp of the midnight, which is left by tDifferenceToLastMidnightInSeconds of Y axis.
    uint8_t tFullDays =
            ((sHoursPerChartToDisplay - 1) / 24) + 1; // Results in 1 for values <= 24
#if defined(USE_C_TIME)
    CO2Chart.drawXAxisAndDateLabels(BlueDisplay1.getHostUnixTimestamp() - ((tFullDays * SECS_PER_DAY) + tDifferenceToLastMidnightInSeconds),
            convertUnixTimestampToHourString);
#else
    CO2Chart.drawXAxisAndDateLabels(now() - ((tFullDays * SECS_PER_DAY) + tDifferenceToLastMidnightInSeconds),
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
        initDisplay();
        initCO2Chart();
        drawDisplay(); // Display is initialized here
    }
    if (sDoChangeBrightness) {
        sDoChangeBrightness = false; // do it once
        changeBrightness();
        drawDisplay();
    }
}

/*
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
    sDoChangeBrightness = true;
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
    BlueDisplay1.getInfo(SUBFUNCTION_GET_INFO_LOCAL_TIME, &getTimeEventCallback); // set
#else
    now(); // set prevMillis to current time
#endif
    printTimeToNextStorage();
    printTime();
}

void printCO2Value() {
    char tStringBuffer[5];
    uint16_t tCO2Value = (sCO2Array[CO2_ARRAY_SIZE - 1] * CO2_COMPRESSION_FACTOR) + CO2_BASE_VALUE;
    snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("%4d"), tCO2Value);
    BlueDisplay1.drawText(CHART_START_X + (CHART_WIDTH / 2), 3 * BASE_TEXT_SIZE, tStringBuffer, 3 * BASE_TEXT_SIZE,
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
    int16_t tSecondsToNext = (sNextStorageMillis - prevMillis) / MILLIS_IN_ONE_SECOND;
    char tStringBuffer[23];

    // give 10 seconds for internal delay / waiting for CO2 value
    if (tSecondsToNext < -10) {
        BlueDisplay1.debug(PSTR("Timeout"));
    } else {
        if (tSecondsToNext < 0) {
            tSecondsToNext = 0;
        }

        uint8_t tMinutesToGo = tSecondsToNext / SECS_PER_MIN;
        uint8_t tSecondsToGo = tSecondsToNext % SECS_PER_MIN;
        snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("Next in %d min %02d sec"), tMinutesToGo, tSecondsToGo);
        // !!!!! What a bug! This gives x min 00 sec with 00 constant !!!!!
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
}

/*
 * Here we take the long part of the value and return strings like "27" or "2:27"
 */
int convertUnixTimestampToDateString(char *aLabelStringBuffer, time_float_union aXvalue) {
#if defined(USE_C_TIME)
    sTimeInfo = localtime((const time_t*) aXvalue.TimeValue);
    return snprintf(aLabelStringBuffer, CHART_LABEL_STRING_BUFFER_SIZE, "%d.%d", sTimeInfo->tm_mday, sTimeInfo->tm_mon);
#else
    return snprintf(aLabelStringBuffer, CHART_LABEL_STRING_BUFFER_SIZE, "%d.%d", day(aXvalue.TimeValue), month(aXvalue.TimeValue));
#endif
}

int convertUnixTimestampToHourString(char *aLabelStringBuffer, time_float_union aXvalue) {
#if defined(USE_C_TIME)
    sTimeInfo = localtime((const time_t*) aXvalue.TimeValue);
    return snprintf(aLabelStringBuffer, CHART_LABEL_STRING_BUFFER_SIZE, "%d", sTimeInfo->tm_hour);
#else
    return snprintf(aLabelStringBuffer, CHART_LABEL_STRING_BUFFER_SIZE, "%d", hour(aXvalue.TimeValue));
#endif
}

int convertUnixTimestampToHourAndMinuteString(char *aLabelStringBuffer, time_float_union aXvalue) {
#if defined(USE_C_TIME)
    sTimeInfo = localtime((const time_t*) aXvalue.TimeValue);
    return snprintf(aLabelStringBuffer, CHART_LABEL_STRING_BUFFER_SIZE, "%2d:%02d", sTimeInfo->tm_hour, sTimeInfo->tm_minute);
#else
    return snprintf(aLabelStringBuffer, CHART_LABEL_STRING_BUFFER_SIZE, "%2d:%02d", hour(aXvalue.TimeValue),
            minute(aXvalue.TimeValue));
#endif
}

void printTime() {
    char tStringBuffer[20];
#if defined(USE_C_TIME)
    BlueDisplay1.getInfo(SUBFUNCTION_GET_INFO_LOCAL_TIME, &getTimeEventCallback);
#  if defined(DEBUG)
    uint16_t tElapsedTime = waitUntilTimeWasUpdated(WAIT_FOR_TIME_SYNC_MAX_MILLIS); // 150 ms
    BlueDisplay1.debug("Time sync lasts ", tElapsedTime, " ms"); // timeout if 0
#  else
    waitUntilTimeWasUpdated(WAIT_FOR_TIME_SYNC_MAX_MILLIS); // 150 ms
#  endif
    snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("%02d.%02d.%4d %02d:%02d:%02d"), sTimeInfo->tm_mday, sTimeInfo->tm_mon,
            sTimeInfo->tm_year + 1900, sTimeInfo->tm_hour, sTimeInfo->tm_min, sTimeInfo->tm_sec);
#else
    time_t tTimestamp = now(); // use this timestamp for all prints
// 1600 byte code size for time handling plus print
// We can ignore warning: '%02d' directive writing between 2 and 11 bytes into a region of size between 0 and 1 [-Wformat-overflow=]
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-overflow"
    snprintf_P(tStringBuffer, sizeof(tStringBuffer), PSTR("%02d.%02d.%4d\n%02d:%02d:%02d"), day(tTimestamp), month(tTimestamp),
            year(tTimestamp), hour(tTimestamp), minute(tTimestamp), second(tTimestamp));
#pragma GCC diagnostic pop
#endif //defined(USE_C_TIME)
    BlueDisplay1.drawText(BUTTONS_START_X, BlueDisplay1.getRequestedDisplayHeight() - (3 * BASE_TEXT_SIZE), tStringBuffer,
    BASE_TEXT_SIZE, sTextColor, sBackgroundColor);
}

#if defined(USE_C_TIME)
/*
 * @return 0 if time was not updated in aMaxWaitMillis milliseconds, else millis used for update
 */
uint16_t waitUntilTimeWasUpdated(uint16_t aMaxWaitMillis) {
    unsigned long tStartMillis = millis();
    while (millis() - tStartMillis < aMaxWaitMillis) {
        checkAndHandleEvents(); // BlueDisplay function, which may call getTimeEventCallback() if SUBFUNCTION_GET_INFO_LOCAL_TIME event received
        if (sTimeInfoWasJustUpdated) {
            return millis() - tStartMillis;
        }
    }
    return 0;
}
/*
 * Is called at startup and every time the actual time is required
 */
void getTimeEventCallback(uint8_t aSubcommand, uint8_t aByteInfo, uint16_t aShortInfo, ByteShortLongFloatUnion aLongInfo) {
    if (aSubcommand == SUBFUNCTION_GET_INFO_LOCAL_TIME) {
        BlueDisplay1.setHostUnixTimestamp(aLongInfo.uint32Value);
        prevMillis = millis();
        sTimeInfoWasJustUpdated = true;
        sTimeInfo = localtime((const time_t*) &aLongInfo.uint32Value); // Compute minutes, hours, day, month etc.
    }
}
#else
/*
 * Is called at startup and every 24 hour
 */
void getTimeEventCallback(uint8_t aSubcommand, uint8_t aByteInfo, uint16_t aShortInfo, ByteShortLongFloatUnion aLongInfo) {
    (void) aByteInfo;
    (void) aShortInfo;
    if (aSubcommand == SUBFUNCTION_GET_INFO_LOCAL_TIME) { // safety net

        setTime(aLongInfo.uint32Value - TIME_ADJUSTMENT);
//        setTime(BlueDisplay1.getHostUnixTimestamp() - TIME_ADJUSTMENT);
        time_t tTimestamp = now(); // use this timestamp for display etc.

        /*
         * Now set sNextStorageMillis, so it is synchronized with clock
         */
#  if defined(STANDALONE_TEST) || defined(TEST)
        // start at 20, 40 seconds
        sNextStorageMillis = millis() + ((20 - (second(tTimestamp) % 20)) * MILLIS_IN_ONE_SECOND);
#  else
        // tweak sNextStorageMillis, so that our next storage is at the next full 5 minute
        uint16_t tSecondsUntilNextFull5Minutes = (STORAGE_INTERVAL_SECONDS
                - ((minute(tTimestamp) * SECS_PER_MIN) + second(tTimestamp)) % STORAGE_INTERVAL_SECONDS);
        sNextStorageMillis = millis() + (tSecondsUntilNextFull5Minutes * MILLIS_IN_ONE_SECOND);
#  endif
        delay(400); // wait for the scale factor toast to vanish
        printTimeToNextStorage();
#  if defined(LOCAL_TRACE)
        Serial.print(sTimestamp);
        Serial.print('|');
        Serial.print(prevMillis);
        Serial.print('|');
        Serial.println(sNextStorageMillis);
#  endif
    }
}

/*
 * Is called every 5 minutes by default
 */
time_t requestHostUnixTimestamp() {
    BlueDisplay1.getInfo(SUBFUNCTION_GET_INFO_LOCAL_TIME, &getTimeEventCallback);
    return 0; // the time will be sent later in response to getInfo
}
#endif // #if defined(USE_C_TIME)

#if defined(LOCAL_DEBUG)
#undef LOCAL_DEBUG
#endif
#endif // _CO2_LOGGER_AND_CHART_HPP
