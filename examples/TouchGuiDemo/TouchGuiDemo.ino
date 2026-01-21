/*
 *  TouchGuiDemo.cpp
 *
 *  Demo of the GUI: LocalTouchButton, LocalTouchSlider and Chart
 *  and the programs Game of life, Draw Lines
 *  and if local display is attached, show font and ADS7846 A/D channels.
 *
 *  Tested on: Arduino Uno with mSD-Shield and MI0283QT Adapter from www.watterott.net
 *
 *  Copyright (C) 2012-2024  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay https://github.com/ArminJo/Arduino-BlueDisplay.
 *  This file is part of STMF3-Discovery-Demos https://github.com/ArminJo/STMF3-Discovery-Demos.
 *
 *  STMF3-Discovery-Demos + BlueDisplay are free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 */

/*
 * Main switches for program
 */
//#define FAST_CLOCK // enable development tests, requires AVRUtils from https://github.com/ArminJo/Arduino-Utils
//#define DEBUG // enable debug button and debug output
//#define RTC_EXISTS  //if a DS1307 is connected to the I2C bus
#include <Arduino.h>

#if defined(TEST)
#include "AVRUtils.h"       // For initStackFreeMeasurement(), printRAMInfo()
#endif

/*
 * Settings to configure the BlueDisplay library and to reduce its size
 */
//#define BLUETOOTH_BAUD_RATE BAUD_115200   // Activate this, if you have reprogrammed the HC05 module for 115200, otherwise 9600 is used as baud rate
//#define DO_NOT_NEED_BASIC_TOUCH_EVENTS    // Disables basic touch events down, move and up. Saves 620 bytes program memory and 36 bytes RAM
//#define DO_NOT_NEED_TOUCH_AND_SWIPE_EVENTS  // Disables LongTouchDown and SwipeEnd events.
//#define ONLY_CONNECT_EVENT_REQUIRED         // Disables reorientation, redraw and SensorChange events
//#define BD_USE_SIMPLE_SERIAL                // Do not use the Serial object. Saves up to 1250 bytes program memory and 185 bytes RAM, if Serial is not used otherwise
//#define BD_USE_USB_SERIAL                   // Activate it, if you want to force using Serial instead of Serial1 for direct USB cable connection* to your smartphone / tablet.
/*
 * Enable this lines to run the demo locally
 */
//#define RUN_ON_LOCAL_HX8347_DISPLAY_ONLY
#if defined(RUN_ON_LOCAL_HX8347_DISPLAY_ONLY)
#define LOCAL_GUI_FEEDBACK_TONE_PIN 2
#define DISABLE_REMOTE_DISPLAY  // Suppress drawing to Bluetooth connected display. Allow only drawing on the locally attached display
#define SUPPORT_LOCAL_DISPLAY   // Supports simultaneously drawing on the locally attached display. Not (yet) implemented for all commands!
#define SUPPORT_LOCAL_LONG_TOUCH_DOWN_DETECTION
#define FONT_8X12               // Font size used here
#include "LocalHX8347DDisplay.hpp" // The implementation of the local display must be included first since it defines LOCAL_DISPLAY_HEIGHT etc.
#define DISPLAY_HEIGHT LOCAL_DISPLAY_HEIGHT // Use local size for whole application
#define DISPLAY_WIDTH  LOCAL_DISPLAY_WIDTH
#include "GUIHelper.hpp"        // Must be included before LocalGUI. For TEXT_SIZE_11, getLocalTextSize() etc.
#include "LocalGUI.hpp"         // Includes the sources for LocalTouchButton etc.
#else
#include "BlueDisplay.hpp"         // Includes the sources for LocalTouchButton etc.
#define DISPLAY_HEIGHT 240 // Use same size as local display
#define DISPLAY_WIDTH  320
void initDisplay(void);
#endif

char sStringBuffer[32]; // Used in GameOfLife and GuiDemo, and provided by different main programs
uint32_t sMillisOfLastLoop;

#if !defined(Button)
#  if defined(SUPPORT_LOCAL_DISPLAY) && defined(DISABLE_REMOTE_DISPLAY)
/*
 * For programs, that must save memory when running on local display only
 * Only local display must be supported, so LocalTouchButton, etc. is sufficient
 */
#define Button              LocalTouchButton
#define AutorepeatButton    LocalTouchButtonAutorepeat
#define Slider              LocalTouchSlider
#define Display             LocalDisplay
#  else
// Remote display is used here, so use BD elements, they are aware of the existence of Local* objects and use them if SUPPORT_LOCAL_DISPLAY is enabled
#define Button              BDButton
#define AutorepeatButton    BDButton
#define Slider              BDSlider
#define Display             BlueDisplay1
#  endif
#endif

Slider TouchSliderBacklight;
Button TouchButtonTPCalibration;
Button TouchButtonBack;

#define TP_EEPROMADDR (E2END -1 - sizeof(CAL_MATRIX)) //eeprom address for calibration data - 28 bytes

#if defined(SUPPORT_LOCAL_DISPLAY)
#include "LocalDisplayGUI.hpp"
#endif

#include "Chart.hpp"
#if defined(SUPPORT_LOCAL_DISPLAY) && !defined(LOCAL_DISPLAY_GENERATES_BD_EVENTS)
void printLocalTouchPanelData(void); // required in PageDraw.hpp and GuiDemo.hpp
#endif
#include "PageDraw.hpp"
#include "GuiDemo.hpp" // The main page / menu implementation for the demo

#ifdef RTC_EXISTS
#include <i2cmaster.h>
#define DS1307_ADDR 0xD0 // 0x68 shifted left
uint8_t bcd2bin(uint8_t val);
uint8_t bin2bcd(uint8_t val);
void showRTCTime(void);
void setRTCTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t dayOfWeek, uint8_t day, uint8_t month, uint16_t year);
#endif

#if defined(DEBUG)
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
#if defined(TEST)
    initStackFreeMeasurement();
#endif

#if defined(DEBUG)
    Serial.begin(115200);
    // Just to know which program is running on my Arduino
    delay(2000);
    Serial.println(F("START " __FILE__ " from " __DATE__));
#endif

#if defined(SUPPORT_LOCAL_DISPLAY)
    /*
     * Initialize local display, fast SPI mode
     */
    LocalDisplay.init(2); //spi-clk = Fcpu/2
    //init touch controller
    TouchPanel.initAndCalibrateOnPress(TP_EEPROMADDR);
#endif

#if !defined(DISABLE_REMOTE_DISPLAY)
    /*
     * Initialize serial connection for BlueDisplay
     */
#  if defined(ESP32)
    Serial.begin(115200);
    Serial.println("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY);
    initSerial("ESP-BD_Example");
    Serial.println("Start ESP32 BT-client with name \"ESP-BD_Example\"");
#  else
    initSerial();
#  endif
    /*
     * Register callback handler and wait for 300 ms if Bluetooth connection is still active.
     * For ESP32 and after power on of the Bluetooth module (HC-05) at other platforms, Bluetooth connection is most likely not active here.
     *
     * If active, mCurrentDisplaySize and mHostUnixTimestamp are set and initDisplay() and drawGui() functions are called.
     * If not active, the periodic call of checkAndHandleEvents() in the main loop waits for the (re)connection and then performs the same actions.
     */
    BlueDisplay1.initCommunication(&Serial, &initDisplay); // redraw is registered by startGuiDemo() called by initDisplay()
#endif

#if defined(SUPPORT_LOCAL_DISPLAY)
    // show menu locally at startup
    startGuiDemo();
#endif
    Button::playFeedbackTone();

#if defined(TEST)
    printRAMInfo(&Serial); // Stack used is 126 bytes
#endif
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

#if !defined(DISABLE_REMOTE_DISPLAY)
void initDisplay(void) {
    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE, DISPLAY_WIDTH, DISPLAY_HEIGHT);
#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalTouchButton::createAllLocalButtonsAtRemote(); // Local buttons already exists, so recreate the remote ones
#else
    startGuiDemo(); // No local buttons created, start initializing GUI elements
#endif
}
#endif

#if defined(SUPPORT_LOCAL_DISPLAY)
//show touchpanel data
void printLocalTouchPanelData(void) {
    snprintf(sStringBuffer, sizeof(sStringBuffer), "X:%03i|%04i Y:%03i|%04i P:%03i %u", TouchPanel.getCurrentX(), TouchPanel.getRawX(),
            TouchPanel.getCurrentY(), TouchPanel.getRawY(), TouchPanel.getPressure(), sTouchObjectTouched);
    LocalDisplay.drawText(30, 2, sStringBuffer, TEXT_SIZE_11, COLOR16_BLACK, BACKGROUND_COLOR);
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
    i2c_start(DS1307_ADDR + I2C_WRITE);// set device address and write mode
    i2c_write(0x00);
    i2c_stop();

// read 6 bytes from start address
    i2c_start(DS1307_ADDR + I2C_READ);// set device address and read mode
    for (uint8_t i = 0; i < 6; ++i) {
        RtcBuf[i] = bcd2bin(i2c_readAck());
    }
// read year and stop
    RtcBuf[6] = bcd2bin(i2c_readNak());
    i2c_stop();

//buf[3] is day of week
    snprintf_P(sStringBuffer, sizeof(sStringBuffer), PSTR("%02i.%02i.%04i %02i:%02i:%02i"), RtcBuf[4], RtcBuf[5], RtcBuf[6] + 2000, RtcBuf[2], RtcBuf[1],
            RtcBuf[0]);
    LocalDisplay.drawText(10, DISPLAY_HEIGHT - FONT_HEIGHT - 1, StringBuffer, 1, COLOR_RED, BACKGROUND_COLOR);
}

void setRTCTime(uint8_t sec, uint8_t min, uint8_t hour, uint8_t dayOfWeek, uint8_t day, uint8_t month, uint16_t year) {
//Write Start Adress
    i2c_start(DS1307_ADDR + I2C_WRITE);// set device address and write mode
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

