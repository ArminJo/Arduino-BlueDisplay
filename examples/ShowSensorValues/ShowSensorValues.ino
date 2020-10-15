/*
 * ShowSensorValuesOnPlotter.cpp
 *
 *  Demo of using the sensor features of BlueDisplay library for HC-05 on Arduino
 *
 *  Copyright (C) 2014-2020  Armin Joachimsmeyer
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
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#include <Arduino.h>

#include "BlueDisplay.h"

//#define SHOW_SENSOR_VALUES_ON_PLOTTER

/****************************************************************************
 * Change this if you have reprogrammed the hc05 module for other baud rate
 ***************************************************************************/
#ifndef BLUETOOTH_BAUD_RATE
//#define BLUETOOTH_BAUD_RATE BAUD_115200
#define BLUETOOTH_BAUD_RATE BAUD_9600
#endif

#define DISPLAY_WIDTH  DISPLAY_HALF_VGA_WIDTH  // 320
#define DISPLAY_HEIGHT DISPLAY_HALF_VGA_HEIGHT // 215


#define SLIDER_BAR_LENGTH       100
#define SLIDER_BAR_WIDTH        12 // must be even
#define SLIDER_BAR_THRESHOLD    50

#define ACCELERATION_SLIDER_CENTER_X    (DISPLAY_WIDTH / 2)
#define ACCELERATION_SLIDER_LEFT_X      (ACCELERATION_SLIDER_CENTER_X - (SLIDER_BAR_WIDTH / 2))
#define ACCELERATION_SLIDER_RIGHT_X     (ACCELERATION_SLIDER_CENTER_X + (SLIDER_BAR_WIDTH / 2))
#define ACCELERATION_SLIDER_CENTER_Y    (DISPLAY_HEIGHT / 2)
#define ACCELERATION_SLIDER_TOP_Y       (ACCELERATION_SLIDER_CENTER_Y - (SLIDER_BAR_WIDTH / 2))

#define ACCELEROMETER_PRINT_VALUES_X    (ACCELERATION_SLIDER_CENTER_X - (12 * TEXT_SIZE_11_WIDTH))

// Pan = Yaw, Tilt = Pitch
#define ROLL_PITCH_YAW_SLIDER_BAR_WIDTH 4 // must be even

#define ROLL_PITCH_YAW_SLIDER_CENTER_X  (DISPLAY_WIDTH / 2)
#define ROLL_PITCH_YAW_SLIDER_LEFT_X    (ROLL_PITCH_YAW_SLIDER_CENTER_X - (ROLL_PITCH_YAW_SLIDER_BAR_WIDTH / 2))
#define ROLL_PITCH_YAW_SLIDER_RIGHT_X   (ROLL_PITCH_YAW_SLIDER_CENTER_X + (ROLL_PITCH_YAW_SLIDER_BAR_WIDTH / 2))

#define ROLL_PITCH_YAW_SLIDER_CENTER_Y  (DISPLAY_HEIGHT / 2)
#define ROLL_PITCH_YAW_SLIDER_TOP_Y     (ROLL_PITCH_YAW_SLIDER_CENTER_Y - (ROLL_PITCH_YAW_SLIDER_BAR_WIDTH / 2))

#define GYROSCOPE_PRINT_VALUES_X        (ROLL_PITCH_YAW_SLIDER_CENTER_X + TEXT_SIZE_11)

// 4 Sliders for accelerometer and 4 for gyroscope
BDSlider SliderAccelerationForward;
BDSlider SliderAccelerationBackward;
BDSlider SliderAccelerationRight;
BDSlider SliderAccelerationLeft;

BDSlider SliderTiltForward;
BDSlider SliderTiltBackward;
BDSlider SliderRollRight;
BDSlider SliderRollLeft;

// to avoid redraw, if the value did not change
int sAcceleratorForwardBackwardValue = 0;
int sLastAcceleratorLeftRightValue = 0;
// These 4 values are required by gyroscope callback
uint8_t sLastAcceleratorForwardSliderValue;
uint8_t sLastAcceleratorBackwardSliderValue;
uint8_t sLastAcceleratorRightSliderValue;
uint8_t sLastAcceleratorLeftSliderValue;

int sGyroscopeForwardBackwardValue = 0;
int sLastGyroscopeLeftRightValue = 0;

// Callback handler for (re)connect and resize
void initDisplay(void);
void drawGui(void);

// a string buffer for BD info output
char sStringBuffer[30];

/*******************************************************************************************
 * Program code starts here
 *******************************************************************************************/

void setup() {
    // Initialize the LED pin as output.
    pinMode(LED_BUILTIN, OUTPUT);

#if defined(ESP32)
    Serial.begin(115299);
    Serial.println("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY);
    initSerial("ESP-BD_Example");
    Serial.println("Start ESP32 BT-client with name \"ESP-BD_Example\"");
#else
    initSerial(BLUETOOTH_BAUD_RATE);
#endif

    // Register callback handler and check for connection
    BlueDisplay1.initCommunication(&initDisplay, &drawGui);

#if defined(USE_SERIAL1) // defined in BlueSerial.h
// Serial(0) is available for Serial.print output.
#  if defined(SERIAL_USB)
    delay(2000); // To be able to connect Serial monitor after reset and before first printout
#  endif

# ifdef SHOW_SENSOR_VALUES_ON_PLOTTER
    Serial.println(F("Accelerator_X Accelerator_Y Accelerator_Z")); // Caption for Plotter
#  else
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY));
    if (BlueDisplay1.isConnectionEstablished()) {
        BlueDisplay1.debug("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY);
    }
#  endif
#else

# ifdef SHOW_SENSOR_VALUES_ON_PLOTTER
    Serial.println(F("Accelerator_X Accelerator_Y Accelerator_Z")); // Caption for Plotter
#  else
    if (BlueDisplay1.isConnectionEstablished()) {
        BlueDisplay1.debug("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY);
    } else {
        Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY));
    }
# endif
#endif

}

void loop() {
    checkAndHandleEvents();
}

/*
 * Sensor callback handler
 * we receive values between -10 and 10
 */
void doAccelerometerChange(struct SensorCallback * aSensorCallbackInfo) {
    digitalWrite(LED_BUILTIN, HIGH);
#ifdef SHOW_SENSOR_VALUES_ON_PLOTTER
        /*
         * Print 2 values for Arduino Plotter
         */
        Serial.print(aSensorCallbackInfo->ValueX);
        Serial.print(' ');
        Serial.print(aSensorCallbackInfo->ValueY);
        Serial.print(' ');
        Serial.print(aSensorCallbackInfo->ValueZ);
        Serial.println();
#else
    /*
     * Show values on 4 sliders
     */
    // scale value for full scale = SLIDER_BAR_LENGTH at at 30 degree
    int tLeftRightValue = aSensorCallbackInfo->ValueX * ((SLIDER_BAR_LENGTH) / 10);
    if (sLastAcceleratorLeftRightValue != tLeftRightValue) {
        bool tSignChanged = (sLastAcceleratorLeftRightValue ^ tLeftRightValue) < 0;
        sLastAcceleratorLeftRightValue = tLeftRightValue;
        if (tLeftRightValue > 0) {
            sLastAcceleratorLeftSliderValue = tLeftRightValue;
            SliderAccelerationLeft.setValueAndDrawBar(tLeftRightValue);
            if (tSignChanged) {
                SliderAccelerationRight.setValueAndDrawBar(0);
            }
        } else {
            sLastAcceleratorRightSliderValue = -tLeftRightValue;
            SliderAccelerationRight.setValueAndDrawBar(-tLeftRightValue);
            if (tSignChanged) {
                SliderAccelerationLeft.setValueAndDrawBar(0);
            }
        }
    }

    int tForwardBackwardValue = aSensorCallbackInfo->ValueY * ((SLIDER_BAR_LENGTH) / 10);
    if (sAcceleratorForwardBackwardValue != tForwardBackwardValue) {
        bool tSignChanged = (sAcceleratorForwardBackwardValue ^ tForwardBackwardValue) < 0;
        sAcceleratorForwardBackwardValue = tForwardBackwardValue;
        if (tForwardBackwardValue > 0) {
            sLastAcceleratorBackwardSliderValue = tForwardBackwardValue;
            SliderAccelerationBackward.setValueAndDrawBar(tForwardBackwardValue);
            if (tSignChanged) {
                SliderAccelerationForward.setValueAndDrawBar(0);
            }
        } else {
            sLastAcceleratorForwardSliderValue = -tForwardBackwardValue;
            SliderAccelerationForward.setValueAndDrawBar(-tForwardBackwardValue);
            if (tSignChanged) {
                SliderAccelerationBackward.setValueAndDrawBar(0);
            }
        }
    }
    uint16_t tYPos = ACCELERATION_SLIDER_CENTER_Y + 2 * TEXT_SIZE_11_HEIGHT;
    dtostrf(aSensorCallbackInfo->ValueX, 5, 2, &sStringBuffer[15]);
    sprintf_P(sStringBuffer, PSTR("Acc.X %s"), &sStringBuffer[15]);
    BlueDisplay1.drawText(ACCELEROMETER_PRINT_VALUES_X, tYPos, sStringBuffer, TEXT_SIZE_11, COLOR_BLACK, COLOR_WHITE);

    tYPos += TEXT_SIZE_11;
    dtostrf(aSensorCallbackInfo->ValueY, 5, 2, &sStringBuffer[15]);
    sprintf_P(sStringBuffer, PSTR("Acc.Y %s"), &sStringBuffer[15]);
    BlueDisplay1.drawText(ACCELEROMETER_PRINT_VALUES_X, tYPos, sStringBuffer);

    tYPos += TEXT_SIZE_11;
    dtostrf(aSensorCallbackInfo->ValueZ, 5, 2, &sStringBuffer[15]);
    sprintf_P(sStringBuffer, PSTR("Acc.Z %s"), &sStringBuffer[15]);
    BlueDisplay1.drawText(ACCELEROMETER_PRINT_VALUES_X, tYPos, sStringBuffer);
#endif
    digitalWrite(LED_BUILTIN, LOW);
}

void doGyroscopeChange(struct SensorCallback * aSensorCallbackInfo) {
    /*
     * Show values on 4 sliders
     */
    // scale value for full scale = SLIDER_BAR_LENGTH at at 3.3
    int tLeftRightValue = aSensorCallbackInfo->ValueY * ((SLIDER_BAR_LENGTH * 5) / 10);
    if (sLastGyroscopeLeftRightValue != tLeftRightValue) {
        bool tSignChanged = (sLastGyroscopeLeftRightValue ^ tLeftRightValue) < 0;
        sLastGyroscopeLeftRightValue = tLeftRightValue;
        if (tLeftRightValue > 0) {
            SliderRollRight.setValueAndDrawBar(tLeftRightValue);
            if (tSignChanged) {
                // clear bar by overwrite wit accelerator value
                SliderAccelerationLeft.setValueAndDrawBar(sLastAcceleratorLeftSliderValue);
            }
        } else {
            SliderRollLeft.setValueAndDrawBar(-tLeftRightValue);
            if (tSignChanged) {
                SliderAccelerationRight.setValueAndDrawBar(sLastAcceleratorRightSliderValue);
            }
        }
    }

    int tForwardBackwardValue = aSensorCallbackInfo->ValueX * ((SLIDER_BAR_LENGTH * 5) / 10);
    if (sGyroscopeForwardBackwardValue != tForwardBackwardValue) {
        bool tSignChanged = (sGyroscopeForwardBackwardValue ^ tForwardBackwardValue) < 0;
        sGyroscopeForwardBackwardValue = tForwardBackwardValue;
        if (tForwardBackwardValue > 0) {
            SliderTiltBackward.setValueAndDrawBar(tForwardBackwardValue);
            if (tSignChanged) {
                SliderAccelerationForward.setValueAndDrawBar(sLastAcceleratorForwardSliderValue);
            }
        } else {
            SliderTiltForward.setValueAndDrawBar(-tForwardBackwardValue);
            if (tSignChanged) {
                SliderAccelerationBackward.setValueAndDrawBar(sLastAcceleratorBackwardSliderValue);
            }
        }
    }
    uint16_t tYPos = ACCELERATION_SLIDER_CENTER_Y + 2 * TEXT_SIZE_11_HEIGHT;
    dtostrf(aSensorCallbackInfo->ValueX, 5, 2, &sStringBuffer[15]);
    sprintf_P(sStringBuffer, PSTR("Gyr.X %s"), &sStringBuffer[15]);
    BlueDisplay1.drawText(GYROSCOPE_PRINT_VALUES_X, tYPos, sStringBuffer, TEXT_SIZE_11, COLOR_BLACK, COLOR_WHITE);

    tYPos += TEXT_SIZE_11;
    dtostrf(aSensorCallbackInfo->ValueY, 5, 2, &sStringBuffer[15]);
    sprintf_P(sStringBuffer, PSTR("Gyr.Y %s"), &sStringBuffer[15]);
    BlueDisplay1.drawText(GYROSCOPE_PRINT_VALUES_X, tYPos, sStringBuffer);

    tYPos += TEXT_SIZE_11;
    dtostrf(aSensorCallbackInfo->ValueZ, 5, 2, &sStringBuffer[15]);
    sprintf_P(sStringBuffer, PSTR("Gyr.Z %s"), &sStringBuffer[15]);
    BlueDisplay1.drawText(GYROSCOPE_PRINT_VALUES_X, tYPos, sStringBuffer);
}

void doSensorChange(uint8_t aSensorType, struct SensorCallback * aSensorCallbackInfo) {
    if (aSensorType == FLAG_SENSOR_TYPE_ACCELEROMETER) {
        doAccelerometerChange(aSensorCallbackInfo);
    } else {
        doGyroscopeChange(aSensorCallbackInfo);
    }
}
/*
 * Function used as callback handler for connect too
 */
void initDisplay(void) {
    // Initialize display size and flags
    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE | BD_FLAG_TOUCH_BASIC_DISABLE, DISPLAY_WIDTH,
    DISPLAY_HEIGHT);

    // Since landscape has 2 orientations, let the user choose the right one and lock it.
    BlueDisplay1.setScreenOrientationLock(FLAG_SCREEN_ORIENTATION_LOCK_CURRENT);

    // FLAG_SENSOR_DELAY_UI -> 60 ms sensor rate, FLAG_SENSOR_DELAY_NORMAL -> 200 ms sensor rate
    registerSensorChangeCallback(FLAG_SENSOR_TYPE_ACCELEROMETER, FLAG_SENSOR_DELAY_NORMAL, FLAG_SENSOR_NO_FILTER, &doSensorChange);
    registerSensorChangeCallback(FLAG_SENSOR_TYPE_GYROSCOPE, FLAG_SENSOR_DELAY_NORMAL, FLAG_SENSOR_NO_FILTER, &doSensorChange);

    /*
     * 4 Slider
     */
// Position Slider at middle of screen
// Top slider
    SliderAccelerationForward.init(ACCELERATION_SLIDER_LEFT_X, ACCELERATION_SLIDER_CENTER_Y - SLIDER_BAR_LENGTH,
    SLIDER_BAR_WIDTH, SLIDER_BAR_LENGTH, SLIDER_BAR_THRESHOLD, 0, COLOR_YELLOW, COLOR_GREEN, FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    SliderAccelerationForward.setBarThresholdColor(COLOR_BLUE);

// Bottom slider
    SliderAccelerationBackward.init(ACCELERATION_SLIDER_LEFT_X, ACCELERATION_SLIDER_CENTER_Y,
    SLIDER_BAR_WIDTH, -(SLIDER_BAR_LENGTH), SLIDER_BAR_THRESHOLD, 0, COLOR_YELLOW, COLOR_GREEN, FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    SliderAccelerationBackward.setBarThresholdColor(COLOR_BLUE);

// slider right from velocity at middle of screen
    SliderAccelerationRight.init(ACCELERATION_SLIDER_RIGHT_X, ACCELERATION_SLIDER_TOP_Y, SLIDER_BAR_WIDTH, SLIDER_BAR_LENGTH,
    SLIDER_BAR_THRESHOLD, 0, COLOR_YELLOW, COLOR_GREEN, FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    SliderAccelerationRight.setBarThresholdColor(COLOR_BLUE);

// Position inverse slider left from Velocity at middle of screen
    SliderAccelerationLeft.init(ACCELERATION_SLIDER_LEFT_X - SLIDER_BAR_LENGTH, ACCELERATION_SLIDER_TOP_Y, SLIDER_BAR_WIDTH,
            -(SLIDER_BAR_LENGTH), SLIDER_BAR_THRESHOLD, 0, COLOR_YELLOW, COLOR_GREEN,
            FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    SliderAccelerationLeft.setBarThresholdColor(COLOR_BLUE);

    /*
     * This 4 sliders are thinner and overlay the acceleration sliders
     */
    SliderTiltForward.init(ROLL_PITCH_YAW_SLIDER_LEFT_X, ROLL_PITCH_YAW_SLIDER_CENTER_Y - SLIDER_BAR_LENGTH,
    ROLL_PITCH_YAW_SLIDER_BAR_WIDTH, SLIDER_BAR_LENGTH, SLIDER_BAR_THRESHOLD, 0, COLOR_YELLOW, COLOR_RED, FLAG_SLIDER_IS_ONLY_OUTPUT,
    NULL);
    SliderTiltForward.setBarThresholdColor(COLOR_BLUE);

    SliderTiltBackward.init(ROLL_PITCH_YAW_SLIDER_LEFT_X, ROLL_PITCH_YAW_SLIDER_CENTER_Y, ROLL_PITCH_YAW_SLIDER_BAR_WIDTH,
            -(SLIDER_BAR_LENGTH), SLIDER_BAR_THRESHOLD, 0, COLOR_YELLOW, COLOR_RED, FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    SliderTiltBackward.setBarThresholdColor(COLOR_BLUE);

    SliderRollRight.init(ACCELERATION_SLIDER_RIGHT_X, ROLL_PITCH_YAW_SLIDER_TOP_Y, ROLL_PITCH_YAW_SLIDER_BAR_WIDTH,
    SLIDER_BAR_LENGTH,
    SLIDER_BAR_THRESHOLD, 0, COLOR_YELLOW, COLOR_RED, FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    SliderRollRight.setBarThresholdColor(COLOR_BLUE);

    SliderRollLeft.init(ACCELERATION_SLIDER_LEFT_X - SLIDER_BAR_LENGTH, ROLL_PITCH_YAW_SLIDER_TOP_Y,
    ROLL_PITCH_YAW_SLIDER_BAR_WIDTH, -(SLIDER_BAR_LENGTH), SLIDER_BAR_THRESHOLD, 0, COLOR_YELLOW, COLOR_RED,
            FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    SliderRollLeft.setBarThresholdColor(COLOR_BLUE);
}

/*
 * Function is called for resize + connect too
 */
void drawGui(void) {
    BlueDisplay1.clearDisplay(COLOR_WHITE);
    SliderAccelerationForward.drawSlider();
    SliderAccelerationBackward.drawSlider();
    SliderAccelerationRight.drawSlider();
    SliderAccelerationLeft.drawSlider();
    SliderTiltForward.drawSlider();
    SliderTiltBackward.drawSlider();
    SliderRollRight.drawSlider();
    SliderRollLeft.drawSlider();
}
