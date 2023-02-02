/*
 * LocalTouchGuiDemo.cpp
 *
 *      Demo of the libs:
 *      TouchButton
 *      TouchSlider
 *      Chart
 *
 *      and the "Apps" ;-)
 *      Draw lines
 *      Game of life
 *      Show ADS7846 A/D channels
 *      Display font
 *
 *      For Arduino Uno
 *      with mSD-Shield and MI0283QT Adapter from www.watterott.net
 *      and the ADS7846 and MI0283QT2 libs from
 *		https://github.com/watterott/mSD-Shield/downloads
 *
 *		MI0283QT2/fonts.h must be a version with disabled "#define FONT_END7F" to show all font characters
 *		ADS7846/ADS7846.cpp must be the modified version in order to show the ADS7846 channels
 *
 * Demo of the GUI: TouchButton, TouchSlider and Chart
 * and the programs Game of life and show ADS7846 A/D channels
 *
 *  Copyright (C) 2012-2023  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of STMF3-Discovery-Demos https://github.com/ArminJo/STMF3-Discovery-Demos.
 *
 *  STMF3-Discovery-Demos is free software: you can redistribute it and/or modify
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
 */

/*
 * Main switches for program
 */
//#define DEBUG // enable debug button and debug output
//#define RTC_EXISTS  //if a DS1307 is connected to the I2C bus
#include <Arduino.h>

#define LOCAL_GUI_FEEDBACK_TONE_PIN 2
#define DISABLE_REMOTE_DISPLAY  // Suppress drawing to Bluetooth connected display. Allow only drawing on the locally attached display
#define SUPPORT_LOCAL_DISPLAY   // Supports simultaneously drawing on the locally attached display. Not (yet) implemented for all commands!

#include "LocalHX8347DDisplay.hpp" // The implementation of the local display must be included first since it defines LOCAL_DISPLAY_HEIGHT etc.
//#include "GUIHelper.hpp"        // Must be included before LocalGUI. For TEXT_SIZE_11, getLocalTextSize() etc.
#include "LocalGUI.hpp"         // Includes the sources for LocalTouchButton etc.

char sStringBuffer[32];
uint32_t sMillisOfLastLoop;
void printTPData(void);

TouchSlider TouchSliderBacklight;
TouchButton TouchButtonTPCalibration;
TouchButton TouchButtonBack;

#define TP_EEPROMADDR (E2END -1 - sizeof(CAL_MATRIX)) //eeprom address for calibration data - 28 bytes

#include "LocalDisplayGUI.hpp"
#include "Chart.hpp"
#include "PageDraw.hpp"
#include "GuiDemo.hpp"

#ifdef RTC_EXISTS
#include <i2cmaster.h>
#define DS1307_ADDR 0xD0 // 0x68 shifted left
uint8_t bcd2bin(uint8_t val);
uint8_t bin2bcd(uint8_t val);
void showRTCTime(void);
void setRTCTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t dayOfWeek, uint8_t day, uint8_t month, uint16_t year);
#endif

#ifdef DEBUG
TouchButton TouchButtonDebug;
void doDebug(TouchButton * const aTheTouchedButton, int aValue);
#endif

/*
 * Loop control
 */
unsigned long LoopMillis = 0;
/*
 * RTC Stuff
 */
#ifdef RTC_EXISTS
#define DS1307_ADDR 0xD0 // 0x68 shifted left
uint8_t bcd2bin(uint8_t val);
uint8_t bin2bcd(uint8_t val);
void showRTCTime(void);
void setRTCTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t dayOfWeek, uint8_t day, uint8_t month, uint16_t year);
#endif

void setup() {

#if defined(DEBUG)
    Serial.begin(115200);
    // Just to know which program is running on my Arduino
    delay(2000);
    Serial.println(F("START " __FILE__ " from " __DATE__));
#endif

    //init display
    // faster SPI mode - works fine :-)
    LocalDisplay.init(2); //spi-clk = Fcpu/2

    //init touch controller
    TouchPanel.initAndCalibrateOnPress(TP_EEPROMADDR);

    //show menu
    startGuiDemo();
    playLocalFeedbackTone();
}

void loop() {

    loopGuiDemo();

#ifdef RTC_EXISTS
    if (mActualApplication == APPLICATION_MENU) {
        if (LoopMillis > 200) {
            LoopMillis = 0;
            //RTC
            showRTCTime();
        }
    }
#endif
}

//show touchpanel data
void printTPData(void) {
    sprintf(sStringBuffer, "X:%03i|%04i Y:%03i|%04i P:%03i", TouchPanel.getActualX(), TouchPanel.getRawX(), TouchPanel.getActualY(),
            TouchPanel.getRawY(), TouchPanel.getPressure());
    LocalDisplay.drawText(30, 2, sStringBuffer, TEXT_SIZE_11, COLOR16_BLACK, BACKGROUND_COLOR);
}

#ifdef DEBUG
void doDebug(TouchButton * const aTheTouchedButton, int aValue) {
    /*
     * Debug button pressed
     */
    uint8_t * stackptr = (uint8_t *) malloc(4); // use stackptr temporarily
    uint8_t * heapptr = stackptr;// save value of heap pointer
    free(stackptr);
    stackptr = (uint8_t *) (SP);
    sprintf(StringBuffer, "TB=%02u TS=%02u CH=%02u Stack=%04u Heap=%04u", sizeof *aTheTouchedButton,
            sizeof TouchSliderBacklight, sizeof ChartExample, (unsigned int) stackptr, (unsigned int) heapptr);
    LocalDisplay.drawText(1, DISPLAY_HEIGHT - (3 * FONT_HEIGHT) - 1, StringBuffer, 1, COLOR_BLACK, BACKGROUND_COLOR);
}
#endif

#ifdef RTC_EXISTS

uint8_t bcd2bin(uint8_t val) {
    return val - 6 * (val >> 4);
}
uint8_t bin2bcd(uint8_t val) {
    return val + 6 * (val / 10);
}

void showRTCTime(void) {
    uint8_t RtcBuf[7];

    /*
     * get time from RTC
     */
// write start address
    i2c_start(DS1307_ADDR + I2C_WRITE); // set device address and write mode
    i2c_write(0x00);
    i2c_stop();

// read 6 bytes from start address
    i2c_start(DS1307_ADDR + I2C_READ); // set device address and read mode
    for (uint8_t i = 0; i < 6; ++i) {
        RtcBuf[i] = bcd2bin(i2c_readAck());
    }
// read year and stop
    RtcBuf[6] = bcd2bin(i2c_readNak());
    i2c_stop();

//buf[3] is day of week
    sprintf_P(StringBuffer, PSTR("%02i.%02i.%04i %02i:%02i:%02i"), RtcBuf[4], RtcBuf[5], RtcBuf[6] + 2000, RtcBuf[2], RtcBuf[1],
            RtcBuf[0]);
    LocalDisplay.drawText(10, DISPLAY_HEIGHT - FONT_HEIGHT - 1, StringBuffer, 1, COLOR_RED, BACKGROUND_COLOR);
}

void setRTCTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t dayOfWeek, uint8_t day, uint8_t month, uint16_t year) {
//Write Start Adress
    i2c_start(DS1307_ADDR + I2C_WRITE); // set device address and write mode
    i2c_write(0x00);
// write data
    i2c_write(bin2bcd(sec));
    i2c_write(bin2bcd(min));
    i2c_write(bin2bcd(hour));
    i2c_write(bin2bcd(dayOfWeek));
    i2c_write(bin2bcd(day));
    i2c_write(bin2bcd(month));
    i2c_write(bin2bcd(year - 2000));
    i2c_stop();
}

#endif

