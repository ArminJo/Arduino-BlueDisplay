/*
 *  MHZ19.cpp
 *
 *  Arduino library to control a MH-Z19C CO2 sensor. Tested with firmware version 5.2
 *  Based on - https://github.com/WifWaf/MH-Z19
 *           - https://revspace.nl/MH-Z19B
 *           - 4 Winsen Datasheets
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
#ifndef _MHZ19_HPP
#define _MHZ19_HPP

//#define MHZ19_USE_MINIMAL_RAM                   // removes all field from the class, which are not required for just reading CO2 value
#include "MHZ19.h"

void MHZ19::setSerial(Stream *aSerial) {
    SerialToMHZ19 = aSerial;
    SerialToMHZ19->setTimeout(MHZ19_RESPONSE_TIMEOUT_MILLIS);
}

/**
 * Initializes send buffer, tests connection with standard command 0x86
 * and reads firmware version string
 * @return true if error happened during response receiving
 */
bool MHZ19::begin(Stream &aSerial) {
    return begin(&aSerial);
}

bool MHZ19::begin(Stream *aSerial) {
    SerialToMHZ19 = aSerial;
    SerialToMHZ19->setTimeout(MHZ19_RESPONSE_TIMEOUT_MILLIS);

    // preset send array once
    CommandToSend[0] = 0xFF;
    CommandToSend[1] = 0x01;
    CommandToSend[3] = 0;
    CommandToSend[4] = 0;
    CommandToSend[6] = 0;
    CommandToSend[7] = 0;

    VersionString[4] = '\0';

    /*
     * My MH-Z19C does not start after power up until first command, which gives timeout :-(.
     */
    // dummy command
    this->readCO2UnmaskedAndTemperatureFloat();
    delay(10); // value is guessed

    // Test connection with standard command
    if (this->readCO2UnmaskedAndTemperatureFloat()) {
        return true;
    }

    if (this->readVersion()) {
        return true;
    }
    return false;
}


/*
 * Datasheet: Checksum = NOT (Byte1+Byte2+Byte3+Byte4+Byte5+Byte6+Byte7))+1, Byte0 is start byte
 */
uint8_t MHZ19::computeChecksum(uint8_t *aArray) {
    uint8_t tChecksum = 0;
    for (uint_fast8_t i = 1; i < (MHZ19_DATA_LEN - 1); i++) {
        tChecksum += aArray[i];
    }
    return ~tChecksum + 1;
}

/**
 * Sends the 9 byte command stream, and receives the 9 byte response into ReceivedResponse
 * 9 bytes at 9600 baud takes 10 ms.
 * Timeout of MHZ19_RESPONSE_TIMEOUT_MILLIS (500) is specified in begin();
 * @return true if error happened during response receiving
 */
bool MHZ19::processCommand(MHZ19_command_t aCommand, bool aDoNotWaitForResponse) {
// Flush response buffer
    while (SerialToMHZ19->available()) {
        SerialToMHZ19->read();
    }

    CommandToSend[COMMAND_SEND_INDEX] = aCommand;
    CommandToSend[CHECKSUM_INDEX] = computeChecksum(CommandToSend);

    SerialToMHZ19->write(CommandToSend, MHZ19_DATA_LEN); // Start sending
    SerialToMHZ19->flush(); // wait to be sent

    if (!aDoNotWaitForResponse) {
        delay(12);
        return readResponse();
    }

    return false;
}

/**
 * Receives the 9 byte response into ReceivedResponse
 * Response starts 2 ms after end of request (23 ms for setAutocalibration())
 * Checks for checksum
 * Timeout of MHZ19_RESPONSE_TIMEOUT_MILLIS (500) is specified in begin();
 * @return true if error happened during response receiving
 */
bool MHZ19::readResponse() {
    /* response received, read buffer */
    if (SerialToMHZ19->readBytes(ReceivedResponse, MHZ19_DATA_LEN) != MHZ19_DATA_LEN) {
        this->errorCode = RESULT_TIMEOUT;
#if !defined(MHZ19_USE_MINIMAL_RAM)
        if (this->SerialDebugOutputIsEnabled) {
            SerialForDebug->print(F("Timeout error. Available="));
            SerialForDebug->println(SerialToMHZ19->available());
        }
#endif
        return true;
    }

#if !defined(MHZ19_USE_MINIMAL_RAM)
    if (this->SerialDebugOutputIsEnabled) {
        SerialForDebug->print(F(" Received cmd=0x"));
        SerialForDebug->print(ReceivedResponse[COMMAND_RECEIVE_INDEX], HEX);
        SerialForDebug->print(F("|"));
        printCommand((MHZ19_command_t) ReceivedResponse[COMMAND_RECEIVE_INDEX], &Serial);

        for (uint_fast8_t i = 2; i < 8; i += 2) {
            SerialForDebug->print(F("   0x"));
            SerialForDebug->print(ReceivedResponse[i], HEX);
            SerialForDebug->print(F(",0x"));
            SerialForDebug->print(ReceivedResponse[i + 1], HEX);
            SerialForDebug->print(F(" ="));
            SerialForDebug->print((uint16_t) (ReceivedResponse[i] << 8 | ReceivedResponse[i + 1])); // Unsigned decimal word
        }

        SerialForDebug->println();
    }
#endif

    uint8_t tChecksum = computeChecksum(ReceivedResponse);
    if (tChecksum != ReceivedResponse[CHECKSUM_INDEX]) {
        this->errorCode = RESULT_CHECKSUM;

#if !defined(MHZ19_USE_MINIMAL_RAM)
        if (this->SerialDebugOutputIsEnabled) {
            SerialForDebug->print(F("Checksum error. Received="));
            SerialForDebug->print(ReceivedResponse[CHECKSUM_INDEX]);
            SerialForDebug->print(F(" expected="));
            SerialForDebug->println(tChecksum);
        }
#endif
        return true;
    }

    if (CommandToSend[COMMAND_SEND_INDEX] != ReceivedResponse[COMMAND_RECEIVE_INDEX]) {
        this->errorCode = RESULT_MATCH;

#if !defined(MHZ19_USE_MINIMAL_RAM)
        if (this->SerialDebugOutputIsEnabled) {
            SerialForDebug->print(F("Command mismatch error. Sent=0x"));
            SerialForDebug->print(CommandToSend[COMMAND_SEND_INDEX], HEX);
            SerialForDebug->print(F(" received=0x"));
            SerialForDebug->println(ReceivedResponse[COMMAND_RECEIVE_INDEX], HEX);
        }
#endif
    }
    this->errorCode = RESULT_OK;
    return false;
}

void MHZ19::printErrorCode(Print *aSerial) {
    if (this->errorCode == 2) {
        aSerial->print(F("Timeout"));
    } else {
        aSerial->print(this->errorCode);
    }
}

void MHZ19::printErrorMessage(Print *aSerial) {
    aSerial->print(F("MHZ19: Response error code="));
    printErrorCode(aSerial);
    aSerial->println();
}

/*
 * Used for debugging
 */
void MHZ19::printCommand(MHZ19_command_t aCommand, Print *aSerial) {
    switch (aCommand) {
    case GETABC:
        aSerial->print(F("getABC   "));
        break;
    case CO2RAW:
        aSerial->print(F("getRawCO2"));
        break;
    case CO2_AND_TEMPERATURE:
        aSerial->print(F("getCO2   "));
        break;
    case CO2MASKED_AND_TEMP:
        aSerial->print(F("getCO2Msk"));
        break;
    case GETRANGE:
        aSerial->print(F("getRange "));
        break;
    case PERIOD:
        aSerial->print(F("getPeriod"));
        break;
    case GETCO2:
        aSerial->print(F("getCO2_2 "));
        break;
    case GETFIRMWARE_VERSION:
        aSerial->print(F("getFW    "));
        break;
    default:
        break;
    }

}


/**
 * Fills TemperatureFloat, CO2Unmasked and MinimumLightADC
 * Received cmd=0x85|getCO2      0x8,0x52 =2130   0x2,0x6F =623   0x3,0xC2 =962
 * @return true if error happened during response receiving
 */
bool MHZ19::readCO2UnmaskedAndTemperatureFloat() {
    if (processCommand(CO2_AND_TEMPERATURE)) { // 0x85
        return true;
    }
    this->TemperatureFloat = (ReceivedResponse[2] << 8 | ReceivedResponse[3]) / 100.0;
    this->CO2Unmasked = ReceivedResponse[4] << 8 | ReceivedResponse[5];
#if !defined(MHZ19_USE_MINIMAL_RAM)
    this->MinimumLightADC = ReceivedResponse[6] << 8 | ReceivedResponse[7]; // Observed 1013 to 1044
#endif
    return false;
}

#if !defined(MHZ19_USE_MINIMAL_RAM)


#endif

/**
 * fills VersionString
 * @return true if error happened during response receiving
 */
bool MHZ19::readVersion() {
    if (processCommand(GETFIRMWARE_VERSION)) {
        return true;
    }
    for (uint_fast8_t i = 0; i < 4; i++) {
        this->VersionString[i] = char(this->ReceivedResponse[i + 2]);
    }
#if !defined(MHZ19_USE_MINIMAL_RAM)
    this->VersionMajor = this->ReceivedResponse[3] - '0';
    this->VersionMinor = (this->ReceivedResponse[4] - '0') * 10;
    this->VersionMinor += this->ReceivedResponse[5] - '0';
    this->Version = (100 * this->VersionMajor) + this->VersionMinor;
#endif
    return false;
}

/*
 * Received cmd=0x7D|getABC      0x1,0x0 =256   0x0,0x0 =0   0x0,0x1 =1
 * @return true if AB_CON_OFF enabled
 */
bool MHZ19::readABC() {
    if (processCommand(GETABC)) { // 0x7D
        return true;
    }
    this->AutoBaselineCorrectionEnabled = this->ReceivedResponse[7]; // (1 - enabled, 0 - disabled)
    return false;
}

void MHZ19::setZeroCalibration() {
    processCommand(SETZERO_CALIBRATION); // 0x87
}

void MHZ19::setAutoCalibration(bool aSwitchOn) {
    if (aSwitchOn) {
        this->CommandToSend[3] = 0xA0; // set parameter to command buffer ??? is 0xA0 = 160 a counter value ???
    }
    processCommand(SETABC_ON_OFF); // 0x79
    this->CommandToSend[3] = 0x00; // clear parameter from command buffer
}

#if !defined(MHZ19_USE_MINIMAL_RAM)

void MHZ19::enableDebug(Print *aSerialForDebugOutput) {
    SerialForDebug = aSerialForDebugOutput;
    this->SerialDebugOutputIsEnabled = true;
}

void MHZ19::disableDebug() {
    this->SerialDebugOutputIsEnabled = false;
}

/*
 * Received cmd=0x86|getCO2Msk   0x2,0x6F =623   0x3F,0x0 =16128   0x7E,0x0 =32256
 * @return true if error happened during response receiving
 */
bool MHZ19::readCO2MaskedTemperatureIntAndABCCounter() {
    if (processCommand(CO2MASKED_AND_TEMP)) { // 0x86
        return true;
    }
    this->CO2 = ReceivedResponse[2] << 8 | ReceivedResponse[3];
    this->Temperature = ReceivedResponse[4] - TEMPERATURE_ADJUST_CONSTANT;
    this->ABCCounter = ReceivedResponse[6];
    return false;
}

/**
 * ADCRaw CO2 ADC value and temperature compensated zero ADC value - not from datasheet
 * Received cmd=0x84|getRawCO2   0xA2,0x8E =41614   0xA3,0xF5 =41973   0xB,0xE7 =3047
 * @return true if error happened during response receiving
 */
bool MHZ19::readCO2Raw() {
    if (processCommand(CO2RAW)) { // 0x84
        return true;
    }
    this->CO2RawADC = ReceivedResponse[2] << 8 | ReceivedResponse[3];
    this->CO2RawTemperatureCompensatedBaseADC = ReceivedResponse[4] << 8 | ReceivedResponse[5];
    this->Unknown2 = ReceivedResponse[6] << 8 | ReceivedResponse[7]; // Observed 0x0B57 to 0x0BEB

    if (this->Version >= 520) {
        this->CO2RawADCDelta = this->CO2RawTemperatureCompensatedBaseADC - this->CO2RawADC;
    } else {
        this->CO2RawADCDelta = this->CO2RawADC - this->CO2RawTemperatureCompensatedBaseADC;
    }
    return false;
}

/**
 * fills SensorRange
 * Received cmd=0x9B|getRange    0x0,0x0 =0   0x13,0x88 =5000   0x0,0x3 =3
 * @return true if error happened during response receiving
 */
bool MHZ19::readRange() {
    if (processCommand(GETRANGE)) {
        return true;
    }
    this->SensorRange = ReceivedResponse[4] << 8 | ReceivedResponse[5];
    return false;
}

/*
 * Received cmd=0x9C|getCO2_2    0x0,0x0 =0   0x2,0x6F =623   0x1,0x0 =256
 * last word seems to be at least short time constant
 */
bool MHZ19::readCO2Alternate() {
    if (processCommand(GETCO2)) {
        return true;
    }
    this->CO2Alternate = ReceivedResponse[4] << 8 | ReceivedResponse[5];
    return false;
}

/*
 * Reads timer cycle length in seconds and returns current value.
 * Received cmd=0x7E|getPeriod   0x0,0x2 =2   0x0,0x0 =0   0x0,0x0 =0
 */
bool MHZ19::readPeriod() {
    if (processCommand(PERIOD)) {
        return true;
    }

//    this->Period = ReceivedResponse[4] << 8 | ReceivedResponse[5]; // suggested by datasheet
    this->Period = ReceivedResponse[2] << 8 | ReceivedResponse[3]; // result in seconds, suggested by real data :-)
    return false;
}

/*
 * Sets timer cycle length (b[4..5]) in seconds and returns current value. b[3] should be 2 to update the length
 * Received cmd=0x7E|getPeriod   0x0,0x2 =2   0x0,0x0 =0   0x0,0x0 =0
 * Does not work :-(
 */
void MHZ19::setPeriod() {
    this->CommandToSend[3] = 4; // 2 or 4 makes no difference, it does not work
    this->CommandToSend[5] = 4;
    processCommand(PERIOD);
//    this->Period = ReceivedResponse[4] << 8 | ReceivedResponse[5]; // suggested by datasheet
    this->Period = ReceivedResponse[2] << 8 | ReceivedResponse[3]; // result in seconds, suggested by real data :-)
    this->CommandToSend[3] = 0;
    this->CommandToSend[5] = 0;
}

/*
 * Span calibration Note: do ZERO calibration before span calibration -
 * From datasheet:
 * Note: Pls do ZERO calibration before span calibration
 * Please make sure the sensor worked under a certain level co2 for over 20 minutes.
 * Suggest using 2000ppm as span, at least 1000ppm
 */
void MHZ19::setSpanCalibration(uint16_t aValueOfCurrentCO2) {
    this->CommandToSend[3] = aValueOfCurrentCO2 >> 8; // high byte
    this->CommandToSend[4] = aValueOfCurrentCO2; // low byte
    processCommand(SETSPAN_CALIBRATION); // 0x79
    // clear parameter from command buffer
    this->CommandToSend[3] = 0x00;
    this->CommandToSend[4] = 0x00;
}

/*
 * English datasheet uses byte 3 and 4, Chinese one uses byte 4 to 7 for 32 bit value and so effectively byte 6 and 7
 * From datasheet:
 * Note: Detection range is 2000ppm or 5000ppm or 10000ppm as mentioned in Chinese datasheet
 */
void MHZ19::setRange(uint16_t aRange) {
    /*
     * This is from English datasheet
     */
//    if (aRange == 2000) {
//        this->CommandToSend[3] = 2000 >> 8; // 0x07 high byte
//        this->CommandToSend[4] = 2000 % 8;  // 0xD0 low byte
//    } else if (aRange == 10000) {
//        this->CommandToSend[3] = 10000 >> 8; // high byte
//        this->CommandToSend[4] = 10000 % 8; // low byte
//    } else {
//        this->CommandToSend[3] = 5000 >> 8; // high byte
//        this->CommandToSend[4] = 5000 % 8; // low byte
//    }
    /*
     * This is the Chinese version
     */
    if (aRange == 2000) {
        this->CommandToSend[6] = 2000 >> 8; // 0x07 high byte
        this->CommandToSend[7] = 2000 % 8;  // 0xD0 low byte
    } else if (aRange == 10000) {
        this->CommandToSend[6] = 10000 >> 8; // high byte
        this->CommandToSend[7] = 10000 % 8; // low byte
    } else {
        this->CommandToSend[6] = 5000 >> 8; // high byte
        this->CommandToSend[7] = 5000 % 8; // low byte
    }
    processCommand(SETSPAN_CALIBRATION); // 0x79
    // clear parameter from command buffer
    this->CommandToSend[3] = 0x00;
    this->CommandToSend[4] = 0x00;
}
#endif // !defined(MHZ19_USE_MINIMAL_RAM)
#endif
