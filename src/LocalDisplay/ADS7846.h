/*
 * ADS7846.h
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
 *
 */

#ifndef _ADS7846_H
#define _ADS7846_H

#include <stdint.h>
#include "BlueDisplayProtocol.h" // for struct XYPosition

#define CAL_POINT_X1 (20)
#define CAL_POINT_Y1 (20)
#define CAL_POINT1   {CAL_POINT_X1,CAL_POINT_Y1}

#define CAL_POINT_X2 (300)
#define CAL_POINT_Y2 (120)
#define CAL_POINT2   {CAL_POINT_X2,CAL_POINT_Y2}

#define CAL_POINT_X3 (160)
#define CAL_POINT_Y3 (220)
#define CAL_POINT3   {CAL_POINT_X3,CAL_POINT_Y3}

#define MIN_REASONABLE_PRESSURE 9   // depends on position :-(( even slight touch gives more than this
#define MAX_REASONABLE_PRESSURE 110 // Greater means panel not connected
/*
 * without oversampling data is very noisy i.e oversampling of 2 is not suitable for drawing (it gives x +/-1 and y +/-2 pixel noise)
 * since Y value has much more noise than X, Y is oversampled 2 times.
 * 4 is reasonable, 8 is pretty good y +/-1 pixel
 */
#define ADS7846_READ_OVERSAMPLING_DEFAULT 4

// A/D input channel for readChannel()
#define CMD_TEMP0       (0x00)
// 2,5V reference 2,1 mV/Celsius 600 mV at 25 Celsius 12 Bit
// 25 Celsius reads 983 / 0x3D7 and 1 celsius is 3,44064 => Celsius is 897 / 0x381
#define CMD_X_POS       (0x10)
#define CMD_BATT        (0x20) // read Vcc /4
#define CMD_Z1_POS      (0x30)
#define CMD_Z2_POS      (0x40)
#define CMD_Y_POS       (0x50)
#define CMD_AUX         (0x60)
#define CMD_TEMP1       (0x70)

#define CHANNEL_MASK    (0x70)

#define CMD_START       (0x80)
#define CMD_12BIT       (0x00)
#define CMD_8BIT        (0x08)
#define CMD_DIFF        (0x00)
#define CMD_SINGLE      (0x04)

typedef struct {
    long x;
    long y;
} CAL_POINT; // only for calibrating purposes

typedef struct {
    long a;
    long b;
    long c;
    long d;
    long e;
    long f;
    long div;
} CAL_MATRIX;

#define ADS7846_CHANNEL_COUNT 8 // The number of ADS7846 channel
extern const char *const ADS7846ChannelStrings[ADS7846_CHANNEL_COUNT];
extern const char ADS7846ChannelChars[ADS7846_CHANNEL_COUNT];
// Channel number to text mapping
extern unsigned char ADS7846ChannelMapping[ADS7846_CHANNEL_COUNT];

class ADS7846 {
public:

    struct XYPosition mCurrentTouchPosition; // calibrated (screen) position
    struct XYPosition mLastTouchPosition;    // for suppressing of pseudo or micro moves for generating of move event and for touch up event position
    struct XYPosition mTouchDownPosition;    // Required for move, long touch down and swipe
    uint8_t mPressure; // touch panel pressure can be 0 or >= MIN_REASONABLE_PRESSURE

#if defined(AVR)
    bool ADS7846TouchActive = false; // is true as long as touch lasts
    bool ADS7846TouchStart = false; // is true once for every touch is reset by call to
#else
    volatile bool ADS7846TouchActive = false; // is true as long as touch lasts
    volatile bool ADS7846TouchStart = false; // is true once for every touch - independent from calling mLongTouchCallback
#endif

    ADS7846();
    void init(void);
    void readData(void);
    uint8_t getPressure(void);
    bool wasJustTouched(void);

#if defined(AVR)
    void doCalibration(uint16_t eeprom_addr, bool check_eeprom);
    void initAndCalibrateOnPress(uint16_t eeprom_addr);
#else
    void readData(uint8_t aOversampling);
    void doCalibration(bool aCheckRTC);
    void initAndCalibrateOnPress();
#endif

    uint16_t getRawX(void);
    uint16_t getRawY(void);
    uint16_t getCurrentX(void);
    uint16_t getCurrentY(void);

    uint16_t readChannel(uint8_t channel, bool use12Bit, bool useDiffMode, uint8_t numberOfReadingsToIntegrate);

#if defined(AVR)
    bool writeCalibration(uint16_t eeprom_addr);
    bool readCalibration(uint16_t eeprom_addr);
#else
    void writeCalibration(CAL_MATRIX aMatrix);
    void readCalibration(CAL_MATRIX *aMatrix);
#endif

private:
    struct XYPosition mTouchActualPositionRaw; // raw pos (touch panel)
    struct XYPosition mTouchLastCalibratedPositionRaw; // last calibrated raw pos - to avoid calibrating the same position twice

    CAL_MATRIX tp_matrix; //calibrate matrix
    bool setCalibration(CAL_POINT *lcd, CAL_POINT *tp);
    void calibrate(void);

#if defined(AVR)
    uint8_t rd_spi(void);
    void wr_spi(uint8_t data);
#endif
};
extern ADS7846 TouchPanel; // The instance provided by the class itself

#endif //_ADS7846_H
