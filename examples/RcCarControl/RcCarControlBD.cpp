/*
 *  RcCarControl.cpp
 *  Demo of using the BlueDisplay library for HC-05 on Arduino
 *  Example of controlling a RC-car by smartphone accelerometer sensor

 *  Copyright (C) 2015  Armin Joachimsmeyer
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

#include "Servo.h"

#include <stdlib.h> // for dtostrf

#define HC_05_BAUD_RATE BAUD_115200

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
const int DISTANCE_TO_HOLD_CENTIMETER = 20; // 20 cm
const int DISTANCE_HYSTERESE_CENTIMETER = 3; // +/- 3 cm
const int FOLLOWER_MAX_SPEED = 150; // empirical value

#define FILTER_WEIGHT 4 // must be 2^n
#define FILTER_WEIGHT_EXPONENT 2 // must be n of 2^n

BDButton TouchButtonFollowerOnOff;
BDSlider SliderShowDistance;
bool sFollowerMode = false;
// to start follower mode after first distance < DISTANCE_TO_HOLD
bool sFollowerModeJustStarted = true;
void doFollowerOnOff(BDButton * aTheTouchedButton, int16_t aValue);

/*
 * Buttons
 */
BDButton TouchButtonToneStartStop;
void doToneStartStop(BDButton * aTheTochedButton, int16_t aValue);
void resetOutputs(void);
bool sStarted = true;

/*
 * Laser
 */

BDButton TouchButtonLaserOnOff;
void doLaserOnOff(BDButton * aTheTouchedButton, int16_t aValue);
BDSlider SliderSpeed;
void doLaserPosition(BDSlider * aTheTouchedSlider, uint16_t aValue);
bool LaserOn = true;
Servo ServoLaser;

BDButton TouchButtonSetZero;
void doSetZero(BDButton * aTheTouchedButton, int16_t aValue);
#define CALLS_FOR_ZERO_ADJUSTMENT 8
int tSensorChangeCallCount;
float sYZeroValueAdded;
float sYZeroValue = 0;

/*
 * Slider
 */
#define SLIDER_BACKGROUND_COLOR COLOR_YELLOW
#define SLIDER_BAR_COLOR COLOR_GREEN
#define SLIDER_THRESHOLD_COLOR COLOR_BLUE
/*
 * Velocity
 */
BDSlider SliderVelocityForward;
BDSlider SliderVelocityBackward;
int sLastSliderVelocityValue = 0;
int sLastMotorValue = 0;
// true if front distance sensor indicates to less clearance
bool sForwardStopByDistance = false;
// stop motor if velocity is less or equal MOTOR_DEAD_BAND_VALUE (max velocity value is 255)
#define MOTOR_DEAD_BAND_VALUE 80

/*
 * Direction
 */
BDSlider SliderRight;
BDSlider SliderLeft;
int sLastLeftRightValue = 0;

/*
 * Timing
 */
uint32_t sMillisOfLastReveivedEvent = 0;
#define SENSOR_RECEIVE_TIMEOUT_MILLIS 500
uint32_t sMillisOfLastVCCInfo = 0;
#define VCC_INFO_PERIOD_MILLIS 1000

/*
 * Layout
 */
int sActualDisplayWidth;
int sActualDisplayHeight;
int sSliderSize;
#define VALUE_X_SLIDER_DEAD_BAND (sSliderSize/2)
int sSliderHeight;
int sSliderHeightLaser;
int sSliderWidth;
#define SLIDER_LEFT_RIGHT_THRESHOLD (sSliderWidth/4)
int sTextSize;
int sTextSizeVCC;

// a string buffer for any purpose...
char sStringBuffer[128];

void doSensorChange(uint8_t aSensorType, struct SensorCallback * aSensorCallbackInfo);

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
    TouchButtonToneStartStop.drawButton();

    TouchButtonFollowerOnOff.drawButton();
    SliderShowDistance.drawSlider();
    // draw cm string
    // y Formula is: mPositionY + tSliderLongWidth + aTextLayoutInfo.mMargin + (int) (0.76 * aTextLayoutInfo.mSize)
    BlueDisplay1.drawText(sActualDisplayWidth / 2 + sSliderSize + 3 * getTextWidth(sTextSize),
    BUTTON_HEIGHT_4_DYN_LINE_2 - BUTTON_VERTICAL_SPACING_DYN + sTextSize / 2 + getTextAscend(sTextSize), "cm", sTextSize,
    COLOR_BLACK,
    COLOR_WHITE);
    // draw Laser Position string
    BlueDisplay1.drawText(0, sActualDisplayHeight / 32 + sSliderHeightLaser + sTextSize, "Laser position", sTextSize,
    COLOR_BLACK, COLOR_WHITE);

    SliderSpeed.drawSlider();
    TouchButtonLaserOnOff.drawButton();
}

void initDisplay(void) {
    /*
     * handle display size
     */
    sActualDisplayWidth = BlueDisplay1.getMaxDisplayWidth();
    sActualDisplayHeight = BlueDisplay1.getMaxDisplayHeight();
    if (sActualDisplayWidth < sActualDisplayHeight) {
        // Portrait -> change to landscape 3/2 format
        sActualDisplayHeight = (sActualDisplayWidth / 3) * 2;
    }
    /*
     * compute layout values
     */
    sSliderSize = sActualDisplayWidth / 16;
    sSliderWidth = sActualDisplayHeight / 4;

    // 3/8 of sActualDisplayHeight
    sSliderHeightLaser = (sActualDisplayHeight / 2) + (sActualDisplayHeight / 8);
    sSliderHeight = ((sActualDisplayHeight / 2) + sActualDisplayHeight / 4) / 2;

    int tSliderThresholdVelocity = (sSliderHeight * (MOTOR_DEAD_BAND_VALUE + 1)) / 255;
    sTextSize = sActualDisplayHeight / 16;
    sTextSizeVCC = sTextSize * 2;

    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_TOUCH_BASIC_DISABLE, sActualDisplayWidth, sActualDisplayHeight);

    sYZeroValueAdded = 0;
    tSensorChangeCallCount = 0;
    registerSensorChangeCallback(FLAG_SENSOR_TYPE_ACCELEROMETER, FLAG_SENSOR_DELAY_UI, FLAG_SENSOR_NO_FILTER, &doSensorChange);
    // Since landscape has 2 orientations, let the user choose the right one.
    BlueDisplay1.setScreenOrientationLock(FLAG_SCREEN_ORIENTATION_LOCK_ACTUAL);

    SliderSpeed.init(0, sActualDisplayHeight / 32, sSliderSize * 3, sSliderHeightLaser, sSliderHeightLaser, sSliderHeightLaser / 2,
    SLIDER_BACKGROUND_COLOR, SLIDER_BAR_COLOR, FLAG_SLIDER_VERTICAL_SHOW_NOTHING, &doLaserPosition);

    /*
     * 4 Slider
     */
// Position Slider at middle of screen
// Top slider
    SliderVelocityForward.init((sActualDisplayWidth - sSliderSize) / 2, (sActualDisplayHeight / 2) - sSliderHeight, sSliderSize,
            sSliderHeight, tSliderThresholdVelocity, 0, SLIDER_BACKGROUND_COLOR, SLIDER_BAR_COLOR, FLAG_SLIDER_IS_ONLY_OUTPUT,
            NULL);
    SliderVelocityForward.setBarThresholdColor(SLIDER_THRESHOLD_COLOR);

    // Bottom slider
    SliderVelocityBackward.init((sActualDisplayWidth - sSliderSize) / 2, sActualDisplayHeight / 2, sSliderSize, -(sSliderHeight),
            tSliderThresholdVelocity, 0, SLIDER_BACKGROUND_COLOR, SLIDER_BAR_COLOR, FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    SliderVelocityBackward.setBarThresholdColor(SLIDER_THRESHOLD_COLOR);

// Position slider right from velocity at middle of screen
    SliderRight.init((sActualDisplayWidth + sSliderSize) / 2, (sActualDisplayHeight - sSliderSize) / 2, sSliderSize, sSliderWidth,
    SLIDER_LEFT_RIGHT_THRESHOLD, 0, SLIDER_BACKGROUND_COLOR, SLIDER_BAR_COLOR,
            FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    SliderRight.setBarThresholdColor(SLIDER_THRESHOLD_COLOR);

// Position inverse slider left from Velocity at middle of screen
    SliderLeft.init(((sActualDisplayWidth - sSliderSize) / 2) - sSliderWidth, (sActualDisplayHeight - sSliderSize) / 2, sSliderSize,
            -(sSliderWidth), SLIDER_LEFT_RIGHT_THRESHOLD, 0, SLIDER_BACKGROUND_COLOR, SLIDER_BAR_COLOR,
            FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    SliderLeft.setBarThresholdColor(SLIDER_THRESHOLD_COLOR);

    // Distance slider
    SliderShowDistance.init(sActualDisplayWidth / 2 + sSliderSize,
    BUTTON_HEIGHT_4_DYN_LINE_2 - sSliderSize - BUTTON_VERTICAL_SPACING_DYN, sSliderSize, sActualDisplayWidth / 2 - sSliderSize, 99,
            0, COLOR_WHITE, COLOR_GREEN, FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT | FLAG_SLIDER_SHOW_VALUE, NULL);
    float tScaleFactor = ((2 - sSliderSize) * 100) / sActualDisplayWidth;
    SliderShowDistance.setScaleFactor(tScaleFactor);
    SliderShowDistance.setPrintValueProperties(sTextSize, FLAG_SLIDER_CAPTION_ALIGN_LEFT, sTextSize / 2, COLOR_BLACK, COLOR_WHITE);

    /*
     * Buttons
     */
    TouchButtonToneStartStop.init(0, BUTTON_HEIGHT_4_DYN_LINE_4, BUTTON_WIDTH_3_DYN, BUTTON_HEIGHT_4_DYN,
    COLOR_BLUE, F("Start"), sTextSizeVCC, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, sStarted,
            &doToneStartStop);
    TouchButtonToneStartStop.setCaptionForValueTrue(F("Stop"));

    TouchButtonFollowerOnOff.init(BUTTON_WIDTH_4_DYN_POS_4, BUTTON_HEIGHT_4_DYN_LINE_2,
    BUTTON_WIDTH_4_DYN, BUTTON_HEIGHT_4_DYN, COLOR_RED, F("Follow"), sTextSizeVCC,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, sFollowerMode, &doFollowerOnOff);

    TouchButtonLaserOnOff.init(BUTTON_WIDTH_4_DYN_POS_4, BUTTON_HEIGHT_4_DYN_LINE_3, BUTTON_WIDTH_4_DYN,
    BUTTON_HEIGHT_4_DYN, COLOR_RED, F("Laser"), sTextSizeVCC, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN,
            LaserOn, &doLaserOnOff);

    TouchButtonSetZero.init(BUTTON_WIDTH_3_DYN_POS_3, BUTTON_HEIGHT_4_DYN_LINE_4, BUTTON_WIDTH_3_DYN,
    BUTTON_HEIGHT_4_DYN, COLOR_RED, F("Zero"), sTextSizeVCC, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doSetZero);
}

void BDsetup() {
// initialize the digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(FORWARD_MOTOR_PWM_PIN, OUTPUT);
    pinMode(BACKWARD_MOTOR_PWM_PIN, OUTPUT);
    pinMode(RIGHT_PIN, OUTPUT);
    pinMode(LEFT_PIN, OUTPUT);
    pinMode(LASER_POWER_PIN, OUTPUT);
    pinMode(TRIGGER_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    digitalWrite(LASER_POWER_PIN, LaserOn);
    ServoLaser.write(90);

    initSimpleSerial(HC_05_BAUD_RATE);
    ServoLaser.attach(LASER_SERVO_PIN);

    // Register callback handler and check for connection
    BlueDisplay1.initCommunication(&initDisplay, &initDisplay, &drawGui);
}

void BDloop() {
    static long sPulseLengthFiltered;
    static int sCentimeterOld;
    uint32_t tMillis = millis();

    /*
     * Stop output if connection lost
     */
    if ((tMillis - sMillisOfLastReveivedEvent) > SENSOR_RECEIVE_TIMEOUT_MILLIS) {
        resetOutputs();
    }

    /*
     * Print VCC and temperature each second
     */
    BlueDisplay1.printVCCAndTemperaturePeriodically(sActualDisplayWidth / 4, sTextSize, sTextSize, 2000);

    /*
     * Check if receive buffer contains an event
     */
    checkAndHandleEvents();

    if (sStarted) {
        /*
         * Measure distance
         */
        // need 10 usec Trigger Pulse
        digitalWrite(TRIGGER_PIN, HIGH);
        delayMicroseconds(40);
        // falling edge starts measurement
        digitalWrite(TRIGGER_PIN, LOW);

        unsigned long tPulseLength = pulseInLong(ECHO_PIN, HIGH, 5850); // timeout at 1m
        if (tPulseLength != 0) {
            /*
             * Filter value
             */
            // tCurrentZeroCrossingCount = 7/4 * old
            sPulseLengthFiltered *= (FILTER_WEIGHT - 1);
            // + 1/4 * new
            sPulseLengthFiltered += tPulseLength;
            sPulseLengthFiltered = sPulseLengthFiltered >> FILTER_WEIGHT_EXPONENT;
            // +1cm was measured at working device
            int tCentimeterNew = (sPulseLengthFiltered / 58) + 1;

            if (sCentimeterOld != tCentimeterNew) {
                SliderShowDistance.setActualValueAndDrawBar(tCentimeterNew);
                sCentimeterOld = tCentimeterNew;
            }

            if (sFollowerMode) {
                int tDeviationFromTargetDistance = tCentimeterNew - DISTANCE_TO_HOLD_CENTIMETER;

                if (tDeviationFromTargetDistance > DISTANCE_HYSTERESE_CENTIMETER) {
                    sForwardStopByDistance = false;
                    if (!sFollowerModeJustStarted) {
                        analogWrite(BACKWARD_MOTOR_PWM_PIN, 0);
                        // go forward
                        int tSpeed = (tDeviationFromTargetDistance * 4) + MOTOR_DEAD_BAND_VALUE;
                        if (tSpeed > FOLLOWER_MAX_SPEED) {
                            tSpeed = FOLLOWER_MAX_SPEED;
                        }
                        analogWrite(FORWARD_MOTOR_PWM_PIN, tSpeed);
                        sprintf(sStringBuffer, "%3d", tSpeed);
                        SliderVelocityBackward.printValue(sStringBuffer);
                    }

                } else if (tDeviationFromTargetDistance < -DISTANCE_HYSTERESE_CENTIMETER) {
                    // enable follower mode
                    sFollowerModeJustStarted = false;
                    analogWrite(FORWARD_MOTOR_PWM_PIN, 0);
                    // go backward
                    sForwardStopByDistance = true;
                    int tSpeed = ((-tDeviationFromTargetDistance) * 2) + MOTOR_DEAD_BAND_VALUE;
                    if (tSpeed > FOLLOWER_MAX_SPEED) {
                        tSpeed = FOLLOWER_MAX_SPEED;
                    }
                    analogWrite(BACKWARD_MOTOR_PWM_PIN, tSpeed);
                    sprintf(sStringBuffer, "%3d", tSpeed);
                    SliderVelocityBackward.printValue(sStringBuffer);
                } else {
                    sForwardStopByDistance = false;
                    analogWrite(FORWARD_MOTOR_PWM_PIN, 0);
                    analogWrite(BACKWARD_MOTOR_PWM_PIN, 0);
                }
            }
        } else if (sCentimeterOld != 100) {
            SliderShowDistance.setActualValueAndDrawBar(100);
            sCentimeterOld = 100;
        }
    } else if (sCentimeterOld != 0) {
        SliderShowDistance.setActualValueAndDrawBar(0);
        sCentimeterOld = 0;
    }
}

#pragma GCC diagnostic ignored "-Wunused-parameter"

/*
 * Handle follower mode
 */
void doFollowerOnOff(BDButton * aTheTouchedButton, int16_t aValue) {
    sFollowerMode = aValue;
    if (sFollowerMode) {
        sFollowerModeJustStarted = true;
    }
}

/*
 * Handle Laser
 */
void doLaserOnOff(BDButton * aTheTouchedButton, int16_t aValue) {
    LaserOn = aValue;
    digitalWrite(LASER_POWER_PIN, LaserOn);
}

/*
 * Convert full range to 180
 */
void doLaserPosition(BDSlider * aTheTouchedSlider, uint16_t aValue) {
    int tValue = map(aValue, 0, sSliderHeightLaser, 0, 180);
    ServoLaser.write(tValue);
}

/*
 * Handle Start/Stop
 */
void doToneStartStop(BDButton * aTheTouchedButton, int16_t aValue) {
    sStarted = aValue;
    if (sStarted) {
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

void doSetZero(BDButton * aTheTouchedButton, int16_t aValue) {
// wait for end of touch vibration
    delay(10);
    sYZeroValueAdded = 0;
    tSensorChangeCallCount = 0;
}

/*
 * Forward / backward speed
 * Values are between +10 at 90 degree canvas top is up and -10 (canvas bottom is up)
 */
void processVerticalSensorValue(float tSensorValue) {

// Scale value for full speed = 0xFF at 30 degree
    int tMotorValue = -((tSensorValue - sYZeroValue) * ((255 * 3) / 10));

// forward backward handling
    uint8_t tActiveMotorPin;
    uint8_t tInactiveMotorPin;
    BDSlider tActiveSlider;
    BDSlider tInactiveSlider;
    if (tMotorValue >= 0) {
        // Forward
//        if (sForwardStopByDistance) {
//            // distance to less
//            tMotorValue = 0;
//        }
        tActiveMotorPin = FORWARD_MOTOR_PWM_PIN;
        tInactiveMotorPin = BACKWARD_MOTOR_PWM_PIN;
        tActiveSlider = SliderVelocityForward;
        tInactiveSlider = SliderVelocityBackward;
    } else {
        tActiveMotorPin = BACKWARD_MOTOR_PWM_PIN;
        tInactiveMotorPin = FORWARD_MOTOR_PWM_PIN;
        tActiveSlider = SliderVelocityBackward;
        tInactiveSlider = SliderVelocityForward;
        tMotorValue = -tMotorValue;
    }

// dead band handling
    if (tMotorValue <= MOTOR_DEAD_BAND_VALUE) {
        tMotorValue = 0;
    }

// overflow handling since analogWrite only accepts byte values
    if (tMotorValue > 0xFF) {
        tMotorValue = 0xFF;
    }

    analogWrite(tInactiveMotorPin, 0);
// use this as delay between deactivating one channel and activating the other
    int tSliderValue = -((tSensorValue - sYZeroValue) * ((sSliderHeight * 3) / 10));
    if (tSliderValue < 0) {
        tSliderValue = -tSliderValue;
    }
    if (sLastSliderVelocityValue != tSliderValue) {
        sLastSliderVelocityValue = tSliderValue;
        tActiveSlider.setActualValueAndDrawBar(tSliderValue);
        tInactiveSlider.setActualValueAndDrawBar(0);
        if (sLastMotorValue != tMotorValue) {
            sLastMotorValue = tMotorValue;
            sprintf(sStringBuffer, "%3d", tMotorValue);
            SliderVelocityBackward.printValue(sStringBuffer);
            analogWrite(tActiveMotorPin, tMotorValue);
        }
    }
}

/*
 * Left / right coil
 * Values are between +10 at 90 degree canvas right is up and -10 (canvas left is up)
 */
void processHorizontalSensorValue(float tSensorValue) {

// scale value for full scale =SLIDER_WIDTH at at 30 degree
    int tLeftRightValue = tSensorValue * ((sSliderWidth * 3) / 10);

// left right handling
    uint8_t tActivePin;
    uint8_t tInactivePin;
    BDSlider tActiveSlider;
    BDSlider tInactiveSlider;
    if (tLeftRightValue >= 0) {
        tActivePin = LEFT_PIN;
        tInactivePin = RIGHT_PIN;
        tActiveSlider = SliderLeft;
        tInactiveSlider = SliderRight;
    } else {
        tActivePin = RIGHT_PIN;
        tInactivePin = LEFT_PIN;
        tActiveSlider = SliderRight;
        tInactiveSlider = SliderLeft;
        tLeftRightValue = -tLeftRightValue;
    }

// dead band handling for slider
    uint8_t tActiveValue = HIGH;
    if (tLeftRightValue > VALUE_X_SLIDER_DEAD_BAND) {
        tLeftRightValue = tLeftRightValue - VALUE_X_SLIDER_DEAD_BAND;
    } else {
        tLeftRightValue = 0;
    }

// dead band handling for steering synchronous to slider threshold
    if (tLeftRightValue < SLIDER_LEFT_RIGHT_THRESHOLD) {
        tActiveValue = LOW;
    }

// overflow handling
    if (tLeftRightValue > sSliderWidth) {
        tLeftRightValue = sSliderWidth;
    }

    digitalWrite(tInactivePin, LOW);
// use this as delay between deactivating one pin and activating the other
    if (sLastLeftRightValue != tLeftRightValue) {
        sLastLeftRightValue = tLeftRightValue;
        tActiveSlider.setActualValueAndDrawBar(tLeftRightValue);
        tInactiveSlider.setActualValueAndDrawBar(0);
    }

    digitalWrite(tActivePin, tActiveValue);
}

/*
 * Not used yet
 */
void printSensorInfo(struct SensorCallback* aSensorCallbackInfo) {
    dtostrf(aSensorCallbackInfo->ValueX, 7, 4, &sStringBuffer[50]);
    dtostrf(aSensorCallbackInfo->ValueY, 7, 4, &sStringBuffer[60]);
    dtostrf(aSensorCallbackInfo->ValueZ, 7, 4, &sStringBuffer[70]);
    dtostrf(sYZeroValue, 7, 4, &sStringBuffer[80]);
    snprintf(sStringBuffer, sizeof sStringBuffer, "X=%s Y=%s Z=%s Zero=%s", &sStringBuffer[50], &sStringBuffer[60],
            &sStringBuffer[70], &sStringBuffer[80]);
    BlueDisplay1.drawText(0, sTextSize, sStringBuffer, sTextSize, COLOR_BLACK, COLOR_GREEN);
}

/*
 * Sensor callback handler
 */
void doSensorChange(uint8_t aSensorType, struct SensorCallback * aSensorCallbackInfo) {
    if (tSensorChangeCallCount < CALLS_FOR_ZERO_ADJUSTMENT) {
        sYZeroValueAdded += aSensorCallbackInfo->ValueY;
    } else if (tSensorChangeCallCount == CALLS_FOR_ZERO_ADJUSTMENT) {
        // compute zero value
        sYZeroValue = sYZeroValueAdded / CALLS_FOR_ZERO_ADJUSTMENT;
        BlueDisplay1.playTone(24);
    } else {
        tSensorChangeCallCount = CALLS_FOR_ZERO_ADJUSTMENT + 1; // to prevent overflow
//        printSensorInfo(aSensorCallbackInfo);
        if (sStarted && !sFollowerMode) {
            processVerticalSensorValue(aSensorCallbackInfo->ValueY);
            processHorizontalSensorValue(aSensorCallbackInfo->ValueX);
        }
    }
    sMillisOfLastReveivedEvent = millis();
    tSensorChangeCallCount++;
}
