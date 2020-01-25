/*
 *  BlueDisplayExample.cpp
 *  Demo of using the BlueDisplay library for HC-05 on Arduino
 *
 *  For handling time, the Arduino "time" library is used. You can install it also with "Tools -> Manage Libraries..." or "Ctrl+Shift+I" -> use "timekeeping" as filter string.
 *  The sources can also be found here: https://github.com/PaulStoffregen/Time
 *

 *  Copyright (C) 2014  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay.
 *  BlueDisplay is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#include <Arduino.h>

#include "BlueDisplay.h"
#include "TimeLib.h"

#define VERSION_EXAMPLE "1.1"

/****************************************************************************
 * Change this if you have reprogrammed the hc05 module for other baud rate
 ***************************************************************************/
#ifndef BLUETOOTH_BAUD_RATE
//#define BLUETOOTH_BAUD_RATE BAUD_115200
#define BLUETOOTH_BAUD_RATE BAUD_9600
#endif

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

#define DELAY_START_VALUE 600
#define DELAY_CHANGE_VALUE 10

#define SLIDER_X_POSITION 80

#define COLOR_DEMO_BACKGROUND COLOR_BLUE
#define COLOR_CAPTION COLOR_RED

#if defined(__STM32F1__)
const int TONE_PIN = 2;
const int ANALOG_INPUT_PIN = PA0;
#else
const int TONE_PIN = 2;
const int ANALOG_INPUT_PIN = A0;
#endif

bool sConnected = false;
bool doBlink = true;
volatile int sDelay = 600;

// a string buffer for any purpose...
char sStringBuffer[128];

/*
 * The buttons
 */
BDButton TouchButtonBDExampleBlinkStartStop;
BDButton TouchButtonPlus;
BDButton TouchButtonMinus;
BDButton TouchButtonValueDirect;

// Touch handler for buttons
void doBDExampleBlinkStartStop(BDButton * aTheTochedButton, int16_t aValue);
void doPlusMinus(BDButton * aTheTochedButton, int16_t aValue);
void doGetDelay(BDButton * aTheTouchedButton, int16_t aValue);

/*
 * The horizontal slider
 */
BDSlider TouchSliderDelay;
// handler for value change
void doDelay(BDSlider * aTheTochedSlider, uint16_t aSliderValue);

/*
 * Time functions
 */
void printTime();
void infoEventCallback(uint8_t aSubcommand, uint8_t aByteInfo, uint16_t aShortInfo, ByteShortLongFloatUnion aLongInfo);
time_t requestTimeSync();

void printDemoString(void);

// Callback handler for (re)connect and reorientation
void initDisplay(void);
// Callback handler for redraw
void drawGui(void);

void setup() {
	// initialize the digital pin as an output.
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(TONE_PIN, OUTPUT);
	pinMode(ANALOG_INPUT_PIN, INPUT);

    /*
     * If you want to see Serial.print output if not connected with BlueDisplay comment out the line "#define USE_STANDARD_SERIAL" in BlueSerial.h
     * or define global symbol with -DUSE_STANDARD_SERIAL in order to force the BlueDisplay library to use the Arduino Serial object
     * and to release the SimpleSerial interrupt handler '__vector_18'
     */
    initSerial(BLUETOOTH_BAUD_RATE);
#if defined (USE_SERIAL1) // defined in BlueSerial.h
    // Can use Serial(0) for Serial.print  output.
    delay(2000); // To be able to connect Serial monitor after reset and before first printout
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ "\r\nVersion " VERSION_EXAMPLE " from " __DATE__));
#endif

	// Register callback handler and check for connection
	BlueDisplay1.initCommunication(&initDisplay, &drawGui);

	//set function to call when time sync required (default: after 300 seconds)
	setSyncProvider(requestTimeSync);

	// to signal that boot has finished
	tone(TONE_PIN, 2000, 200);
}

void loop() {
	static unsigned long sLastMilisOfTimePrinted;

	/*
	 * This debug output can also be recognized at the Arduino Serial Monitor
	 */
//    BlueDisplay1.debug("\r\nDoBlink=", (uint8_t) doBlink);
	if (!BlueDisplay1.isConnectionEstablished()) {
		int tBlinkDuration = analogRead(ANALOG_INPUT_PIN);

		// This serial output is readable at the Arduino serial monitor
		BlueDisplay1.debug("\r\nAnalogIn=", tBlinkDuration);

		digitalWrite(LED_BUILTIN, HIGH);
		delay(tBlinkDuration / 2);
		digitalWrite(LED_BUILTIN, LOW);
		delay(tBlinkDuration / 2);
	} else {
		if (doBlink) {

			uint8_t i;
			/*
			 * LED on
			 */
			digitalWrite(LED_BUILTIN, HIGH);
			BlueDisplay1.fillCircle(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, 20, COLOR_RED);
			/*
			 *  Wait for delay time and update "Demo" string at a rate 16 times the blink rate.
			 *  For Arduino serial check touch events 8 times while waiting.
			 */
			for (i = 0; i < 8; ++i) {
				delayMillisWithCheckAndHandleEvents(sDelay / 8);
				printDemoString();
			}
			/*
			 * LED off
			 */
			digitalWrite(LED_BUILTIN, LOW);
			BlueDisplay1.fillCircle(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, 20, COLOR_DEMO_BACKGROUND);
			for (i = 0; i < 8; ++i) {
				delayMillisWithCheckAndHandleEvents(sDelay / 8);
				printDemoString();
			}
			printDemoString();
		}
		/*
		 * print time every second
		 */
		unsigned long tMillis = millis();
		if (tMillis - sLastMilisOfTimePrinted > 1000) {
			sLastMilisOfTimePrinted = tMillis;
			printTime();
		}
	}

	checkAndHandleEvents();
}

void initDisplay(void) {
	BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE | BD_FLAG_TOUCH_BASIC_DISABLE, DISPLAY_WIDTH,
	DISPLAY_HEIGHT);

	TouchButtonPlus.init(270, 80, 40, 40, COLOR_YELLOW, "+", 33, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT,
	DELAY_CHANGE_VALUE, &doPlusMinus);
	TouchButtonPlus.setButtonAutorepeatTiming(600, 100, 10, 30);

	TouchButtonMinus.init(10, 80, 40, 40, COLOR_YELLOW, "-", 33, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT,
			-DELAY_CHANGE_VALUE, &doPlusMinus);
	TouchButtonMinus.setButtonAutorepeatTiming(600, 100, 10, 30);

	TouchButtonBDExampleBlinkStartStop.init(30, 150, 140, 55, COLOR_DEMO_BACKGROUND, "Start", 44,
			FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, doBlink, &doBDExampleBlinkStartStop);
	TouchButtonBDExampleBlinkStartStop.setCaptionForValueTrue("Stop");

	// F("...") and init saves RAM since string "..." is stored in flash
	TouchButtonValueDirect.init(210, 150, 90, 55, COLOR_YELLOW, F("..."), 44, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doGetDelay);

	TouchSliderDelay.init(SLIDER_X_POSITION, 40, 12, 150, 100, DELAY_START_VALUE / 10, COLOR_YELLOW, COLOR_GREEN,
			FLAG_SLIDER_SHOW_BORDER | FLAG_SLIDER_SHOW_VALUE | FLAG_SLIDER_IS_HORIZONTAL, &doDelay);
	TouchSliderDelay.setCaptionProperties(TEXT_SIZE_22, FLAG_SLIDER_CAPTION_ALIGN_RIGHT, 4, COLOR_RED,
	COLOR_DEMO_BACKGROUND);
	TouchSliderDelay.setCaption("Delay");
	TouchSliderDelay.setScaleFactor(10); // Slider is virtually 10 times larger
	TouchSliderDelay.setValueUnitString("ms");

	TouchSliderDelay.setPrintValueProperties(TEXT_SIZE_22, FLAG_SLIDER_CAPTION_ALIGN_LEFT, 4, COLOR_WHITE,
	COLOR_DEMO_BACKGROUND);

	// here we have received a new local timestamp
	setTime(BlueDisplay1.mHostUnixTimestamp);
}

void drawGui(void) {
	BlueDisplay1.clearDisplay(COLOR_DEMO_BACKGROUND);
	TouchButtonBDExampleBlinkStartStop.drawButton();
	TouchButtonPlus.drawButton();
	TouchButtonMinus.drawButton();
	TouchButtonValueDirect.drawButton();
	TouchSliderDelay.drawSlider();
}

#pragma GCC diagnostic ignored "-Wunused-parameter"
/*
 * Change doBlink flag
 */
void doBDExampleBlinkStartStop(BDButton * aTheTouchedButton, int16_t aValue) {
	doBlink = aValue;
}

/*
 * Is called every 5 minutes by default
 */
time_t requestTimeSync() {
	BlueDisplay1.getInfo(SUBFUNCTION_GET_INFO_LOCAL_TIME, &infoEventCallback);
	return 0; // the time will be sent later in response to getInfo
}

void infoEventCallback(uint8_t aSubcommand, uint8_t aByteInfo, uint16_t aShortInfo, ByteShortLongFloatUnion aLongInfo) {
	if (aSubcommand == SUBFUNCTION_GET_INFO_LOCAL_TIME) {
		setTime(aLongInfo.uint32Value);
		// to prove that it is called every 5 minutes by default
		// tone(TONE_PIN, 1000, 200);
	}
}

/*
 * Is called by touch of both plus and minus buttons.
 * We just take the passed value and do not care which button was touched
 */
void doPlusMinus(BDButton * aTheTouchedButton, int16_t aValue) {
	sDelay += aValue;
	if (sDelay < DELAY_CHANGE_VALUE) {
		sDelay = DELAY_CHANGE_VALUE;
	}
	if (!doBlink) {
		// enable blinking
		doBlink = true;
		TouchButtonBDExampleBlinkStartStop.setValueAndDraw(doBlink);
	}
	// set slider bar accordingly
	TouchSliderDelay.setActualValueAndDrawBar(sDelay);
	/*
	 * Example for debug/toast output by BlueDisplay
	 */
	BlueDisplay1.debug("Delay=", sDelay);
}

/*
 * Handler for number receive event - set delay to value
 */
void doSetDelay(float aValue) {
// clip value
	if (aValue > 100000) {
		aValue = 100000;
	} else if (aValue < 1) {
		aValue = 1;
	}
	sDelay = aValue;
	// set slider bar accordingly
	TouchSliderDelay.setActualValueAndDrawBar(sDelay);
}

/*
 * Request delay value as number
 */
void doGetDelay(BDButton * aTheTouchedButton, int16_t aValue) {
	BlueDisplay1.getNumberWithShortPrompt(&doSetDelay, "delay [ms]");
}

/*
 * Is called by touch or move on slider and sets the new delay according to the passed slider value
 */
void doDelay(BDSlider * aTheTouchedSlider, uint16_t aSliderValue) {
	sDelay = aSliderValue;
}

void printTime() {
	// 1600 byte code size for time handling plus print
	sprintf_P(sStringBuffer, PSTR("%02d.%02d.%4d %02d:%02d:%02d"), day(), month(), year(), hour(), minute(), second());
	BlueDisplay1.drawText(DISPLAY_WIDTH - 20 * TEXT_SIZE_11_WIDTH, DISPLAY_HEIGHT - TEXT_SIZE_11_HEIGHT, sStringBuffer, 11,
	COLOR_BLACK, COLOR_DEMO_BACKGROUND);
}

#define MILLIS_PER_CHANGE 20 // gives minimal 2 seconds
void printDemoString(void) {
	static float tFadingFactor = 0.0; // 0 -> Background 1 -> Caption
	static float tInterpolationDelta = 0.01;

	static bool tFadingFactorDirectionFromBackground = true;
	static unsigned long MillisOfNextChange;

	// Timing
	unsigned long tMillis = millis();
	if (tMillis >= MillisOfNextChange) {
		MillisOfNextChange = tMillis + MILLIS_PER_CHANGE;

		// slow fade near background color
		if (tFadingFactor <= 0.1) {
			tInterpolationDelta = 0.002;
		} else {
			tInterpolationDelta = 0.01;
		}

		// manage fading factor
		if (tFadingFactorDirectionFromBackground) {
			tFadingFactor += tInterpolationDelta;
			if (tFadingFactor >= (1.0 - 0.01)) {
				// toggle direction to background
				tFadingFactorDirectionFromBackground = !tFadingFactorDirectionFromBackground;
			}
		} else {
			tFadingFactor -= tInterpolationDelta;
			if (tFadingFactor <= tInterpolationDelta) {
				// toggle direction
				tFadingFactorDirectionFromBackground = !tFadingFactorDirectionFromBackground;
			}
		}

		// get resulting color
		uint8_t ColorRed = GET_RED(COLOR_DEMO_BACKGROUND)
				+ ((int16_t) ( GET_RED(COLOR_CAPTION) - GET_RED(COLOR_DEMO_BACKGROUND)) * tFadingFactor);
		uint8_t ColorGreen = GET_GREEN(COLOR_DEMO_BACKGROUND)
				+ ((int16_t) (GET_GREEN(COLOR_CAPTION) - GET_GREEN(COLOR_DEMO_BACKGROUND)) * tFadingFactor);
		uint8_t ColorBlue = GET_BLUE(COLOR_DEMO_BACKGROUND)
				+ ((int16_t) ( GET_BLUE(COLOR_CAPTION) - GET_BLUE(COLOR_DEMO_BACKGROUND)) * tFadingFactor);
		BlueDisplay1.drawText(DISPLAY_WIDTH / 2 - 2 * getTextWidth(36), 4 + getTextAscend(36), "Demo", 36,
				RGB(ColorRed, ColorGreen, ColorBlue), COLOR_DEMO_BACKGROUND);
	}
}
