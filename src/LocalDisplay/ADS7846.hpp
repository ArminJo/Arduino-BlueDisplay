/**
 * ADS7846.hpp
 *
 * Firmware for the TI ADS7846 resistive touch controller
 *
 *  Based on ADS7846.cpp  https://github.com/watterott/Arduino-Libs/tree/master/ADS7846
 *
 *
 *  Copyright (C) 2012-2023  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay https://github.com/ArminJo/android-blue-display.
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

#ifndef _ADS7846_HPP
#define _ADS7846_HPP

#include "ADS7846.h"
#include "EventHandler.h"
#include "LocalEventHelper.h"

#if defined(AVR)
void ADS7846_IO_initalize(void);
#else
#include "STM32TouchScreenDriver.h"
#endif

ADS7846 TouchPanel; // The instance provided by the class itself

#define TOUCH_DELAY_AFTER_READ_MILLIS 3
#define TOUCH_DEBOUNCE_DELAY_MILLIS 10 // wait for debouncing in ISR - minimum 8 ms

const char StringPosZ1[] PROGMEM = "Z Pos 1";
const char StringPosZ2[] PROGMEM = "Z Pos 2";
const char StringPosX[] PROGMEM = "X Pos";
const char StringPosY[] PROGMEM = "Y Pos";
const char StringTemperature0[] PROGMEM = "Temp. 0";
const char StringTemperature1[] PROGMEM = "Temp. 1";
const char StringVcc[] PROGMEM = "VCC";
const char StringAux[] PROGMEM = "Aux In";
const char *const ADS7846ChannelStrings[] = { StringPosZ1, StringPosZ2, StringPosX, StringPosY, StringTemperature0,
        StringTemperature1, StringVcc, StringAux };
const char ADS7846ChannelChars[] = { 'z', 'Z', 'X', 'Y', 't', 'T', 'V', 'A' };

// Channel number to text mapping
unsigned char ADS7846ChannelMapping[] = { 3, 4, 1, 5, 0, 7, 2, 6 };

// to start quick if backup battery was not assembled or empty
const CAL_MATRIX sInitalMatrix = { 320300, -1400, -52443300, -3500, 237700, -21783300, 1857905 };

#if defined(AVR)
//#define SOFTWARE_SPI
#ifdef __AVR_ATmega32U4__
#define SOFTWARE_SPI
#endif

#if (defined(__AVR_ATmega1280__) || \
     defined(__AVR_ATmega1281__) || \
     defined(__AVR_ATmega2560__) || \
     defined(__AVR_ATmega2561__))      //--- Arduino Mega ---
#define SOFTWARE_SPI
//# define BUSY_PIN       (5)
//# define IRQ_PIN        (3)
# define ADS7846_CS_PIN (6)
# if defined(SOFTWARE_SPI)
#  define MOSI_PIN      (11)
#  define MISO_PIN      (12)
#  define CLK_PIN       (13)
# else
#  define MOSI_PIN      (51)
#  define MISO_PIN      (50)
#  define CLK_PIN       (52)
# endif

#elif (defined(__AVR_ATmega644__) || \
       defined(__AVR_ATmega644P__))    //--- Arduino 644 ---
//# define BUSY_PIN       (15)
//# define IRQ_PIN        (11)
# define ADS7846_CS_PIN (14)
# define MOSI_PIN       (5)
# define MISO_PIN       (6)
# define CLK_PIN        (7)

#else                                  //--- Arduino Uno ---
//# define BUSY_PIN       (5)
//# define IRQ_PIN        (3)
# define ADS7846_CS_PIN (6)
# define MOSI_PIN       (11)
# define MISO_PIN       (12)
# define CLK_PIN        (13)

#endif

#define ADS7846_CS_DISABLE()    digitalWriteFast(ADS7846_CS_PIN, HIGH)
#define ADS7846_CS_ENABLE()     digitalWriteFast(ADS7846_CS_PIN, LOW)

#define MOSI_HIGH()     digitalWriteFast(MOSI_PIN, HIGH)
#define MOSI_LOW()      digitalWriteFast(MOSI_PIN, LOW)

#define MISO_READ()     digitalReadFast(MISO_PIN)

#define CLK_HIGH()      digitalWriteFast(CLK_PIN, HIGH)
#define CLK_LOW()       digitalWriteFast(CLK_PIN, LOW)
#endif // defined(AVR)

//-------------------- Constructor --------------------

ADS7846::ADS7846() { // @suppress("Class members should be properly initialized")
}

//-------------------- Public --------------------

void ADS7846::init(void) {
    //init pins
    ADS7846_IO_initalize();

    //set vars
    tp_matrix.div = 0;
    mTouchActualPositionRaw.PositionX = 0;
    mTouchActualPositionRaw.PositionY = 0;
    mTouchLastCalibratedPositionRaw.PositionX = 0;
    mTouchLastCalibratedPositionRaw.PositionY = 0;
//    mTouchActualPosition.PositionX = 0;
//    mTouchActualPosition.PositionY = 0;
    mPressure = 0;
}

bool ADS7846::setCalibration(CAL_POINT *aTargetValues, CAL_POINT *aRawValues) {
    tp_matrix.div = ((aRawValues[0].x - aRawValues[2].x) * (aRawValues[1].y - aRawValues[2].y))
            - ((aRawValues[1].x - aRawValues[2].x) * (aRawValues[0].y - aRawValues[2].y));

    if (tp_matrix.div == 0) {
        return false;
    }

    tp_matrix.a = ((aTargetValues[0].x - aTargetValues[2].x) * (aRawValues[1].y - aRawValues[2].y))
            - ((aTargetValues[1].x - aTargetValues[2].x) * (aRawValues[0].y - aRawValues[2].y));

    tp_matrix.b = ((aRawValues[0].x - aRawValues[2].x) * (aTargetValues[1].x - aTargetValues[2].x))
            - ((aTargetValues[0].x - aTargetValues[2].x) * (aRawValues[1].x - aRawValues[2].x));

    tp_matrix.c = (aRawValues[2].x * aTargetValues[1].x - aRawValues[1].x * aTargetValues[2].x) * aRawValues[0].y
            + (aRawValues[0].x * aTargetValues[2].x - aRawValues[2].x * aTargetValues[0].x) * aRawValues[1].y
            + (aRawValues[1].x * aTargetValues[0].x - aRawValues[0].x * aTargetValues[1].x) * aRawValues[2].y;

    tp_matrix.d = ((aTargetValues[0].y - aTargetValues[2].y) * (aRawValues[1].y - aRawValues[2].y))
            - ((aTargetValues[1].y - aTargetValues[2].y) * (aRawValues[0].y - aRawValues[2].y));

    tp_matrix.e = ((aRawValues[0].x - aRawValues[2].x) * (aTargetValues[1].y - aTargetValues[2].y))
            - ((aTargetValues[0].y - aTargetValues[2].y) * (aRawValues[1].x - aRawValues[2].x));

    tp_matrix.f = (aRawValues[2].x * aTargetValues[1].y - aRawValues[1].x * aTargetValues[2].y) * aRawValues[0].y
            + (aRawValues[0].x * aTargetValues[2].y - aRawValues[2].x * aTargetValues[0].y) * aRawValues[1].y
            + (aRawValues[1].x * aTargetValues[0].y - aRawValues[0].x * aTargetValues[1].y) * aRawValues[2].y;

    return true;
}

#if defined(AVR)
bool ADS7846::writeCalibration(uint16_t eeprom_addr) {
    if (tp_matrix.div != 0) {
        eeprom_write_byte((uint8_t*) eeprom_addr++, 0x55);
        eeprom_write_block((void*) &tp_matrix, (void*) eeprom_addr, sizeof(CAL_MATRIX));
        return true;
    }
    return false;
}

bool ADS7846::readCalibration(uint16_t eeprom_addr) {
    uint8_t c;

    c = eeprom_read_byte((uint8_t*) eeprom_addr++);
    if (c == 0x55) {
        eeprom_read_block((void*) &tp_matrix, (void*) eeprom_addr, sizeof(CAL_MATRIX));
        return true;
    }
    return false;
}
#else
void ADS7846::writeCalibration(CAL_MATRIX aMatrix) {
    HAL_PWR_EnableBkUpAccess();
    // Write data
    HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR2, aMatrix.a);
    HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR3, aMatrix.b);
    HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR4, aMatrix.c);
    HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR5, aMatrix.d);
    HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR6, aMatrix.e);
    HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR7, aMatrix.f);
    HAL_RTCEx_BKUPWrite(&RTCHandle, RTC_BKP_DR8, aMatrix.div);
    RTC_PWRDisableBkUpAccess();
}

/**
 *
 * @return true if calibration data valid and matrix is set
 */
void ADS7846::readCalibration(CAL_MATRIX *aMatrix) {
    aMatrix->a = HAL_RTCEx_BKUPRead(&RTCHandle, RTC_BKP_DR2);
    aMatrix->b = HAL_RTCEx_BKUPRead(&RTCHandle, RTC_BKP_DR3);
    aMatrix->c = HAL_RTCEx_BKUPRead(&RTCHandle, RTC_BKP_DR4);
    aMatrix->d = HAL_RTCEx_BKUPRead(&RTCHandle, RTC_BKP_DR5);
    aMatrix->e = HAL_RTCEx_BKUPRead(&RTCHandle, RTC_BKP_DR6);
    aMatrix->f = HAL_RTCEx_BKUPRead(&RTCHandle, RTC_BKP_DR7);
    aMatrix->div = HAL_RTCEx_BKUPRead(&RTCHandle, RTC_BKP_DR8);
}
#endif // defined(AVR)

/**
 * touch panel calibration routine
 * On my MI0283QT calibration points must be touched quick and hard to get good results
 */
#if defined(AVR)
void ADS7846::doCalibration(uint16_t aEEPROMAdress, bool aCheckEEPROM)
#else
void ADS7846::doCalibration(bool aCheckRTC)
#endif
        {
    CAL_POINT tReferencePoints[3] = { CAL_POINT1, CAL_POINT2, CAL_POINT3 }; // calibration point positions
    CAL_POINT tRawPoints[3];

#if defined(AVR)
    //calibration data in EEPROM?
    if (readCalibration(aEEPROMAdress) && aCheckEEPROM) {
        return;
    }
#else
    if (aCheckRTC) {
        //calibration data in CMOS RAM?
        if (RTC_checkMagicNumber()) {
            readCalibration(&tp_matrix);
        } else {
            // not valid -> copy initial values to data structure
            writeCalibration(sInitalMatrix);
            readCalibration(&tp_matrix);
            //It is workaround for not working read + writeCalibration
            if (tp_matrix.a == 0) {
                tp_matrix = sInitalMatrix;
            }
        }
        return;
    }
#endif

    //show calibration points
    for (uint8_t i = 0; i < 3; i++) {
        /*
         * Clear screen
         */
        LocalDisplay.clearDisplay(COLOR16_WHITE);
        LocalDisplay.drawText((LOCAL_DISPLAY_WIDTH / 2) - 50, (LOCAL_DISPLAY_HEIGHT / 2) - 10, F("Calibration"), 1,
                COLOR16_BLACK, COLOR16_WHITE);
        // Wait for touch release
        do {
            readData();
        } while (getPressure() >= MIN_REASONABLE_PRESSURE);

        // Draw current point
        LocalDisplay.drawCircle(tReferencePoints[i].x, tReferencePoints[i].y, 2, COLOR16_BLACK);
        LocalDisplay.drawCircle(tReferencePoints[i].x, tReferencePoints[i].y, 5, COLOR16_BLACK);
        LocalDisplay.drawCircle(tReferencePoints[i].x, tReferencePoints[i].y, 10, COLOR16_RED);

        // wait for touch to become active
#if defined(AVR)
        delay(100);
        do {
            readData();
        } while (getPressure() < 2 * MIN_REASONABLE_PRESSURE);

        // wait for stabilizing data
        delay(20);
        readData();

#else
        while (!TouchPanel.wasJustTouched()) {
            delay(5);
        }
        // wait for stabilizing data
        delay(20);
        //get new data
        readData(4 * ADS7846_READ_OVERSAMPLING_DEFAULT);
        // reset touched flag
        TouchPanel.wasJustTouched();
#endif

        //press detected -> save point
        LocalDisplay.fillCircle(tReferencePoints[i].x, tReferencePoints[i].y, 2, COLOR16_RED);
        tRawPoints[i].x = getRawX();
        tRawPoints[i].y = getRawY();
#if defined(AVR)
        delay(1000);
#else
        delay(1000);
#endif
    }

    // Calculate calibration matrix
    setCalibration(tReferencePoints, tRawPoints);

    if (tp_matrix.div != 0) {
#if defined(AVR)
        //save calibration matrix and flag for valid data
        writeCalibration(aEEPROMAdress);
        // Wait for touch release
        do {
            readData();
        } while (getPressure() >= 2 * MIN_REASONABLE_PRESSURE);
#else
        //save calibration matrix and flag for valid data
        RTC_setMagicNumber();
        writeCalibration(tp_matrix);
#endif
    }
}

/*
 * Init Touch panel and calibrate it, if it is pressed (at startup)
 */
#if defined(AVR)
void ADS7846::initAndCalibrateOnPress(uint16_t aEEPROMAdress)
#else
void ADS7846::initAndCalibrateOnPress()
#endif
        {
    init();
    TouchPanel.readData();
#if defined(AVR)
    if (getPressure() >= MIN_REASONABLE_PRESSURE) {
        doCalibration(aEEPROMAdress, false); // Don't check EEPROM for calibration data
    } else {
        doCalibration(aEEPROMAdress, true); // Check EEPROM for calibration data
    }
#else
    if (getPressure() >= MIN_REASONABLE_PRESSURE) {
        doCalibration(false); // Don't check RTC for calibration data
    } else {
        doCalibration(true); // Check RTC for calibration data
    }
#endif
}

/**
 * convert raw to calibrated position in mTouchActualPosition
 * mTouchActualPositionRaw -> mTouchActualPosition
 */
void ADS7846::calibrate(void) {
    long x, y;

    // calculate x position
    if (mTouchLastCalibratedPositionRaw.PositionX != mTouchActualPositionRaw.PositionX) {
        mTouchLastCalibratedPositionRaw.PositionX = mTouchActualPositionRaw.PositionX;
        x = mTouchActualPositionRaw.PositionX;
        y = mTouchActualPositionRaw.PositionY;
        x = ((tp_matrix.a * x) + (tp_matrix.b * y) + tp_matrix.c) / tp_matrix.div;
        if (x < 0) {
            x = 0;
        } else if (x >= LOCAL_DISPLAY_WIDTH) {
            x = LOCAL_DISPLAY_WIDTH - 1;
        }
        mCurrentTouchPosition.PositionX = x;
    }

    // calculate y position
    if (mTouchLastCalibratedPositionRaw.PositionY != mTouchActualPositionRaw.PositionY) {
        mTouchLastCalibratedPositionRaw.PositionY = mTouchActualPositionRaw.PositionY;
        x = mTouchActualPositionRaw.PositionX;
        y = mTouchActualPositionRaw.PositionY;
        y = ((tp_matrix.d * x) + (tp_matrix.e * y) + tp_matrix.f) / tp_matrix.div;
        if (y < 0) {
            y = 0;
        } else if (y >= LOCAL_DISPLAY_HEIGHT) {
            y = LOCAL_DISPLAY_HEIGHT - 1;
        }
        mCurrentTouchPosition.PositionY = y;
    }
}

uint16_t ADS7846::getRawX(void) {
    return mTouchActualPositionRaw.PositionX;
}

uint16_t ADS7846::getRawY(void) {
    return mTouchActualPositionRaw.PositionY;
}

uint16_t ADS7846::getCurrentX(void) {
    return mCurrentTouchPosition.PositionX;
}

uint16_t ADS7846::getCurrentY(void) {
    return mCurrentTouchPosition.PositionY;
}

uint8_t ADS7846::getPressure(void) {
    return mPressure;
}

#if defined(AVR)
/**
 * We do a 8 times oversampling here
 */
void ADS7846::readData(void) {
    uint8_t a, b, i;
    uint16_t x, y;

    //SPI speed-down
#  if !defined(SOFTWARE_SPI)
    uint8_t spcr, spsr;
    spcr = SPCR;
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0); //enable SPI, Master, clk=Fcpu/16
    spsr = SPSR;
    SPSR = (1 << SPI2X); //clk*2 -> clk=Fcpu/8
#  endif

    //get mPressure
    ADS7846_CS_ENABLE();
    wr_spi(CMD_START | CMD_8BIT | CMD_DIFF | CMD_Z1_POS);
    a = rd_spi();
    wr_spi(CMD_START | CMD_8BIT | CMD_DIFF | CMD_Z2_POS);
    b = 127 - rd_spi();
    ADS7846_CS_DISABLE();
    mPressure = a + b;

    if (mPressure >= MIN_REASONABLE_PRESSURE) {
        for (x = 0, y = 0, i = 8; i != 0; i--) //8 samples
                {
            ADS7846_CS_ENABLE();
            //get X data
            wr_spi(CMD_START | CMD_12BIT | CMD_DIFF | CMD_X_POS);
            a = rd_spi();
            b = rd_spi();
            x += 1023 - ((a << 2) | (b >> 6)); //12bit: ((a<<4)|(b>>4)) //10bit: ((a<<2)|(b>>6))
            //get Y data
            wr_spi(CMD_START | CMD_12BIT | CMD_DIFF | CMD_Y_POS);
            a = rd_spi();
            b = rd_spi();
            y += ((a << 2) | (b >> 6));                //12bit: ((a<<4)|(b>>4)) //10bit: ((a<<2)|(b>>6))
            ADS7846_CS_DISABLE();
        }
        x >>= 3; //x/8
        y >>= 3; //y/8

        if ((x >= 10) && (y >= 10)) {
            mTouchActualPositionRaw.PositionX = x;
            mTouchActualPositionRaw.PositionY = y;
            calibrate();
            if (!ADS7846TouchActive) {
                /*
                 * Here we have a touch down event
                 */
                mTouchDownPosition.PositionX = x;
                mTouchDownPosition.PositionY = y;
                ADS7846TouchStart = true; // flag is only reset externally by call of wasJustTouched()
            }
            ADS7846TouchActive = true;
        }
    } else {
        ADS7846TouchActive = false;
        mPressure = 0;
    }

    //restore SPI settings
#  if !defined(SOFTWARE_SPI)
    SPCR = spcr;
    SPSR = spsr;
#  endif
}

/**
 * Read individual A/D channels like temperature or Vcc
 */
uint16_t ADS7846::readChannel(uint8_t channel, bool use12Bit, bool useDiffMode, uint8_t numberOfReadingsToIntegrate) {
    channel <<= 4;
    // mask for channel 0 to 7
    channel &= CHANNEL_MASK;
    uint16_t tRetValue = 0;
    uint8_t low, high, i;

//SPI speed-down
#  if !defined(SOFTWARE_SPI)
    uint8_t spcr, spsr;
    spcr = SPCR;
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0); //enable SPI, Master, clk=Fcpu/16
    spsr = SPSR;
    SPSR = (1 << SPI2X); //clk*2 -> clk=Fcpu/8
#  endif

//read channel
    uint8_t tMode = CMD_SINGLE;
    if (useDiffMode) {
        tMode = CMD_DIFF;
    }

    ADS7846_CS_ENABLE();
    for (i = numberOfReadingsToIntegrate; i != 0; i--) {
        if (use12Bit) {
            wr_spi(CMD_START | CMD_12BIT | tMode | channel);
            high = rd_spi();
            low = rd_spi();
            tRetValue += (high << 4) | (low >> 4); //12bit: ((a<<4)|(b>>4)) //10bit: ((a<<2)|(b>>6))
        } else {
            wr_spi(CMD_START | CMD_8BIT | tMode | channel);
            tRetValue += rd_spi();
        }
    }
    ADS7846_CS_DISABLE();
//restore SPI settings
#  if !defined(SOFTWARE_SPI)
    SPCR = spcr;
    SPSR = spsr;
#  endif

    return tRetValue / numberOfReadingsToIntegrate;
}

#else // defined(AVR)
    void ADS7846::readData(void) {
        readData(ADS7846_READ_OVERSAMPLING_DEFAULT);
    }

    /**
     * 3.3 ms at SPI_BaudRatePrescaler_256 and 16 times oversampling
     * 420 us at SPI_BaudRatePrescaler_64 and x4/y8 times oversampling
     * @param aOversampling for X. Y is oversampled by (2 *aOversampling).
     */
    void ADS7846::readData(uint8_t aOversampling) {
        uint8_t a, b;
        int i;
        uint32_t tXValue, tYValue;

//  Set_DebugPin();
        /*
         * SPI speed-down
         * datasheet says: optimal is CLK < 125kHz (40-80 kHz)
         * slowest frequency for SPI1 is 72 MHz / 256 = 280 kHz
         * but results for divider 64 looks as good as for 256
         * results for divider 16 are offsetted and not usable
         */
        uint16_t tPrescaler = SPI1_getPrescaler();
        SPI1_setPrescaler (SPI_BAUDRATEPRESCALER_64); // 72 MHz / 256 = 280 kHz, /64 = 1,1Mhz

//Disable interrupt because while ADS7846 is read int line goes low (active)
        ADS7846_disableInterrupt();

//get pressure
        ADS7846_CSEnable();
        SPI1_sendReceiveFast(CMD_START | CMD_8BIT | CMD_DIFF | CMD_Z1_POS);
        a = SPI1_sendReceiveFast(0);
        SPI1_sendReceiveFast(CMD_START | CMD_8BIT | CMD_DIFF | CMD_Z2_POS);
        b = SPI1_sendReceiveFast(0);
        b = 127 - b;// 127 is maximum reading of CMD_Z2_POS!
        int tPressure = a + b;

        if (tPressure >= MIN_REASONABLE_PRESSURE) {
            // n times oversampling
            unsigned int j = 0;
            for (tXValue = 0, tYValue = 0, i = aOversampling; i != 0; i--) {
                //get X data
                SPI1_sendReceiveFast(CMD_START | CMD_12BIT | CMD_DIFF | CMD_X_POS);
                a = SPI1_sendReceiveFast(0);
                b = SPI1_sendReceiveFast(0);
                uint32_t tX = (a << 5) | (b >> 3);//12bit: ((a<<5)|(b>>3)) //10bit: ((a<<3)|(b>>5))
                if (tX >= 4000) {
                    // no reasonable value
                    break;
                }
                tXValue += 4048 - tX;

                //get Y data twice, because it is noisier than Y
                SPI1_sendReceiveFast(CMD_START | CMD_12BIT | CMD_DIFF | CMD_Y_POS);
                a = SPI1_sendReceiveFast(0);
                b = SPI1_sendReceiveFast(0);
                uint32_t tY = (a << 5) | (b >> 3);//12bit: ((a<<5)|(b>>3)) //10bit: ((a<<3)|(b>>5))
                if (tY <= 100) {
                    // no reasonable value
                    break;
                }
                tYValue += tY;

                SPI1_sendReceiveFast(CMD_START | CMD_12BIT | CMD_DIFF | CMD_Y_POS);
                a = SPI1_sendReceiveFast(0);
                b = SPI1_sendReceiveFast(0);
                tY = (a << 5) | (b >> 3); //12bit: ((a<<5)|(b>>3)) //10bit: ((a<<3)|(b>>5))
                tYValue += tY;

                j += 2;// +2 to get 11 bit values at the end
            }
            if (j == (aOversampling * 2)) {
                // scale down to 11 bit because calibration does not work with 12 bit values
                tXValue /= j;
                tYValue /= 2 * j;

                // plausi check - is pressure still greater than 7/8 start pressure
                SPI1_sendReceiveFast(CMD_START | CMD_8BIT | CMD_DIFF | CMD_Z1_POS);
                a = SPI1_sendReceiveFast(0);
                SPI1_sendReceiveFast(CMD_START | CMD_8BIT | CMD_DIFF | CMD_Z2_POS);
                b = SPI1_sendReceiveFast(0);
                b = 127 - b;// 127 is maximum reading of CMD_Z2_POS!

                // plausi check - x raw value is from 130 to 3900 here x is (4048 - x)/2, y raw is from 150 to 3900 - low is upper right corner
                if (((a + b) > (tPressure - (tPressure >> 3))) && (tXValue >= 10) && (tYValue >= 10)) {
                    mTouchActualPositionRaw.PositionX = tXValue;
                    mTouchActualPositionRaw.PositionY = tYValue;
                    calibrate();
                    mPressure = tPressure;
                    ADS7846TouchActive = true;
                }
            }
        } else {
            mPressure = 0;
            ADS7846TouchActive = false;
        }

        ADS7846_CSDisable();
//restore SPI settings
        SPI1_setPrescaler(tPrescaler);
// enable interrupts after some ms in order to wait for the interrupt line to go high  - minimum 3 ms (2ms give errors)
        changeDelayCallback(&ADS7846_clearAndEnableInterrupt, TOUCH_DELAY_AFTER_READ_MILLIS);
        return;
    }

    /**
     * Read individual A/D channels like temperature or Vcc
     */
    uint16_t ADS7846::readChannel(uint8_t channel, bool use12Bit, bool useDiffMode, uint8_t numberOfReadingsToIntegrate) {
        channel <<= 4;
// mask for channel 0 to 7
        channel &= CHANNEL_MASK;
        uint16_t tRetValue = 0;
        uint8_t low, high, i;

//SPI speed-down
        uint16_t tPrescaler = SPI1_getPrescaler();
        SPI1_setPrescaler (SPI_BAUDRATEPRESCALER_64);

// disable interrupts for some ms in order to wait for the interrupt line to go high
        ADS7846_disableInterrupt();// only needed for X, Y + Z channel

//read channel
        ADS7846_CSEnable();
        uint8_t tMode = CMD_SINGLE;
        if (useDiffMode) {
            tMode = CMD_DIFF;
        }
        for (i = numberOfReadingsToIntegrate; i != 0; i--) {
            if (use12Bit) {
                SPI1_sendReceiveFast(CMD_START | CMD_12BIT | tMode | channel);
                high = SPI1_sendReceiveFast(0);
                low = SPI1_sendReceiveFast(0);
                tRetValue += (high << 5) | (low >> 3);
            } else {
                SPI1_sendReceiveFast(CMD_START | CMD_8BIT | tMode | channel);
                tRetValue += SPI1_sendReceiveFast(0);
            }
        }
        ADS7846_CSDisable();
// enable interrupts after some ms in order to wait for the interrupt line to go high - minimum 3 ms
        changeDelayCallback(&ADS7846_clearAndEnableInterrupt, TOUCH_DELAY_AFTER_READ_MILLIS);

//restore SPI settings
        SPI1_setPrescaler(tPrescaler);

        return tRetValue / numberOfReadingsToIntegrate;
    }
#endif // defined(AVR)

/**
 * Can be called by main loops
 * returns only one times a "true" per touch
 */ //
bool ADS7846::wasJustTouched(void) {
    if (ADS7846TouchStart) {
        // reset => return only one true per touch
        ADS7846TouchStart = false;
        return true;
    }
    return false;
}

//-------------------- Private --------------------

#if defined(AVR)
void ADS7846_IO_initalize(void) {
    //init pins
    pinMode(ADS7846_CS_PIN, OUTPUT);
    ADS7846_CS_DISABLE();
    pinMode(CLK_PIN, OUTPUT);
    pinMode(MOSI_PIN, OUTPUT);
    pinMode(MISO_PIN, INPUT);
    digitalWriteFast(MISO_PIN, HIGH);
    //pull-up
#  ifdef IRQ_PIN
        pinMode(IRQ_PIN, INPUT);
        digitalWriteFast(IRQ_PIN, HIGH); //pull-up
#  endif
#  ifdef BUSY_PIN
        pinMode(BUSY_PIN, INPUT);
        digitalWriteFast(BUSY_PIN, HIGH); //pull-up
#  endif

#  if !defined(SOFTWARE_SPI)
    //SS has to be output or input with pull-up
#    if (defined(__AVR_ATmega1280__) || \
      defined(__AVR_ATmega1281__) || \
      defined(__AVR_ATmega2560__) || \
      defined(__AVR_ATmega2561__))     //--- Arduino Mega ---
#  define SS_PORTBIT (0) //PB0
#    elif (defined(__AVR_ATmega644__) || \
        defined(__AVR_ATmega644P__))   //--- Arduino 644 ---
#  define SS_PORTBIT (4) //PB4
#    else                                 //--- Arduino Uno ---
#  define SS_PORTBIT (2) //PB2
#    endif
    if (!(DDRB & (1 << SS_PORTBIT))) //SS is input
    {
        PORTB |= (1 << SS_PORTBIT); //pull-up on
    }

#  endif
}

uint8_t ADS7846::rd_spi(void) {
#  if defined(SOFTWARE_SPI)
    uint8_t bit, data;

    MOSI_LOW();
    for(data=0, bit=8; bit!=0; bit--)
    {
        CLK_HIGH();
        data <<= 1;
        if(MISO_READ())
        {
            data |= 1;
        }
        else
        {
            //data |= 0;
        }
        CLK_LOW();
    }
    return data;

#  else
    SPDR = 0x00;
    while (!(SPSR & (1 << SPIF)))
        ;
    return SPDR;
#  endif
}

void ADS7846::wr_spi(uint8_t data) {
#  if defined(SOFTWARE_SPI)
    uint8_t mask;

    for(mask=0x80; mask!=0; mask>>=1)
    {
        CLK_LOW();
        if(mask & data)
        {
            MOSI_HIGH();
        }
        else
        {
            MOSI_LOW();
        }
        CLK_HIGH();
    }
    CLK_LOW();

#  else
    SPDR = data;
    while (!(SPSR & (1 << SPIF)))
        ;
#  endif
}
#endif // defined(AVR)


#endif //_ADS7846_HPP
