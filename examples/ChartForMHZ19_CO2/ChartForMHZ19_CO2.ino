/*
 *  ChartForMHZ19_CO2.cpp
 *
 *  A full display GUI displaying 4 days of CO2 values with BlueDisplay chart.
 *  For full screen applications, the app's menu is called by swiping horizontal from the left edge of the screen.
 *
 *  Connect a MHZ19 Co2 Sensor at pin 9 + 10 for RX and TX (see code below).
 *  Connect a 2004 LCD at the I2C serial, or parallel (pins see code below.
 *  Connect a HC-05 to the Arduino RX and TX.
 *  Insert a Schottky diode in the Arduino RX to HC-05 TX connection.
 *  see: https://github.com/ArminJo/Arduino-BlueDisplay?tab=readme-ov-file#connecting-tx
 *
 *  On Calibration, the current value is immediately taken as the 410 ppm environment value
 *  Powering via USB, which uses the Arduino Nano serial diode, leads to incorrect readings.
 *  This is due to the high current drawn by the sensor.
 *  After Reset an initial value of 500 ppm is displayed for 1 minute.
 *
 *
 *  Copyright (C) 2022-2025  Armin Joachimsmeyer
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
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 */

/*
 * MH-Z19C Supply current: 60 mA 500 ms and 160 mA 500 ms.
 *
 * Reengineering of a Technoline WL1030 and a SA1200P gives:
 * Supply is 5.17 volt
 * 1 measurement/s taken by the module
 * and every 15 seconds a request with 0x11, 0x01, 0x01, 0xED
 * WL1030 answers with 0x16, 0x05, 0x01, <MSB_CO2>, <LSB_CO2>, 0x01, <MSB_Counter>, <LSB_Counter> with an increment of around 3700 to 3900
 * SA1200P answers with 0x16, 0x05, 0x01, <MSB_CO2>, <LSB_CO2>, 0x00, <MSB_Counter>, <LSB_Counter> with an increment of around 1300
 */

#include <Arduino.h>

//#define SUPPORT_BLUEDISPLAY_CHART // Show chart of historical data on a tablet running the BlueDisplay app

/*
 * !!!!!!!!!!!!!!!!!
 * Compile time symbol _SS_MAX_RX_BUFF must be set to 32 (with -D_SS_MAX_RX_BUFF=32)
 * or change line 43 in SoftwareSerial.h "#define _SS_MAX_RX_BUFF 64" to "#define _SS_MAX_RX_BUFF 32"
 * Otherwise stacks overwrite CO2Array at GUI initialization time
 * !!!!!!!!!!!!!!!!!
 */
#if defined(SUPPORT_BLUEDISPLAY_CHART)
// To save RAM, we also require the stripped down version of the MHZ19 library.
#define MHZ19_USE_MINIMAL_RAM   // removes all field from the class, which are not required for just reading CO2 value
#endif
// For available commands, see https://revspace.nl/MH-Z19B
#include "MHZ19.hpp"

#define VERSION_EXAMPLE                     "2.0"
#define DISPLAY_PERIOD_MILLIS               2000    // The sensor makes a measurement every 2 seconds
#define LOOP_PERIOD_MILLIS                  200 // delay in loop

#if !defined(DO_NOT_DISPLAY_MHZ19_TEMPERATURE)
#define DISPLAY_MHZ19_TEMPERATURE               // Shows the temperature provided by MHZ19 on display
#endif

#if defined(DISPLAY_MHZ19_TEMPERATURE)
#define TEMPERATURE_CORRECTION_MINUS_PIN    A1
#define TEMPERATURE_CORRECTION_PLUS_PIN     A2
#define TEMPERATURE_CORRECTION_VALUE        0.01
float sTemperatureCorrectionFloat;              // 2.00 degree for 2 seconds and 4.4 degree for 1 second period sensors.
EEMEM float sTemperatureCorrectionFloatEeprom;  // Storage in EEPROM for sTemperatureCorrectionFloat
#define EEPROM_REQUIRED_FOR_APPLICATION_BYTES 4
void checkTemperatureCorrectionPins();
#endif

//#define TEST // like production, but every 30 seconds
//#define STANDALONE_TEST // runs fast without Serial input
#if defined(STANDALONE_TEST)
#define ONE_MEASUREMENT_PERIOD_SECOND       10L
#elif defined(TEST)
#define ONE_MEASUREMENT_PERIOD_SECOND       30L
#else
#define ONE_MEASUREMENT_PERIOD_SECOND       (5L * SECS_PER_MIN) // store dataset every 5 minutes
#endif
#define STORAGE_PERIOD_MILLIS    (ONE_MEASUREMENT_PERIOD_SECOND * MILLIS_IN_ONE_SECOND)
uint32_t sMillisOfLastRequestedCO2Data;

#if defined(SUPPORT_BLUEDISPLAY_CHART)
//#define ENABLE_STACK_ANALYSIS
#  if defined(ENABLE_STACK_ANALYSIS)
#include "AVRUtils.h" // include sources for initStackFreeMeasurement() and printRAMInfo()
#  endif
//#define BLUETOOTH_BAUD_RATE BAUD_115200   // Activate this, if you have reprogrammed the HC-05 module for 115200
#define BD_USE_SIMPLE_SERIAL            // To save the RAM used for Serial object and buffers
#if !defined(BLUETOOTH_BAUD_RATE)
#define BLUETOOTH_BAUD_RATE     9600    // Default baud rate of my HC-05 modules, which is not very reactive
#endif
#include "CO2LoggerAndChart.hpp"
#endif // defined(STANDALONE_TEST)

#define MHZ19_RX_PIN                 9 // Rx pin which the MHZ19 Tx pin is attached to
#define MHZ19_TX_PIN                10 // Tx pin which the MHZ19 Rx pin is attached to

#define DEBUG_PIN                   11 // Connecting these pin to ground enables debug mode
#define ENABLE_3_LINE_DIGITS_PIN    12 // Connecting these pin to ground enables 3 line digits instead of the 4 line ones. Has precedence over DEBUG_PIN.
bool sShow3LineDigits = false;
bool sDebugModeActive = false; // sDebugModeActive has precedence over sShow3LineDigits. Precedence is implemented by checkDebugPin().
void checkDebugPin(bool aInSetup);

MHZ19 myMHZ19;                         // Constructor for library
#if defined(ESP32)
HardwareSerial MHZ19Serial(2);            // On ESP32 we do not require the SoftwareSerial library, since we have 2 USARTS available
#else
#include <SoftwareSerial.h>            //  Remove if using HardwareSerial or not available for board
SoftwareSerial MHZ19Serial(MHZ19_RX_PIN, MHZ19_TX_PIN);    // Software Serial to MH-Z19 serial
#endif

//#define USE_PARALLEL_2004_LCD // Is default
//#define USE_PARALLEL_1602_LCD
//#define USE_SERIAL_2004_LCD
//#define USE_SERIAL_1602_LCD
#include "LCDPrintUtils.hpp" // sets USE_PARALLEL_LCD or USE_SERIAL_LCD

#if defined(USE_PARALLEL_LCD)
#include <LiquidCrystal.h>
LiquidCrystal myLCD(7, 8, 3, 4, 5, 6);
//LiquidCrystal myLCD(2, 3, 4, 5, 6, 7);
//LiquidCrystal myLCD(4, 5, 6, 7, 8, 9);
#else
#define LCD_I2C_ADDRESS 0x27    // Default LCD address is 0x27 for a 20 chars and 4 line / 2004 display
#define USE_SOFT_I2C_MASTER // Requires SoftI2CMaster.h + SoftI2CMasterConfig.h. Saves 2110 bytes program memory and 200 bytes RAM compared with Arduino Wire
#include "LiquidCrystal_I2C.hpp"  // This defines USE_SOFT_I2C_MASTER, if SoftI2CMasterConfig.h is available. Use only the modified version delivered with this program!
LiquidCrystal_I2C myLCD(LCD_I2C_ADDRESS, 20, 4);
#endif
#define LCD_STRING_BUFFER_SIZE 6
char sLCDStringBuffer[LCD_STRING_BUFFER_SIZE];

#include "LCDBigNumbers.hpp" // Include sources for LCD big number generation
LCDBigNumbers BigNumbersLCD(&myLCD, BIG_NUMBERS_FONT_3_COLUMN_4_ROWS_VARIANT_1); // Use 3x4 numbers, 1. variant

void printErrorCode();
void printData();
void printCO2DataOnLCD();
void checkDebugAndSmallDigitsPin(bool aInSetup);
void checkSmallDigitsPin(bool aInSetup);

//#define PRINT_PERIODIC_DATA_ALWAYS_ON_SERIAL
/*
 * PRINT_PERIODIC_DATA_ALWAYS_ON_SERIAL generates output:
 * CO2 alt. =657 Temperature  (C)=28    ABCCounter=0
 * CO2 (ppm)=657 TemperatureFloat=25.23 MinimumLightADC=557
 * CO2Raw|38973 - CO2Base|32000 = 6973  -CO2 = 6316
 * ExcelData=2508;557;28;25.23;657;32000;38973;6973;6316
 */

/*
 * Helper macro for getting a macro definition as string
 */
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

void setup() {
// initialize the digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    pinMode(DEBUG_PIN, INPUT_PULLUP);
    pinMode(ENABLE_3_LINE_DIGITS_PIN, INPUT_PULLUP);
#if defined(DISPLAY_MHZ19_TEMPERATURE)
    pinMode(TEMPERATURE_CORRECTION_MINUS_PIN, INPUT_PULLUP);
    pinMode(TEMPERATURE_CORRECTION_PLUS_PIN, INPUT_PULLUP);
#endif

#if defined(SUPPORT_BLUEDISPLAY_CHART)
    initSerial(BLUETOOTH_BAUD_RATE); // converted to Serial.begin(BLUETOOTH_BAUD_RATE);
#  if defined(ENABLE_STACK_ANALYSIS)
    initStackFreeMeasurement();
#  endif
#else
#define BlueDisplay1    Serial
#define isConnectionEstablished()  available()
    Serial.begin(115200);
#endif

#if defined(BD_USE_SERIAL1) || defined(ESP32) // USE_SERIAL1 may be defined in BlueSerial.h
// Serial(0) is available for Serial.print output.
#  if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/ \
    || defined(SERIALUSB_PID)  || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#  endif
// Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ "\r\nVersion " VERSION_EXAMPLE " from " __DATE__));
#elif !defined(BD_USE_SIMPLE_SERIAL)
    // If using simple serial on first USART we cannot use Serial.print, since this uses the same interrupt vector as simple serial.
#  if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/ \
    || defined(SERIALUSB_PID)  || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_attiny3217)
        delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#  endif
    // If connection is enabled, this message was already sent as BlueDisplay1.debug()
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ "\r\nVersion " VERSION_EXAMPLE " from " __DATE__));
#endif

    MHZ19Serial.begin(MHZ19_BAUDRATE); // 9600

#if defined(USE_PARALLEL_2004_LCD)
    myLCD.begin(20, 4); // This also clears display
#else
    myLCD.init();
    myLCD.backlight();  // Switch backlight LED on
#endif
    checkDebugAndSmallDigitsPin(true);  // This initializes BigNumbers, clears LCD and sets cursor to 0.0
    myLCD.print(F("CO2 Sensor")); // Show this before waiting for BlueDisplay connection

#if defined(DISPLAY_MHZ19_TEMPERATURE)
    myLCD.print(F(" +Temp"));
#else
    myLCD.print(F(" -Temp"));
#endif

    /*
     * Try to connect to BlueDisplay host, this may introduce a delay of 1.5 seconds :-(
     */
#if defined(SUPPORT_BLUEDISPLAY_CHART)
    myLCD.setCursor(0, 1);
    myLCD.print(F("BD - try to connect")); // Show this before waiting for BlueDisplay connection
    InitCo2LoggerAndChart(); // introduces a delay of up to 1.5 second :-(
    myLCD.setCursor(5, 1);
    if (!BlueDisplay1.isConnectionEstablished()) {
        myLCD.print(F("not "));
    }
    myLCD.print(F("connected     "));
#  if defined(ENABLE_STACK_ANALYSIS)
    printRAMInfo(&Serial); // 1.12.24 - 90 bytes unused here
#  endif
#else
    myLCD.print(F(" -BD"));
#endif

#if !defined(BD_USE_SIMPLE_SERIAL)
    if (!BlueDisplay1.isConnectionEstablished()) {
        Serial.println(F("Small digit pin is pin " STR(ENABLE_3_LINE_DIGITS_PIN)));
        Serial.println(F("Debug pin is pin " STR(DEBUG_PIN)));
        Serial.println(F("DISPLAY_PERIOD = " STR(DISPLAY_PERIOD_MILLIS) " ms"));
    }

    if (!BlueDisplay1.isConnectionEstablished()) {
#  if defined(DISPLAY_MHZ19_TEMPERATURE)
        Serial.println(F("Display of MHZ19 temperature value"));
#  else
       Serial.println(F("No display of MHZ19 temperature value"));
#  endif
    }
#endif

    myLCD.setCursor(0, 1);
    myLCD.print(F(VERSION_EXAMPLE " " __DATE__));
//    myLCD.print(F("\xDF""C temp. corr."));
    myLCD.setCursor(0, 2);
    myLCD.print(F("Small digit pin = " STR(ENABLE_3_LINE_DIGITS_PIN)));
    myLCD.setCursor(0, 3);
    myLCD.print(F("Debug pin = " STR(DEBUG_PIN)));

    delay(2000);

    if (myMHZ19.begin(&MHZ19Serial)) { // (*Stream) reference must be passed to library begin(). The function tries to read version.
        printErrorCode();
        // Blink forever
        while (true) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(200);
            digitalWrite(LED_BUILTIN, LOW);
            delay(200);
        }
    }

#if defined(DISPLAY_MHZ19_TEMPERATURE)
#  if !defined(BD_USE_SIMPLE_SERIAL)
    if (!BlueDisplay1.isConnectionEstablished()) {
        Serial.println(F("Temperature correction + pin=" STR(TEMPERATURE_CORRECTION_PLUS_PIN)));
        Serial.println(F("Temperature correction - pin=" STR(TEMPERATURE_CORRECTION_MINUS_PIN)));
    }
#  endif

    myLCD.setCursor(0, 2);
    myLCD.print(F("Temp. corr. + pin=" STR(TEMPERATURE_CORRECTION_PLUS_PIN)));
    myLCD.setCursor(0, 3);
    myLCD.print(F("Temp. corr. - pin=" STR(TEMPERATURE_CORRECTION_MINUS_PIN)));
    delay(3000);

    /*
     * Temperature correction depends on the period
     */
    sTemperatureCorrectionFloat = eeprom_read_float(&sTemperatureCorrectionFloatEeprom);
    if (isnan(sTemperatureCorrectionFloat) || sTemperatureCorrectionFloat < -10 || sTemperatureCorrectionFloat > 0.0) {
        sTemperatureCorrectionFloat = -2.00; // initial value
        eeprom_write_float(&sTemperatureCorrectionFloatEeprom, sTemperatureCorrectionFloat);
    }
#  if !defined(BD_USE_SIMPLE_SERIAL)
    if (!BlueDisplay1.isConnectionEstablished()) {
        Serial.print(F("TEMPERATURE_FLOAT_CORRECTION="));
        Serial.print(sTemperatureCorrectionFloat, 2);
        Serial.println(F(" degree Celsius"));
    }
#  endif

    myLCD.setCursor(0, 3);
    // 13 characters
    myLCD.print(sTemperatureCorrectionFloat);
    myLCD.print(F("\xDF" "C correction  "));
#endif

    /*
     getVersion(char array[]) returns version number to the argument. The first 2 char are the major
     version, and second 2 bytes the minor version. e.g 02.11
     */
#if !defined(BD_USE_SIMPLE_SERIAL)
    if (!BlueDisplay1.isConnectionEstablished()) {
        Serial.print(F("MH-Z19 firmware version="));
        Serial.println(myMHZ19.VersionString); // Version string was read by begin(); Tested with 5.12
    }
#endif
    myLCD.setCursor(0, 1);
    myLCD.print(F("FW version "));
    myLCD.print(myMHZ19.VersionString);

    if (myMHZ19.readABC()) {
#if !defined(BD_USE_SIMPLE_SERIAL)
        myMHZ19.printErrorCode(&Serial);
#endif
    } else {
#if !defined(BD_USE_SIMPLE_SERIAL)
        Serial.print(F("Auto calibration status="));
#endif
        // Clear line
        myLCD.setCursor(0, 2);
        myLCD.print(F("                    "));
        myLCD.setCursor(0, 2);

        myLCD.print(F("ABC="));
        if (myMHZ19.AutoBaselineCorrectionEnabled) {
#if !defined(BD_USE_SIMPLE_SERIAL)
            Serial.println(F("ON"));
#endif
            myLCD.print(F("ON              "));
        } else {
#if !defined(BD_USE_SIMPLE_SERIAL)
            Serial.println(F("OFF"));
            Serial.println(F("Enable auto calibration now"));
#endif
            myLCD.print(F("OFF"));
            delay(1000);
            myLCD.setCursor(0, 3);
            myLCD.print(F("Enable ABC now      "));
            delay(2000);
            myMHZ19.setAutoCalibration(true);      // Turn auto calibration ON (OFF autoCalibration(false))
        }
    }

    delay(2000);

#if !defined(MHZ19_USE_MINIMAL_RAM)
    myMHZ19.readRange();

    if (!BlueDisplay1.isConnectionEstablished()) {
        Serial.print(F("Range="));
        Serial.print(myMHZ19.SensorRange);
        Serial.print(F(" Period="));
        Serial.println(myMHZ19.Period);
    }

    myLCD.print(F("R="));
    myLCD.print(myMHZ19.SensorRange); // is 5000 on my device with FW 5.12
    myLCD.print(F(" P="));
    myLCD.print(myMHZ19.Period); // is 0 on my device with FW 5.12

    delay(4000);
#endif
#if defined(PRINT_PERIODIC_DATA_ALWAYS_ON_SERIAL)
    Serial.print(F("Excel data caption="));
    Serial.print(F("Unknown2;MinimumLightADC;Temperature;TemperatureFloat;CO2Unmasked;"));
    Serial.print(F("CO2RawTemperatureCompensatedBaseADC;CO2RawADC;CO2RawTemperatureCompensatedBaseADC-CO2RawADC;"));
    Serial.println(F("CO2RawTemperatureCompensatedBaseADC-CO2RawADC-CO2Unmasked"));
#endif

    myLCD.clear();
    sMillisOfLastRequestedCO2Data = - DISPLAY_PERIOD_MILLIS; // force first measurement
}

void loop() {

#if defined(DISPLAY_MHZ19_TEMPERATURE)
    checkTemperatureCorrectionPins(); // must before checkDebugAndSmallDigitsPin()
#endif
    checkDebugAndSmallDigitsPin(false);

    if (millis() - sMillisOfLastRequestedCO2Data >= DISPLAY_PERIOD_MILLIS) {
        sMillisOfLastRequestedCO2Data = millis(); // set for next check
        // The sensor makes a measurement every 2 seconds

        if (myMHZ19.readCO2UnmaskedAndTemperatureFloat()) {
            printErrorCode();
        } else {
#if !defined(MHZ19_USE_MINIMAL_RAM)
            myMHZ19.readCO2MaskedTemperatureIntAndABCCounter();
            myMHZ19.readCO2Alternate();
            myMHZ19.readCO2Raw();
#endif
#if defined(PRINT_PERIODIC_DATA_ALWAYS_ON_SERIAL)
        printData();
#else

#  if !defined(MHZ19_USE_MINIMAL_RAM)
            if (sDebugModeActive) {
                printData();
            }
#  endif
#endif
            printCO2DataOnLCD();
        }
    }
#if defined(SUPPORT_BLUEDISPLAY_CHART)

    if (myMHZ19.CO2Unmasked < 400) {
        myMHZ19.CO2Unmasked = 400; // It maybe 0 after power up
    }

    if (storeCO2ValuePeriodically(myMHZ19.CO2Unmasked, STORAGE_PERIOD_MILLIS)) {
#if defined(ENABLE_STACK_ANALYSIS)
        printRAMInfo(&Serial); // 33 unused 1.12.24
#endif
#if defined(STANDALONE_TEST)
        /*
         * Increment time by 5 minutes for demo
         */
        adjustTime(5 * SECS_PER_MIN);
#endif
    }

    delayMillisWithCheckForEventAndFlags(LOOP_PERIOD_MILLIS);
#else
    delay(LOOP_PERIOD_MILLIS); // serves as button debouncing
#endif
}

void printErrorCode() {
#if !defined(BD_USE_SIMPLE_SERIAL)
    myMHZ19.printErrorMessage(&Serial);
#endif
    LCDClearLine(&myLCD, 3);
    myLCD.print(F("MHZ error: "));
    myMHZ19.printErrorCode(&myLCD);
    delay(1000);
}

void checkDebugAndSmallDigitsPin(bool aInSetup) {
    checkDebugPin(aInSetup);
    /*
     * sDebugModeActive has precedence over sShow3LineDigits
     */
#if defined(MHZ19_USE_MINIMAL_RAM)
    // No debug mode for !sShow3LineDigits so precendence is not required
    checkSmallDigitsPin(aInSetup);
#else
    if (!sDebugModeActive) {
        checkSmallDigitsPin(aInSetup);
    }
#endif
}

/*
 * Set sDebugModeActive and en/dis-able myMHZ19 debug and clear screen on changing mode
 */
void checkDebugPin(bool aInSetup) {
    bool tDebugModeActive = !digitalRead(DEBUG_PIN);
    if (sDebugModeActive != tDebugModeActive) {
        sDebugModeActive = tDebugModeActive;
// Debug mode changed

#if defined(MHZ19_USE_MINIMAL_RAM)
        if (tDebugModeActive && !sShow3LineDigits) { // 3 LineDigits has precedence over debug
            myLCD.setCursor(0, 3);
            myLCD.print(F("No debug data!  ")); // ... because of RAM shortage
#if !defined(BD_USE_SIMPLE_SERIAL)
            Serial.print(F("No debug data available for minimal RAM setting"));
#endif
            delay(2000);
        }
#else
        if (tDebugModeActive) {
            myMHZ19.enableDebug(&Serial);
            Serial.print(F("En"));
        } else {
            myMHZ19.disableDebug();
            Serial.print(F("Dis"));
        }
        Serial.println(F("abled debug mode"));
#endif
        myLCD.clear(); // clear content of former page
        if (!aInSetup) {
            printCO2DataOnLCD();
        }
    }
}

void checkSmallDigitsPin(bool aInSetup) {
    bool tShow3LineDigits = !digitalRead(ENABLE_3_LINE_DIGITS_PIN);
    if (sShow3LineDigits != tShow3LineDigits || aInSetup) {
        sShow3LineDigits = tShow3LineDigits;
#if !defined(BD_USE_SIMPLE_SERIAL)
        Serial.print(F("Changed digit size to "));
#endif

        /*
         * Show3LineDigits mode changed, change digit generation
         */
        if (tShow3LineDigits) {
            sDebugModeActive = false;
            BigNumbersLCD.init(BIG_NUMBERS_FONT_3_COLUMN_3_ROWS_VARIANT_1);
//            Serial.println('3');
        } else {
            BigNumbersLCD.init(BIG_NUMBERS_FONT_3_COLUMN_4_ROWS_VARIANT_1);
//            Serial.println('4');
        }
        BigNumbersLCD.begin(); // Generate font symbols in LCD controller
        myLCD.clear(); // clear content of former page
        if (!aInSetup) {
            printCO2DataOnLCD();
        }
    }
}

/*
 * Output:
 * CO2 alt. =657 Temperature  (C)=28    ABCCounter=0
 * CO2 (ppm)=657 TemperatureFloat=25.23 MinimumLightADC=557
 * CO2Raw|38973 - CO2Base|32000 = 6973  -CO2 = 6316
 * ExcelData=2508;557;28;25.23;657;32000;38973;6973;6316
 */
void printData() {
#if !defined(MHZ19_USE_MINIMAL_RAM)
    Serial.print(F("CO2 alt. ="));
    Serial.print(myMHZ19.CO2Alternate);
    Serial.print(F(" Temperature  (C)="));
    Serial.print(myMHZ19.Temperature);
    Serial.print(F("    ABCCounter="));
    Serial.print(myMHZ19.ABCCounter);
    Serial.println();

    Serial.print(F("CO2 (ppm)="));
    Serial.print(myMHZ19.CO2Unmasked);
    Serial.print(F(" TemperatureFloat="));
    Serial.print(myMHZ19.TemperatureFloat);
    Serial.print(F(" MinimumLightADC="));
    Serial.print(myMHZ19.MinimumLightADC);
    Serial.println();

    if (myMHZ19.Version >= 520) {
        Serial.print(F("CO2Base|"));
        Serial.print(myMHZ19.CO2RawTemperatureCompensatedBaseADC);
        Serial.print(F(" - CO2Raw|"));
        Serial.print(myMHZ19.CO2RawADC);
    } else {
        Serial.print(F("CO2Raw|"));
        Serial.print(myMHZ19.CO2RawADC);
        Serial.print(F(" - CO2Base|"));
        Serial.print(myMHZ19.CO2RawTemperatureCompensatedBaseADC);
    }
    Serial.print(F(" = "));
    Serial.print(myMHZ19.CO2RawADCDelta);
    Serial.print(F("  -CO2 = "));
    Serial.print((int) (myMHZ19.CO2RawADCDelta - myMHZ19.CO2Unmasked));
    Serial.println();

    Serial.print(F("ExcelData="));
    Serial.print(myMHZ19.Unknown2);
    Serial.print(';');
    Serial.print(myMHZ19.MinimumLightADC);
    Serial.print(';');

    Serial.print(myMHZ19.Temperature);
    Serial.print(';');
    Serial.print(myMHZ19.TemperatureFloat);
    Serial.print(';');

    Serial.print(myMHZ19.CO2Unmasked);
    Serial.print(';');
    Serial.print(myMHZ19.CO2RawTemperatureCompensatedBaseADC);
    Serial.print(';');
    Serial.print(myMHZ19.CO2RawADC);
    Serial.print(';');
    Serial.print(myMHZ19.CO2RawADCDelta);
    Serial.print(';');
    Serial.print((int) (myMHZ19.CO2RawADCDelta - myMHZ19.CO2Unmasked));
    Serial.println();
#endif
}

/*
 * Show 4 line PPM digits and short temperature (22.2) on lower right
 * If sShow3LineDigits, show 3 line PPM digits and long temperature (22.14°C) on lower right
 * If sDebugModeActive and sShow3LineDigits and MHZ19_USE_MINIMAL_RAM show temperature correction value on lower left
 *
 * If sDebugModeActive and NOT MHZ19_USE_MINIMAL_RAM show debug page:
 *
 * xxxx PPM   510|510  | CO2Unmasked, CO2|CO2Alternate
 * 38770 - 32000 = 6770| CO2RawTemperatureCompensatedBaseADC - CO2RawADC = CO2RawADCDelta or CO2RawADC - CO2RawTemperatureCompensatedBaseADC = CO2RawADCDelta
 * 557 0x9BB        144| MinimumLightADC, Unknown2, (144 - ABCCounter)
 * -2.00° 22.31°C   26°| sTemperatureCorrectionFloat, TemperatureFloat + sTemperatureCorrectionFloat, Temperature
 */
void printCO2DataOnLCD() {
    /*
     * CO2 as big numbers
     */
#if defined(__AVR__)
    snprintf_P(sLCDStringBuffer, LCD_STRING_BUFFER_SIZE, PSTR("%4u"), myMHZ19.CO2Unmasked);
#else
    snprintf(sLCDStringBuffer, LCD_STRING_BUFFER_SIZE, "%4u", myMHZ19.CO2Unmasked);
#endif
#if !defined(MHZ19_USE_MINIMAL_RAM)
    if (sDebugModeActive) {
        myLCD.setCursor(0, 0);
        myLCD.print(sLCDStringBuffer);
        myLCD.print(' ');
    } else
#endif
    {
        /*
         * Print the 3 or 4 line big digits here
         */
        BigNumbersLCD.setBigNumberCursor(0, 0);
        BigNumbersLCD.print(sLCDStringBuffer);
        myLCD.setCursor(16, 1);
    }
    myLCD.print(F("PPM "));

#if !defined(MHZ19_USE_MINIMAL_RAM)
    if (sDebugModeActive) {
        /*
         * Print also CO2 masked and CO2Alternate (Background CO2?) in line 1
         */
        myLCD.print(F("  "));
        myLCD.print(myMHZ19.CO2);
        myLCD.print('|');
        myLCD.print(myMHZ19.CO2Alternate);

        /*
         * Print raw values in line 2
         */
        myLCD.setCursor(0, 1);
        if (myMHZ19.Version >= 520) {
            myLCD.print(myMHZ19.CO2RawTemperatureCompensatedBaseADC);
            myLCD.print(F(" - "));
            myLCD.print(myMHZ19.CO2RawADC);
        } else {
            myLCD.print(myMHZ19.CO2RawADC);
            myLCD.print(F(" - "));
            myLCD.print(myMHZ19.CO2RawTemperatureCompensatedBaseADC);
        }
        myLCD.print(F(" ="));
        if (myMHZ19.CO2RawADCDelta < 10000 && myMHZ19.CO2RawADCDelta > -100) {
            myLCD.print(' ');
        }

        // Raw delta
#if defined(__AVR__)
        snprintf_P(sLCDStringBuffer, LCD_STRING_BUFFER_SIZE, PSTR("%4d"), myMHZ19.CO2RawADCDelta);
#else
        snprintf(sLCDStringBuffer, LCD_STRING_BUFFER_SIZE, "%4d", myMHZ19.CO2RawADCDelta);
#endif
        myLCD.print(sLCDStringBuffer);
        /*
         * Print some data in line 3
         */
        myLCD.setCursor(0, 2);
        myLCD.print(myMHZ19.MinimumLightADC);
        myLCD.print(F(" 0x"));
        myLCD.print(myMHZ19.Unknown2, HEX);
    } // if (sDebugModeActive)
#endif

    if (!sShow3LineDigits
#if !defined(MHZ19_USE_MINIMAL_RAM)
            && !sDebugModeActive
#endif
            ) {
#if defined(DISPLAY_MHZ19_TEMPERATURE)
        /*
         * Big numbers
         * Only 4 character float temperature in line 4
         */
        myLCD.setCursor(16, 3);
        myLCD.print(myMHZ19.TemperatureFloat + sTemperatureCorrectionFloat, 1);
#endif
    } else {
        /*
         * 3 line digits or debug here
         */
#if defined(DISPLAY_MHZ19_TEMPERATURE)
        /*
         * Float temperature in line 4
         */
        myLCD.setCursor(7, 3);
        myLCD.print(myMHZ19.TemperatureFloat + sTemperatureCorrectionFloat, 2);
        myLCD.print(F("\xDF" "C "));

        if (sDebugModeActive) {
            myLCD.setCursor(0, 3);
            /*
             * Temperature correction (and integer temperature) in line 4
             */
            myLCD.print(sTemperatureCorrectionFloat, 2);
            myLCD.print(F("\xDF "));
#  if !defined(MHZ19_USE_MINIMAL_RAM)
            myLCD.setCursor(17, 3);
            myLCD.print(myMHZ19.Temperature);
            myLCD.print(F("\xDF"));
#  endif

        }
#endif // if defined(DISPLAY_MHZ19_TEMPERATURE)
#if !defined(MHZ19_USE_MINIMAL_RAM)
        /*
         * ABC counter in line 4 (3 for debug)
         */
        if (sDebugModeActive) {
            myLCD.setCursor(17, 2);
        } else {
            myLCD.setCursor(17, 3);
        }
#if defined(__AVR__)
        snprintf_P(sLCDStringBuffer, LCD_STRING_BUFFER_SIZE, PSTR("%3u"), (ABC_COUNTER_MAX + 1) - myMHZ19.ABCCounter);
#else
        snprintf(sLCDStringBuffer, LCD_STRING_BUFFER_SIZE, "%3u", (ABC_COUNTER_MAX + 1) - myMHZ19.ABCCounter);
#endif
        myLCD.print(sLCDStringBuffer);
#endif
    }
}

#if defined(DISPLAY_MHZ19_TEMPERATURE)
/*
 * Changes the temperature correction by 0.01 degree units.
 * Switches display mode to debug temporarily in order to show result of button press.
 *
 * Is called every DISPLAY_PERIOD_MILLIS / 2 seconds (-> every second)
 * For continuous press, autorepeat 5/s is entered after 2 seconds.
 * The function exits 2 seconds after last press.
 */
void checkTemperatureCorrectionPins() {
    uint8_t t200MillisecondsActiveCounter = 0;
    uint8_t t200MillisecondsInactiveCounter = 0; // for automatic return to CO2 page after 2 seconds of not pressed

    /*
     * Do this while loop until we have inactivity for 2 seconds, or first check is no button pressed
     */
    while (t200MillisecondsInactiveCounter < (2000 / LOOP_PERIOD_MILLIS)) {
        bool tMinusActivated = !digitalRead(TEMPERATURE_CORRECTION_MINUS_PIN);
        bool tPlusActivated = !digitalRead(TEMPERATURE_CORRECTION_PLUS_PIN);
        if (tMinusActivated || tPlusActivated) {
            /*
             * At least one button is pressed here
             */
            if (t200MillisecondsActiveCounter == 0 || t200MillisecondsActiveCounter > (1000 / LOOP_PERIOD_MILLIS)) {
                /*
                 * Last state was: both button inactive or long press for more than 1 second
                 * -> change value
                 */
                if (tMinusActivated) {
                    sTemperatureCorrectionFloat -= TEMPERATURE_CORRECTION_VALUE;
#if !defined(BD_USE_SIMPLE_SERIAL)
                    Serial.print(F("De"));
#endif
                }
                if (tPlusActivated) {
                    sTemperatureCorrectionFloat += TEMPERATURE_CORRECTION_VALUE;
#if !defined(BD_USE_SIMPLE_SERIAL)
                    Serial.print(F("In"));
#endif
                }
#if !defined(BD_USE_SIMPLE_SERIAL)
                Serial.print(F("crement temperature correction to "));
                Serial.println(sTemperatureCorrectionFloat, 2);
#endif
            }

            t200MillisecondsActiveCounter++;
            t200MillisecondsInactiveCounter = 0;
            if (!sDebugModeActive) {
                sDebugModeActive = true;
                myLCD.clear();
            }
            printCO2DataOnLCD();

        } else {
            /*
             * No button pressed here
             */
            if (t200MillisecondsActiveCounter == 0 && t200MillisecondsInactiveCounter == 0) {
                // Check if we are in the first loop. Then no button was pressed before, and we can do an immediate exit.
                return;
            }
            t200MillisecondsActiveCounter = 0;
            t200MillisecondsInactiveCounter++;
        }
        delay(LOOP_PERIOD_MILLIS); // 5 per second
    }

// Write final value to EEPROM
    eeprom_write_float(&sTemperatureCorrectionFloatEeprom, sTemperatureCorrectionFloat);
    myLCD.clear();
    // printCO2DataOnLCD(); not required since next statement in main loop is  is checkDebugAndSmallDigitsPin()
}
#endif
