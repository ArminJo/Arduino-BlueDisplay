/*
 *  MHZ19.h
 *
 *  Arduino library to control a MH-Z19C CO2 sensor. Tested with firmware version 5.2 and V5.12
 *  Based on - https://github.com/WifWaf/MH-Z19
 *           - https://revspace.nl/MH-Z19B
 *           - 4 Winsen Datasheets https://datasheet.lcsc.com/szlcsc/1901021600_Zhengzhou-Winsen-Elec-Tech-MH-Z19_C242514.pdf
 *
 *
 *  Copyright (C) 2022-2024  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of MHZ19 https://github.com/ArminJo/MH-Z19.
 *
 *  MHZ19 is free software: you can redistribute it and/or modify
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
 *
 */
#ifndef MHZ19_H
#define MHZ19_H

#include <Arduino.h>

//#define MHZ19_USE_MINIMAL_RAM                   // removes all field from the class, which are not required for just reading CO2 value

#define MHZ19_BAUDRATE                  9600    // Device to MH-Z19 Serial baudrate (should not be changed) -> around 1000 bytes per second
#define MHZ19_DATA_LEN                     9    // Data protocol length
#define MHZ19_RESPONSE_TIMEOUT_MILLIS    500    // 1/2 second timeout period for MH-Z19C response
#define MHZ19_DATASET_DURATION_MILLIS  MHZ19_DATA_LEN   // Data protocol duration

// Constants for SETABC_ON_OFF command
#define MHZ19_ABC_PERIOD_OFF    0x00
#define MHZ19_ABC_PERIOD_DEF    0xA0

#define TEMPERATURE_ADJUST_CONSTANT       40    // This is the constant value one get with command GETEMPERATURE_OFFSET.

class MHZ19 {
public:
    typedef enum {
        RECOVERY_RESET = 0x78,      // Recovery Reset - Changes operation mode and performs MCU reset
        SETABC_ON_OFF = 0x79, // Turns SETABC_ON_OFF (Automatic Baseline Correction) logic on or off (b[3] == 0xA0 - on, 0x00 - off) - mentioned in datasheet
        GETABC = 0x7D,              // Get SETABC_ON_OFF logic status  (1 - enabled, 0 - disabled)
        PERIOD = 0x7E,              // Set (b[3] = 2) and get measurement period - not from datasheet
        CO2RAW = 0x84,              // Raw CO2 ADC value and temperature compensated zero ADC value - not from datasheet
        CO2_AND_TEMPERATURE = 0x85, // Smoothed temperature ADC value, CO2 level - not from datasheet
        CO2MASKED_AND_TEMP = 0x86,  // CO2 masked at 500 in the first minute, temperature integer - not from datasheet
        SETZERO_CALIBRATION = 0x87, // Zero point calibration (like connecting HD to ground)
        SETSPAN_CALIBRATION = 0x88, // Span calibration Note: do ZERO calibration before span calibration
        SETRANGE = 0x99, // Sets sensor range. Note that parameter byte numbers are taken from the Chinese and not the English datasheet
        GETRANGE = 0x9B,            // Get Range
        GETCO2 = 0x9C,              // Get CO2 returns the same value as other CO2
        GETFIRMWARE_VERSION = 0xA0, // Get firmware version - not from datasheet
        GETLAST_RESPONSE = 0xA2,    // Get Last Response - not from datasheet
        GETEMPERATURE_OFFSET = 0xA3 // Get temperature offset - returns constant 40  - not from datasheet
    } MHZ19_command_t;

    /*
     * The serial connections
     */
    Stream *SerialToMHZ19;
    Print *SerialForDebug;

    // Possible values of member variable errorCode
    typedef enum {
        RESULT_NULL = 0, RESULT_OK = 1, RESULT_TIMEOUT = 2, RESULT_MATCH = 3, RESULT_CHECKSUM = 4
    } Error_Code_t;
    Error_Code_t errorCode; // Holds last received error code from recieveResponse()

    char VersionString[5]; // 0512,


#define ABC_COUNTER_MAX 143
    /*
     * Results of command 0x85 CO2_AND_TEMPERATURE
     */
    float TemperatureFloat; // Seems to be around 2.00 degree for 2 seconds and 4.4 degree for 1 second period higher than environment.
    uint16_t CO2Unmasked; // Is displayed even at first minute. Values are also clipped between 405 and 5000. V5.12 value was seen to go down to 175.
#if !defined(MHZ19_USE_MINIMAL_RAM)
    uint16_t MinimumLightADC;
#endif

    bool AutoBaselineCorrectionEnabled;

#if !defined(MHZ19_USE_MINIMAL_RAM)
    uint16_t Version;
    uint8_t VersionMajor;
    uint8_t VersionMinor;
    /*
     * Results of command 0x84 CO2RAW
     */
    uint16_t CO2RawADC;
    uint16_t CO2RawTemperatureCompensatedBaseADC; // v5.2 The ADC Value of 410 ppm ??? compensated by temperature. V5.12 constant 32000.
    int16_t CO2RawADCDelta; // v5.2 CO2RawTemperatureCompensatedBaseADC - CO2RawADC, V5.12 CO2RawADC - CO2RawTemperatureCompensatedBaseADC
    uint16_t Unknown2;

    /*
     * Results of command 0x86 CO2MASKED_AND_TEMP
     */
    uint16_t CO2; // == CO2Unmasked, except that it is displayed / masked as 500 for the first minute. Values are clipped between 405 and 5000.
    int8_t Temperature;
    uint8_t ABCCounter; // Is incremented by MHZ19 every 10 minutes. Range is from 0 to 143


    /*
     * Results of other commands
     */
    uint16_t SensorRange; // 5.12 is 5000
    uint16_t CO2Alternate; // 5.12 Values are clipped at 400.
    uint16_t Period; // 5.12 is 0

    bool SerialDebugOutputIsEnabled = false; // if true, SerialForDebug must be set (by enableDebug())
#endif

    /*
     * The send and receive buffers
     */
#define COMMAND_RECEIVE_INDEX   1 // Index of command in ReceivedResponse
#define COMMAND_SEND_INDEX      2 // Index of command in CommandToSend
#define CHECKSUM_INDEX          8 // Index of checksum in CommandToSend and ReceivedResponse
    uint8_t CommandToSend[MHZ19_DATA_LEN];      // Array for commands to be sent
    uint8_t ReceivedResponse[MHZ19_DATA_LEN];   // Array for response

    void setSerial(Stream *aStream);
    bool begin(Stream *aStream);
    bool begin(Stream &aStream); // use & to be compatible to other libraries


    uint8_t computeChecksum(uint8_t *aArray);
    bool processCommand(MHZ19_command_t aCommand, bool aDoNotWaitForResponse = false);
    bool readResponse();
    bool readCO2UnmaskedAndTemperatureFloat();
    bool readVersion();
    bool readABC(); // Reads SETABC_ON_OFF-Status using command 125 / 0x7D
#if !defined(MHZ19_USE_MINIMAL_RAM)
    void enableDebug(Print *aSerialForDebugOutput);
    void disableDebug();
    bool readCO2MaskedTemperatureIntAndABCCounter();
    bool readRange();
    bool readCO2Alternate();
    bool readPeriod();
    bool readCO2Raw();
    void setSpanCalibration(uint16_t aValueOfCurrentCO2);
    void setRange(uint16_t aRange);
    void setPeriod();
#endif

    void setZeroCalibration();
    void setAutoCalibration(bool aSwitchOn);

    void printErrorCode(Print *aSerial);
    void printErrorMessage(Print *aSerial);

    void printCommand(MHZ19_command_t aCommand, Print *aSerial);

};
#endif
