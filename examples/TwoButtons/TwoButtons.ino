/*
 * TwoButtons.ino
 *
 *  Created on: 31.01.2012
 *      Author: Armin Joachimsmeyer
 *      Email:   armin.joachimsmeyer@gmx.de
 *      License: GPL v3 (http://www.gnu.org/licenses/gpl.html)
 *      Version: 1.0.0
 *
 *      Demo of the TouchButton lib
 *
 *      For Arduino Uno
 *      with mSD-Shield and MI0283QT Adapter from www.watterott.net
 *      and the ADS7846 and MI0283QT2 libs from
 *      https://github.com/watterott/mSD-Shield/downloads
 *
 */

#include <Arduino.h>

/*
 * Settings to configure the LocalTouchGUI library
 */
#define LOCAL_GUI_FEEDBACK_TONE_PIN 2
#define SUPPORT_ONLY_TEXT_SIZE_11_AND_22  // Saves 248 bytes program memory
#define DISABLE_REMOTE_DISPLAY   // Suppress drawing to Bluetooth connected display. Allow only drawing on the locally attached display
#define SUPPORT_LOCAL_DISPLAY      // Supports simultaneously drawing on the locally attached display. Not (yet) implemented for all commands!
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
LocalTouchButtonAutorepeat TouchButtonCaptionAutorepeat;
LocalTouchButton TouchButtonBackground;

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
    int8_t tErrorValue = TouchButtonCaptionAutorepeat.init(20, 20, BUTTON_WIDTH, BUTTON_HEIGHT, COLOR16_RED, "Caption",
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doButtons);
    TouchButtonCaptionAutorepeat.setButtonAutorepeatTiming(1000, 400, 5, 100);

    tErrorValue += TouchButtonBackground.init(20, TouchButtonCaptionAutorepeat.getPositionYBottom() + BUTTON_SPACING,
    BUTTON_WIDTH, BUTTON_HEIGHT, COLOR16_RED, "Background", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doButtons);

    TouchButtonCaptionAutorepeat.drawButton();
    TouchButtonBackground.drawButton();

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
    if (aTheTouchedButton == &TouchButtonCaptionAutorepeat) {
        aValue += CAPTION_COLOR_INCREMENT;
        aTheTouchedButton->setCaptionColor(aValue);
        aTheTouchedButton->setValue(aValue);
        aTheTouchedButton->drawCaption();
        return;
    }
    if (aTheTouchedButton == &TouchButtonBackground) {
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
    sprintf(StringBuffer, "X:%03i|%04i Y:%03i|%04i P:%03i", TouchPanel.getCurrentX(), TouchPanel.getRawX(), TouchPanel.getCurrentY(),
            TouchPanel.getRawY(), TouchPanel.getPressure());
    LocalDisplay.drawText(20, 2, StringBuffer, TEXT_SIZE_11, COLOR16_BLACK, BACKGROUND_COLOR);
}
