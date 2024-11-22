/*
 * TwoButtonsOnLocalDisplay.cpp
 *
 *  Demo of creating two buttons on a local display
 *      using the LocalTouchButton lib for Arduino Uno
 *      with mSD-Shield and MI0283QT Adapter from www.watterott.net
 *      and the ADS7846 and MI0283QT2 libs from
 *      https://github.com/watterott/mSD-Shield/downloads
 *
 *  Copyright (C) 2012-2024  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay https://github.com/ArminJo/Arduino-BlueDisplay.
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
#include <Arduino.h>

/*
 * Settings to configure the LocalGUI library
 */
#define LOCAL_GUI_FEEDBACK_TONE_PIN 2
#define SUPPORT_ONLY_TEXT_SIZE_11_AND_22  // Saves 248 bytes program memory
#define DISABLE_REMOTE_DISPLAY  // Suppress drawing to Bluetooth connected display. Allow only drawing on the locally attached display
#define SUPPORT_LOCAL_DISPLAY   // Supports simultaneously drawing on the locally attached display. Not (yet) implemented for all commands!
#define FONT_8X12               // Font size used here
#include "LocalHX8347DDisplay.hpp" // The implementation of the local display must be included first since it defines LOCAL_DISPLAY_HEIGHT etc.
#include "LocalGUI.hpp"            // Includes the sources for LocalTouchButton etc.

/*
 * LCD and touch panel stuff
 */
#define TP_EEPROMADDR (E2END -1 - sizeof(CAL_MATRIX)) //eeprom address for calibration data - 28 bytes
void printTPData(void);
void printRGB(const uint16_t aColor, const uint16_t aXPos, const uint16_t aYPos);

#define BACKGROUND_COLOR COLOR16_WHITE

// a string buffer for any purpose...
char StringBuffer[128];

#define BUTTON_WIDTH 180
#define BUTTON_HEIGHT 50
#define BUTTON_SPACING 30

// Increment to create a lot of different colors
#define CAPTION_COLOR_INCREMENT             COLOR16(0x08,0x20,0x60)
#define BUTTON_BACKGROUND_COLOR_INCREMENT   COLOR16(0x08,0x10,0x40)
LocalTouchButtonAutorepeat TouchButtonTextColorAutorepeat;
LocalTouchButton TouchButtonBackgroundColor;

// Callback touch handler
void doButtons(LocalTouchButton *const aTheTouchedButton, int16_t aValue);

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
//  LocalDisplay.init(4); //spi-clk = Fcpu/4

    LocalDisplay.clearDisplay(COLOR16_WHITE);

    //init touch controller
    TouchPanel.initAndCalibrateOnPress(TP_EEPROMADDR);

    // Create  2 buttons
    int8_t tErrorValue = TouchButtonTextColorAutorepeat.init(20, 20, BUTTON_WIDTH, BUTTON_HEIGHT, COLOR16_RED, "Text",
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doButtons);
    TouchButtonTextColorAutorepeat.setButtonAutorepeatTiming(1000, 400, 5, 100);

    tErrorValue += TouchButtonBackgroundColor.init(20, TouchButtonTextColorAutorepeat.getPositionYBottom() + BUTTON_SPACING,
    BUTTON_WIDTH, BUTTON_HEIGHT, COLOR16_RED, "Background", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doButtons);

    TouchButtonTextColorAutorepeat.drawButton();
    TouchButtonBackgroundColor.drawButton();

    if (tErrorValue != 0) {
        // Show Error
        LocalDisplay.clearDisplay(BACKGROUND_COLOR);
        LocalDisplay.drawText(40, 100, (char*) "Error on button rendering", TEXT_SIZE_22, COLOR16_RED, BACKGROUND_COLOR);
//        LocalDisplay.drawText(40, 120, tErrorValue, TEXT_SIZE_22, COLOR16_RED, BACKGROUND_COLOR);
        delay(5000);
    }
    LocalTouchButton::playFeedbackTone();
}

void loop() {
    checkAndHandleTouchPanelEvents();
    printTPData();
}

void doButtons(LocalTouchButton *const aTheTouchedButton, int16_t aValue) {
    printRGB(aValue, 10, 200);
    if (aTheTouchedButton == &TouchButtonTextColorAutorepeat) {
        aValue += CAPTION_COLOR_INCREMENT;
        aTheTouchedButton->setTextColor(aValue);
        aTheTouchedButton->setValue(aValue);
        aTheTouchedButton->drawText();
        return;
    }
    if (aTheTouchedButton == &TouchButtonBackgroundColor) {
        aValue += BUTTON_BACKGROUND_COLOR_INCREMENT;
        aTheTouchedButton->setButtonColor(aValue);
        aTheTouchedButton->setValue(aValue);
        aTheTouchedButton->drawButton();
        return;
    }
}

/*
 * Show RGB data of touched button
 */
void printRGB(const uint16_t aColor, uint16_t aXPos, const uint16_t aYPos) {
    //RED
    sprintf(StringBuffer, "R=%02X", (aColor & 0xF800) >> 8);
    aXPos = LocalDisplay.drawText(aXPos, aYPos, StringBuffer, TEXT_SIZE_22, COLOR16_RED, BACKGROUND_COLOR);
    //GREEN
    sprintf(StringBuffer, "G=%02X", (aColor & 0x07E0) >> 3);
    aXPos += 2 * FONT_WIDTH;
    aXPos = LocalDisplay.drawText(aXPos, aYPos, StringBuffer, TEXT_SIZE_22, COLOR16_GREEN, BACKGROUND_COLOR);
    //BLUE
    sprintf(StringBuffer, "B=%02X", (aColor & 0x001F) << 3);
    aXPos += 2 * FONT_WIDTH;
    LocalDisplay.drawText(aXPos, aYPos, StringBuffer, TEXT_SIZE_22, COLOR16_BLUE, BACKGROUND_COLOR);
}

/*
 * Show touch panel raw and processed data in the first line
 */
void printTPData(void) {
    sprintf(StringBuffer, "X:%03i|%04i Y:%03i|%04i P:%03i", TouchPanel.getCurrentX(), TouchPanel.getRawX(),
            TouchPanel.getCurrentY(), TouchPanel.getRawY(), TouchPanel.getPressure());
    LocalDisplay.drawText(20, 2, StringBuffer, TEXT_SIZE_11, COLOR16_BLACK, BACKGROUND_COLOR);
}
