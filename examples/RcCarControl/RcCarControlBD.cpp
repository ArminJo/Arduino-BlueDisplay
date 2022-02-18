/*
 *  RcCarControl.cpp
 *  Demo of using the BlueDisplay library for HC-05 on Arduino
 *  Example of controlling a RC-car by smartphone accelerometer sensor

 *  Copyright (C) 2015-2020  Armin Joachimsmeyer
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
#include "RcCarControlBD.h"

#include "BlueDisplay.h"

#include "HCSR04.h"
#include "Servo.h"

/****************************************************************************
 * Change this if you have reprogrammed the hc05 module for other baud rate
 ***************************************************************************/
#ifndef BLUETOOTH_BAUD_RATE
//#define BLUETOOTH_BAUD_RATE BAUD_115200
#define BLUETOOTH_BAUD_RATE BAUD_9600
#endif

// These pins are used by Timer 2
const int BACKWARD_MOTOR_PWM_PIN = 11;
const int FORWARD_MOTOR_PWM_PIN = 3;
const int RIGHT_PIN = 4;
const int LEFT_PIN = 5;
const int LASER_POWER_PIN = 6;
const int LASER_SERVO_PIN = 9;
const int TRIGGER_PIN = 7;
const int ECHO_PIN = 8;

/*
 * Distance / Follower mode
 */
#define FOLLOWER_DISTANCE_MINIMUM_CENTIMETER         20 // If measured distance is less than this value, go backwards
#define FOLLOWER_DISTANCE_MAXIMUM_CENTIMETER         30 // If measured distance is greater than this value, go forward
#define FOLLOWER_DISTANCE_DELTA_CENTIMETER           (FOLLOWER_DISTANCE_MAXIMUM_CENTIMETER - FOLLOWER_DISTANCE_MINIMUM_CENTIMETER)
const int FOLLOWER_MAX_SPEED = 150; // empirical value

#define FILTER_WEIGHT 4 // must be 2^n
#define FILTER_WEIGHT_EXPONENT 2 // must be n of 2^n

BDButton TouchButtonFollowerOnOff;
BDSlider SliderShowDistance;
bool sFollowerMode = false;
// to start follower mode after first distance < DISTANCE_TO_HOLD
bool sFollowerModeJustStarted = true;
void doFollowerOnOff(BDButton *aTheTouchedButton, int16_t aValue);

/*
 * Buttons
 */
BDButton TouchButtonRcCarStartStop;
void doRcCarStartStop(BDButton *aTheTochedButton, int16_t aValue);
void resetOutputs(void);
bool sRCCarStarted = true;

/*
 * Laser
 */
BDButton TouchButtonLaserOnOff;
void doLaserOnOff(BDButton *aTheTouchedButton, int16_t aValue);
BDSlider SliderSpeed;
void doLaserPosition(BDSlider *aTheTouchedSlider, uint16_t aValue);
bool LaserOn = true;
Servo ServoLaser;

BDButton TouchButtonSetZero;
void doSetZero(BDButton *aTheTouchedButton, int16_t aValue);
#define CALLS_FOR_ZERO_ADJUSTMENT 8 // The values of the first 8 calls are used as zero value.
int sSensorChangeCallCountForZeroAdjustment;
float sYZeroValueAdded; // The accumulator for the values of the first 8 calls.
float sYZeroValue = 0;

/*
 * Slider
 */
#define SLIDER_BACKGROUND_COLOR COLOR16_YELLOW
#define SLIDER_BAR_COLOR COLOR16_GREEN
#define SLIDER_THRESHOLD_COLOR COLOR16_BLUE
/*
 * Velocity
 */
BDSlider SliderVelocityForward;
BDSlider SliderVelocityBackward;
int sLastSliderVelocityValue = 0;
int sLastSpeedSliderValue = 0;
// true if front distance sensor indicates to less clearance
bool sForwardStopByDistance = false;
// stop motor if velocity is less or equal MOTOR_DEAD_BAND_VALUE (max velocity value is 255)
#define MOTOR_DEAD_BAND_VALUE 60

/*
 * Direction
 */
BDSlider SliderRight;
BDSlider SliderLeft;
int sLastHorizontalSliderValue = 0;

/*
 * Timing
 */
#define SENSOR_RECEIVE_TIMEOUT_MILLIS 500
uint32_t sMillisOfLastVCCInfo = 0;
#define VCC_INFO_PERIOD_MILLIS 1000

/*
 * Layout
 */
int sCurrentDisplayWidth;
int sCurrentDisplayHeight;
int sSliderWidth;
int sVerticalSliderLength;
int sSliderHeightLaser;
int sHorizontalSliderLength;
#define SLIDER_LEFT_RIGHT_THRESHOLD (sHorizontalSliderLength/4)
int sTextSize;
int sTextSizeVCC;

// a string buffer for any purpose...
char sStringBuffer[128];

void doSensorChange(uint8_t aSensorType, struct SensorCallback *aSensorCallbackInfo);

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega644A__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644PA__)
//#define INTERNAL1V1 2
#undef INTERNAL
#define INTERNAL 2
#else
#define INTERNAL 3
#endif

/*******************************************************************************************
 * Program code starts here
 *******************************************************************************************/

void drawGui(void) {
    BlueDisplay1.clearDisplay();
    SliderVelocityForward.drawSlider();
    SliderVelocityBackward.drawSlider();
    SliderRight.drawSlider();
    SliderLeft.drawSlider();
    TouchButtonSetZero.drawButton();
    TouchButtonRcCarStartStop.drawButton();

    TouchButtonFollowerOnOff.drawButton();
    SliderShowDistance.drawSlider();
    // draw cm string
    // y Formula is: mPositionY + tSliderLongWidth + aTextLayoutInfo.mMargin + (int) (0.76 * aTextLayoutInfo.mSize)
    BlueDisplay1.drawText(sCurrentDisplayWidth / 2 + sSliderWidth + 3 * getTextWidth(sTextSize),
    BUTTON_HEIGHT_4_DYN_LINE_2 - BUTTON_VERTICAL_SPACING_DYN + sTextSize / 2 + getTextAscend(sTextSize), "cm", sTextSize,
    COLOR16_BLACK, COLOR16_WHITE);
    // draw Laser Position string
    BlueDisplay1.drawText(0, sCurrentDisplayHeight / 32 + sSliderHeightLaser + sTextSize, "Laser position", sTextSize,
    COLOR16_BLACK, COLOR16_WHITE);

    SliderSpeed.drawSlider();
    TouchButtonLaserOnOff.drawButton();
}

void initDisplay(void) {
    /*
     * handle display size
     */
    BlueDisplay1.debug("XWidth=", BlueDisplay1.mCurrentDisplaySize.XWidth);
    BlueDisplay1.debug("cWidth=", BlueDisplay1.getCurrentDisplayWidth());
    BlueDisplay1.debug("mWidth=", BlueDisplay1.getMaxDisplayWidth());

    sCurrentDisplayWidth = BlueDisplay1.getMaxDisplayWidth();
    sCurrentDisplayHeight = BlueDisplay1.getMaxDisplayHeight();
    if (sCurrentDisplayWidth < sCurrentDisplayHeight) {
        // Portrait -> change to landscape 3/2 format
        sCurrentDisplayHeight = (sCurrentDisplayWidth / 3) * 2;
    }
    /*
     * compute layout values
     */
    sSliderWidth = sCurrentDisplayWidth / 16;
    sHorizontalSliderLength = sCurrentDisplayHeight / 4;

    // 3/8 of sCurrentDisplayHeight
    sSliderHeightLaser = (sCurrentDisplayHeight / 2) + (sCurrentDisplayHeight / 8);
    sVerticalSliderLength = (sCurrentDisplayHeight * 3) / 8;

    int tSliderThresholdVelocity = (sVerticalSliderLength * (MOTOR_DEAD_BAND_VALUE + 1)) / 255;
    sTextSize = sCurrentDisplayHeight / 16;
    sTextSizeVCC = sTextSize * 2;

    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_TOUCH_BASIC_DISABLE, sCurrentDisplayWidth,
            sCurrentDisplayHeight);

    sSensorChangeCallCountForZeroAdjustment = 0;
    registerSensorChangeCallback(FLAG_SENSOR_TYPE_ACCELEROMETER, FLAG_SENSOR_DELAY_UI, FLAG_SENSOR_SIMPLE_FILTER, &doSensorChange);
    // Lock the screen orientation to avoid screen flip while rotating the smartphone
    BlueDisplay1.setScreenOrientationLock(FLAG_SCREEN_ORIENTATION_LOCK_CURRENT);

    SliderSpeed.init(0, sCurrentDisplayHeight / 32, sSliderWidth * 3, sSliderHeightLaser, sSliderHeightLaser,
            sSliderHeightLaser / 2, SLIDER_BACKGROUND_COLOR, SLIDER_BAR_COLOR, FLAG_SLIDER_VERTICAL_SHOW_NOTHING, &doLaserPosition);

    /*
     * 4 Slider
     */
// Position Slider at middle of screen
// Top slider
    uint16_t tSliderLeftX = (sCurrentDisplayWidth - sSliderWidth) / 2;

    SliderVelocityForward.init(tSliderLeftX, (sCurrentDisplayHeight / 2) - sVerticalSliderLength, sSliderWidth,
            sVerticalSliderLength, tSliderThresholdVelocity, 0, SLIDER_BACKGROUND_COLOR, SLIDER_BAR_COLOR,
            FLAG_SLIDER_IS_ONLY_OUTPUT,
            NULL);
    SliderVelocityForward.setBarThresholdColor(SLIDER_THRESHOLD_COLOR);

    // Bottom slider
    SliderVelocityBackward.init(tSliderLeftX, sCurrentDisplayHeight / 2, sSliderWidth, -(sVerticalSliderLength),
            tSliderThresholdVelocity, 0, SLIDER_BACKGROUND_COLOR, SLIDER_BAR_COLOR, FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    SliderVelocityBackward.setBarThresholdColor(SLIDER_THRESHOLD_COLOR);

// Position slider right from velocity at middle of screen
    SliderRight.init(tSliderLeftX + sSliderWidth, (sCurrentDisplayHeight - sSliderWidth) / 2, sSliderWidth, sHorizontalSliderLength,
    SLIDER_LEFT_RIGHT_THRESHOLD, 0, SLIDER_BACKGROUND_COLOR, SLIDER_BAR_COLOR,
            FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    SliderRight.setBarThresholdColor(SLIDER_THRESHOLD_COLOR);

// Position inverse slider left from Velocity at middle of screen
    SliderLeft.init((tSliderLeftX) - sHorizontalSliderLength, (sCurrentDisplayHeight - sSliderWidth) / 2, sSliderWidth,
            -(sHorizontalSliderLength), SLIDER_LEFT_RIGHT_THRESHOLD, 0, SLIDER_BACKGROUND_COLOR, SLIDER_BAR_COLOR,
            FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    SliderLeft.setBarThresholdColor(SLIDER_THRESHOLD_COLOR);

    // US distance Display slider
    uint16_t tUSSliderLength = sCurrentDisplayWidth / 2 - sSliderWidth;
    SliderShowDistance.init(sCurrentDisplayWidth / 2 + sSliderWidth,
    BUTTON_HEIGHT_4_DYN_LINE_2 - sSliderWidth - BUTTON_VERTICAL_SPACING_DYN, sSliderWidth, tUSSliderLength, 99, 0, COLOR16_WHITE,
    COLOR16_GREEN, FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT | FLAG_SLIDER_SHOW_VALUE, NULL);
    SliderShowDistance.setScaleFactor(100.0 / tUSSliderLength);
    SliderShowDistance.setPrintValueProperties(sTextSize, FLAG_SLIDER_CAPTION_ALIGN_LEFT, sTextSize / 2, COLOR16_BLACK, COLOR16_WHITE);

    BlueDisplay1.debug("XWidth1=", BlueDisplay1.mCurrentDisplaySize.XWidth);
    BlueDisplay1.debug("BUTTON_WIDTH_3_DYN=", (uint16_t) BUTTON_WIDTH_3_DYN);
    /*
     * Buttons
     */
    TouchButtonRcCarStartStop.init(0, BUTTON_HEIGHT_4_DYN_LINE_4, BUTTON_WIDTH_3_DYN, BUTTON_HEIGHT_4_DYN, COLOR16_BLUE, F("Start"),
            sTextSizeVCC, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, sRCCarStarted, &doRcCarStartStop);
    TouchButtonRcCarStartStop.setCaptionForValueTrue(F("Stop"));

    TouchButtonFollowerOnOff.init(BUTTON_WIDTH_4_DYN_POS_4, BUTTON_HEIGHT_4_DYN_LINE_2, BUTTON_WIDTH_4_DYN, BUTTON_HEIGHT_4_DYN,
    COLOR16_RED, F("Follow"), sTextSizeVCC, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, sFollowerMode,
            &doFollowerOnOff);

    TouchButtonLaserOnOff.init(BUTTON_WIDTH_4_DYN_POS_4, BUTTON_HEIGHT_4_DYN_LINE_3, BUTTON_WIDTH_4_DYN, BUTTON_HEIGHT_4_DYN,
    COLOR16_RED, F("Laser"), sTextSizeVCC, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, LaserOn, &doLaserOnOff);

    TouchButtonSetZero.init(BUTTON_WIDTH_3_DYN_POS_3, BUTTON_HEIGHT_4_DYN_LINE_4, BUTTON_WIDTH_3_DYN, BUTTON_HEIGHT_4_DYN,
    COLOR16_RED, F("Zero"), sTextSizeVCC, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doSetZero);
}

void BDsetup() {
// initialize the digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(FORWARD_MOTOR_PWM_PIN, OUTPUT);
    pinMode(BACKWARD_MOTOR_PWM_PIN, OUTPUT);
    pinMode(RIGHT_PIN, OUTPUT);
    pinMode(LEFT_PIN, OUTPUT);
    pinMode(LASER_POWER_PIN, OUTPUT);

    initUSDistancePins(TRIGGER_PIN, ECHO_PIN);

    digitalWrite(LASER_POWER_PIN, LaserOn);
    ServoLaser.write(90);

    initSerial(BLUETOOTH_BAUD_RATE);

    // Register callback handler and check for connection
    BlueDisplay1.initCommunication(&initDisplay, &drawGui, &initDisplay);

#if defined(USE_SERIAL1) // defined in BlueSerial.h
    // Serial(0) is available for Serial.print output.
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) || defined(SERIALUSB_PID) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#  endif
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY));
#endif

    if (BlueDisplay1.isConnectionEstablished()) {
        BlueDisplay1.debug("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY);
    } else {
#if !defined(USE_SIMPLE_SERIAL) && !defined(USE_SERIAL1)  // print it now if not printed above
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) || defined(SERIALUSB_PID) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#  endif
// Just to know which program is running on my Arduino
        Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY));
#endif
    }

    ServoLaser.attach(LASER_SERVO_PIN);
}

void BDloop() {
    static unsigned int sDistanceCmFiltered;
    static unsigned int sLastCentimeter;

    /*
     * Stop output if connection lost
     */
    if ((millis() - sMillisOfLastReceivedBDEvent) > SENSOR_RECEIVE_TIMEOUT_MILLIS) {
        resetOutputs();
    }

#ifdef AVR
    /*
     * Print VCC and temperature each second
     */
    BlueDisplay1.printVCCAndTemperaturePeriodically(sCurrentDisplayWidth / 4, sTextSize, sTextSize, 2000);
#endif

    /*
     * Check if receive buffer contains an event
     */
    checkAndHandleEvents();

    /*
     * Measure distance
     */
    unsigned int tCentimeterNew = getUSDistanceAsCentimeter(US_DISTANCE_TIMEOUT_MICROS_FOR_1_METER); // timeout at 1m
    if (tCentimeterNew == 0) {
        // Stop on timeout
        resetOutputs();
        // set filtered value to "in range"
        sDistanceCmFiltered = (FOLLOWER_DISTANCE_MINIMUM_CENTIMETER + (FOLLOWER_DISTANCE_DELTA_CENTIMETER / 2)) << FILTER_WEIGHT_EXPONENT;
    } else {
        /*
         * Filter distance value and show
         */
        // tCurrentZeroCrossingCount = 3/4 * old
        sDistanceCmFiltered *= (FILTER_WEIGHT - 1);
        // + 1/4 * new
        sDistanceCmFiltered += tCentimeterNew;
        sDistanceCmFiltered = sDistanceCmFiltered >> FILTER_WEIGHT_EXPONENT;

        if (sLastCentimeter != sDistanceCmFiltered) {
            SliderShowDistance.setValueAndDrawBar(sDistanceCmFiltered);
            sLastCentimeter = sDistanceCmFiltered;
        }
    }

    if (sRCCarStarted) {
        /*
         * Only follower mode handled in loop
         */
        if (sFollowerMode) {
            if (sDistanceCmFiltered > FOLLOWER_DISTANCE_MAXIMUM_CENTIMETER) {
                sForwardStopByDistance = false;
                if (!sFollowerModeJustStarted) {
                    analogWrite(BACKWARD_MOTOR_PWM_PIN, 0);
                    // go forward
                    unsigned int tSpeed = MOTOR_DEAD_BAND_VALUE + (sDistanceCmFiltered - FOLLOWER_DISTANCE_MAXIMUM_CENTIMETER) * 4;
                    if (tSpeed > FOLLOWER_MAX_SPEED) {
                        tSpeed = FOLLOWER_MAX_SPEED;
                    }
                    analogWrite(FORWARD_MOTOR_PWM_PIN, tSpeed);
                    sprintf(sStringBuffer, "%3d", tSpeed);
                    SliderVelocityBackward.printValue(sStringBuffer);
                }

            } else if (sDistanceCmFiltered < FOLLOWER_DISTANCE_MINIMUM_CENTIMETER) {
                // enable follower mode
                sFollowerModeJustStarted = false;
                analogWrite(FORWARD_MOTOR_PWM_PIN, 0);
                // go backward
                sForwardStopByDistance = true;
                unsigned int tSpeed = MOTOR_DEAD_BAND_VALUE + (FOLLOWER_DISTANCE_MINIMUM_CENTIMETER - sDistanceCmFiltered) * 4;
                if (tSpeed > FOLLOWER_MAX_SPEED) {
                    tSpeed = FOLLOWER_MAX_SPEED;
                }
                analogWrite(BACKWARD_MOTOR_PWM_PIN, tSpeed);
                sprintf(sStringBuffer, "%3d", tSpeed);
                SliderVelocityBackward.printValue(sStringBuffer);
            } else {
                /*
                 * Distance is in range
                 */
                sForwardStopByDistance = false;
                resetOutputs();
            }
        }
    }
}

#pragma GCC diagnostic ignored "-Wunused-parameter"

/*
 * Handle follower mode
 */
void doFollowerOnOff(BDButton *aTheTouchedButton, int16_t aValue) {
    sFollowerMode = aValue;
    if (sFollowerMode) {
        sFollowerModeJustStarted = true;
    }
}

/*
 * Handle Laser
 */
void doLaserOnOff(BDButton *aTheTouchedButton, int16_t aValue) {
    LaserOn = aValue;
    digitalWrite(LASER_POWER_PIN, LaserOn);
}

/*
 * Convert full range to 180
 */
void doLaserPosition(BDSlider *aTheTouchedSlider, uint16_t aValue) {
    int tValue = map(aValue, 0, sSliderHeightLaser, 0, 180);
    ServoLaser.write(tValue);
}

/*
 * Handle Start/Stop
 */
void doRcCarStartStop(BDButton *aTheTouchedButton, int16_t aValue) {
    sRCCarStarted = aValue;
    if (sRCCarStarted) {
        registerSensorChangeCallback(FLAG_SENSOR_TYPE_ACCELEROMETER, FLAG_SENSOR_DELAY_UI, FLAG_SENSOR_NO_FILTER, &doSensorChange);
    } else {
        registerSensorChangeCallback(FLAG_SENSOR_TYPE_ACCELEROMETER, FLAG_SENSOR_DELAY_UI, FLAG_SENSOR_NO_FILTER, NULL);
        resetOutputs();
    }
}

/*
 * Stop output signals
 */
void resetOutputs(void) {
    analogWrite(FORWARD_MOTOR_PWM_PIN, 0);
    analogWrite(BACKWARD_MOTOR_PWM_PIN, 0);
    digitalWrite(RIGHT_PIN, LOW);
    digitalWrite(LEFT_PIN, LOW);
}

void doSetZero(BDButton *aTheTouchedButton, int16_t aValue) {
// wait for end of touch vibration
    delay(10);
    sSensorChangeCallCountForZeroAdjustment = 0;
}

uint8_t speedOverflowAndDeadBandHandling(unsigned int aSpeed) {
    // overflow handling since analogWrite only accepts byte values
    if (aSpeed > 0xFF) {
        aSpeed = 0xFF;
    }
    if (aSpeed <= MOTOR_DEAD_BAND_VALUE) {
        aSpeed = 0;
    }
    return aSpeed;
}

/*
 * Forward / backward speed
 * Values are in (m/s^2)
 * positive -> backward / bottom down
 * negative -> forward  / top down
 */
void processVerticalSensorValue(float tSensorValue) {

// Scale value
    int tSpeedValue = -((tSensorValue - sYZeroValue) * ((255 * 2) / 10));

// forward backward handling
    if (sLastSpeedSliderValue != tSpeedValue) {
        sLastSpeedSliderValue = tSpeedValue;
        if (tSpeedValue >= 0) {
            // Forward
            analogWrite(BACKWARD_MOTOR_PWM_PIN, 0);
            SliderVelocityBackward.setValueAndDrawBar(0);

            SliderVelocityForward.setValueAndDrawBar(tSpeedValue);
            tSpeedValue = speedOverflowAndDeadBandHandling(tSpeedValue);
            analogWrite(FORWARD_MOTOR_PWM_PIN, tSpeedValue);

        } else {
            // Backward
            tSpeedValue = -tSpeedValue;
            analogWrite(FORWARD_MOTOR_PWM_PIN, 0);
            SliderVelocityForward.setValueAndDrawBar(0);

            SliderVelocityBackward.setValueAndDrawBar(tSpeedValue);
            tSpeedValue = speedOverflowAndDeadBandHandling(tSpeedValue);
            analogWrite(BACKWARD_MOTOR_PWM_PIN, tSpeedValue);
        }
        /*
         * Print speed as value of bottom slider
         */
        sprintf(sStringBuffer, "%3d", tSpeedValue);
        SliderVelocityBackward.printValue(sStringBuffer);
    }
}

/*
 * Left / right coil
 * positive -> left down
 * negative -> right down
 */
void processHorizontalSensorValue(float tSensorValue) {

// scale value for full scale
    int tLeftRightValue = tSensorValue * ((sHorizontalSliderLength * 3) / 10);

// left right handling
    if (sLastHorizontalSliderValue != tLeftRightValue) {
        sLastHorizontalSliderValue = tLeftRightValue;
        uint8_t tActivePin;
        if (tLeftRightValue >= 0) {
            tActivePin = LEFT_PIN;
            digitalWrite(RIGHT_PIN, LOW);
            SliderLeft.setValueAndDrawBar(tLeftRightValue);
            SliderRight.setValueAndDrawBar(0);
        } else {
            tLeftRightValue = -tLeftRightValue;
            tActivePin = RIGHT_PIN;
            digitalWrite(LEFT_PIN, LOW);
            SliderRight.setValueAndDrawBar(tLeftRightValue);
            SliderLeft.setValueAndDrawBar(0);
        }

// dead band handling for steering synchronous to slider threshold
        if (tLeftRightValue < SLIDER_LEFT_RIGHT_THRESHOLD) {
            digitalWrite(tActivePin, LOW);
        } else {
            digitalWrite(tActivePin, HIGH);
        }
    }
}

/*
 * Sensor callback handler
 */
void doSensorChange(uint8_t aSensorType, struct SensorCallback *aSensorCallbackInfo) {
    if (sSensorChangeCallCountForZeroAdjustment < CALLS_FOR_ZERO_ADJUSTMENT) {
        if (sSensorChangeCallCountForZeroAdjustment == 0) {
            // init values
            sYZeroValueAdded = 0;
        }
        // The values of the first 8 calls are used as zero value.
        sYZeroValueAdded += aSensorCallbackInfo->ValueY;
        sSensorChangeCallCountForZeroAdjustment++;
    } else if (sSensorChangeCallCountForZeroAdjustment == CALLS_FOR_ZERO_ADJUSTMENT) {
        // compute zero value. Only Y values makes sense.
        sYZeroValue = sYZeroValueAdded / CALLS_FOR_ZERO_ADJUSTMENT;
        BlueDisplay1.playTone(24); // feedback for zero value acquired

    } else {
        if (sRCCarStarted && !sFollowerMode) {
            processVerticalSensorValue(aSensorCallbackInfo->ValueY);
            processHorizontalSensorValue(aSensorCallbackInfo->ValueX);
        }
    }
}
