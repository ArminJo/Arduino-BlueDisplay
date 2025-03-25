/*
 * ShowSensorValues.cpp
 *
 *  Demo of using the sensor features of BlueDisplay library for HC-05 on Arduino.
 *  Screen orientation is fixed to the orientation at connect time.
 *  Sensor values can be printed on plotter if one of the two macros
 *  SHOW_ACCELEROMETER_VALUES_ON_PLOTTER or SHOW_GYROSCOPE_VALUES_ON_PLOTTER is defined
 *
 *  Android axis are defined for "natural" screen orientation, which is portrait for my devices:
 *  See https://source.android.com/devices/sensors/sensor-types
 *    When the device lies flat on a table and its left side is down and right side is up or pushed on its left side toward the right,
 *      the X acceleration value is positive.
 *    When the device lies flat on a table and its bottom side is down and top side is up or pushed on its bottom side toward the top,
 *      the Y acceleration value is positive.
 *    When the device lies flat on a table, the acceleration value along Z is +9.81 (m/s^2)
 *
 *  The BlueDisplay application converts the axis, so that this definition holds for EACH screen orientation.
 *  So we have:
 *  X positive -> left down
 *  X negative -> right down
 *  Y positive -> backward / bottom down
 *  Y negative -> forward  / top down
 *  Unit is (m/s^2)
 *
 *  Rotation is positive in the counterclockwise direction:
 *  X positive -> roll bottom downwards
 *  X negative -> roll top downwards
 *  Y positive -> pitch right downwards
 *  Y negative -> pitch left downwards
 *  Z positive -> rotate counterclockwise
 *  Z negative -> rotate clockwise
 *  Unit is radians per second (rad/s) 1 -> ~57 degree per second
 *
 *  If FLAG_SENSOR_SIMPLE_FILTER is set on sensor registering, then sensor values are sent via BT only if values changed.
 *  To avoid noise (event value is solely switching between 2 values), values are skipped too if they are equal last or second last value.
 *
 *
 *  Copyright (C) 2014-2025  Armin Joachimsmeyer
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
 * Settings to configure the BlueDisplay library and to reduce its size
 */
//#define BLUETOOTH_BAUD_RATE BAUD_115200   // Activate this, if you have reprogrammed the HC05 module for 115200, otherwise 9600 is used as baud rate
#define DO_NOT_NEED_BASIC_TOUCH_EVENTS    // Disables basic touch events down, move and up. Saves 620 bytes program memory and 36 bytes RAM
//#define DO_NOT_NEED_LONG_TOUCH_DOWN_AND_SWIPE_EVENTS  // Disables LongTouchDown and SwipeEnd events. Saves up to 88 bytes program memory and 4 bytes RAM.
#define DO_NOT_NEED_SPEAK_EVENTS            // Disables SpeakingDone event handling. Saves up to 54 bytes program memory and 18 bytes RAM.
//#define BD_USE_SIMPLE_SERIAL                 // Do not use the Serial object. Saves up to 1250 bytes program memory and 185 bytes RAM, if Serial is not used otherwise
//#define BD_USE_USB_SERIAL                    // Activate it, if you want to force using Serial instead of Serial1 for direct USB cable connection* to your smartphone / tablet.
#include "BlueDisplay.hpp"

// only one macro can be activated
//#define SHOW_ACCELEROMETER_VALUES_ON_PLOTTER
//#define SHOW_GYROSCOPE_VALUES_ON_PLOTTER
#if defined(SHOW_ACCELEROMETER_VALUES_ON_PLOTTER) && defined(SHOW_GYROSCOPE_VALUES_ON_PLOTTER)
#warning We can only plot one sensor, but both are enabled -> plot only accelerometer.
#undef SHOW_GYROSCOPE_VALUES_ON_PLOTTER
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

#define ACCELEROMETER_PRINT_VALUES_X    (ACCELERATION_SLIDER_CENTER_X - (11 * TEXT_SIZE_11_WIDTH))

// Pan = Yaw, Tilt = Pitch
#define ROLL_PITCH_YAW_SLIDER_BAR_WIDTH 4 // must be even

#define ROLL_PITCH_YAW_SLIDER_CENTER_X  (DISPLAY_WIDTH / 2)
#define ROLL_PITCH_YAW_SLIDER_LEFT_X    (ROLL_PITCH_YAW_SLIDER_CENTER_X - (ROLL_PITCH_YAW_SLIDER_BAR_WIDTH / 2))
#define ROLL_PITCH_YAW_SLIDER_RIGHT_X   (ROLL_PITCH_YAW_SLIDER_CENTER_X + (ROLL_PITCH_YAW_SLIDER_BAR_WIDTH / 2))

#define ROLL_PITCH_YAW_SLIDER_CENTER_Y  (DISPLAY_HEIGHT / 2)
#define ROLL_PITCH_YAW_SLIDER_TOP_Y     (ROLL_PITCH_YAW_SLIDER_CENTER_Y - (ROLL_PITCH_YAW_SLIDER_BAR_WIDTH / 2))

#define GYROSCOPE_PRINT_VALUES_X        (ROLL_PITCH_YAW_SLIDER_CENTER_X + TEXT_SIZE_11)

// 4 Sliders for accelerometer and 4 for gyroscope
BDSlider SliderAccelerationForward;     // Y negative
BDSlider SliderAccelerationBackward;    // Y positive
BDSlider SliderAccelerationRight;       // X negative
BDSlider SliderAccelerationLeft;        // X positive

BDSlider SliderRollForward;
BDSlider SliderRollBackward;
BDSlider SliderPitchRight;
BDSlider SliderPitchLeft;

struct positiveNegativeSlider sAccelerationForwardBackwardSliders;
struct positiveNegativeSlider sAccelerationLeftRightSliders;
struct positiveNegativeSlider sRollForwardBackwardSliders;
struct positiveNegativeSlider sPitchLeftRightSliders;

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
char sStringBuffer[20];

// PROGMEM messages sent by BlueDisplay1.debug() are truncated to 32 characters :-(, so must use RAM here
const char StartMessage[] PROGMEM = "START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY;

// Helper macro for getting a macro definition as string
#if !defined(STR)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#endif

/*******************************************************************************************
 * Program code starts here
 *******************************************************************************************/

void setup() {
    // Initialize the LED pin as output.
    pinMode(LED_BUILTIN, OUTPUT);

#if defined(ESP32)
    Serial.begin(115200);
    Serial.println(StartMessage);
    initSerial("ESP-BD_Example");
    Serial.println("Start ESP32 BT-client with name \"ESP-BD_Example\"");
#else
    initSerial();
#endif

    /*
     * Register callback handler and wait for 300 ms if Bluetooth connection is still active.
     * For ESP32 and after power on of the Bluetooth module (HC-05) at other platforms, Bluetooth connection is most likely not active here.
     *
     * If active, mCurrentDisplaySize and mHostUnixTimestamp are set and initDisplay() and drawGui() functions are called.
     * If not active, the periodic call of checkAndHandleEvents() in the main loop waits for the (re)connection and then performs the same actions.
     */
    BlueDisplay1.initCommunication(&Serial, &initDisplay, &drawGui); // introduces up to 1.5 seconds delay

#if defined(BD_USE_SERIAL1) || defined(ESP32) // BD_USE_SERIAL1 may be defined in BlueSerial.h
// Serial(0) is available for Serial.print output.
#  if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/ \
    || defined(SERIALUSB_PID)  || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#  endif

#  if !(defined(SHOW_ACCELEROMETER_VALUES_ON_PLOTTER) || defined(SHOW_GYROSCOPE_VALUES_ON_PLOTTER))
    // Just to know which program is running on my Arduino
    Serial.println(reinterpret_cast<const __FlashStringHelper *>(StartMessage));
#  endif
#elif !defined(BD_USE_SIMPLE_SERIAL)
    // If using simple serial on first USART we cannot use Serial.print, since this uses the same interrupt vector as simple serial.
#  if !(defined(SHOW_ACCELEROMETER_VALUES_ON_PLOTTER) || defined(SHOW_GYROSCOPE_VALUES_ON_PLOTTER))
    if (!BlueDisplay1.isConnectionEstablished()) {
#    if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/ \
    || defined(SERIALUSB_PID)  || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_attiny3217)
        delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#    endif
        // If connection is enabled, this message was already sent as BlueDisplay1.debug()
        Serial.println(reinterpret_cast<const __FlashStringHelper *>(StartMessage));
    }
# endif
#endif

#if defined(SHOW_ACCELEROMETER_VALUES_ON_PLOTTER)
    Serial.println();
    Serial.println(F("Acceleration_X Acceleration_Y Acceleration_Z")); // Caption for Plotter
#elif defined(SHOW_GYROSCOPE_VALUES_ON_PLOTTER)
    Serial.println();
    Serial.println(F("Roll Pitch Yaw")); // Caption for Plotter
#endif
}

void loop() {
    checkAndHandleEvents();
}

/*
 * Accelerometer callback handler
 * Unit is (m/s^2)
 *  X positive -> left down
 *  X negative -> right down
 *  Y positive -> backward / bottom down
 *  Y negative -> forward  / top down
 */
void doAccelerometerChange(struct SensorCallback *aSensorCallbackInfo) {
#if defined(SHOW_ACCELEROMETER_VALUES_ON_PLOTTER)
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
     * Show values on 4 sliders, no dead band applied
     */
    // scale value for full scale = SLIDER_BAR_LENGTH at at 30 degree
    int tLeftRightValue = aSensorCallbackInfo->ValueX * ((SLIDER_BAR_LENGTH) / 10);
    setPositiveNegativeSliders(&sAccelerationLeftRightSliders, tLeftRightValue, 0);

    int tForwardBackwardValue = aSensorCallbackInfo->ValueY * ((SLIDER_BAR_LENGTH) / 10);
    setPositiveNegativeSliders(&sAccelerationForwardBackwardSliders, tForwardBackwardValue, 0);

    uint16_t tYPos = ACCELERATION_SLIDER_CENTER_Y + 2 * TEXT_SIZE_11_HEIGHT;
    dtostrf(aSensorCallbackInfo->ValueX, 4, 1, &sStringBuffer[10]);
    snprintf_P(sStringBuffer, sizeof(sStringBuffer), PSTR("Acc.X %s"), &sStringBuffer[10]); // we have no overflow here
    BlueDisplay1.drawText(ACCELEROMETER_PRINT_VALUES_X, tYPos, sStringBuffer, TEXT_SIZE_11, COLOR16_BLACK, COLOR16_WHITE);

    tYPos += TEXT_SIZE_11;
    dtostrf(aSensorCallbackInfo->ValueY, 4, 1, &sStringBuffer[10]);
    snprintf_P(sStringBuffer, sizeof(sStringBuffer), PSTR("Acc.Y %s"), &sStringBuffer[10]);
    BlueDisplay1.drawText(ACCELEROMETER_PRINT_VALUES_X, tYPos, sStringBuffer);

    tYPos += TEXT_SIZE_11;
    dtostrf(aSensorCallbackInfo->ValueZ, 4, 1, &sStringBuffer[10]);
    snprintf_P(sStringBuffer, sizeof(sStringBuffer), PSTR("Acc.Z %s"), &sStringBuffer[10]);
    BlueDisplay1.drawText(ACCELEROMETER_PRINT_VALUES_X, tYPos, sStringBuffer);
#endif
}

/*
 *  Gyroscope callback handler
 *
 *  Unit is radians per second (rad/s) 1 -> ~57 degree per second
 *  X positive -> roll bottom downwards
 *  X negative -> roll top downwards
 *  Y positive -> pitch right downwards
 *  Y negative -> pitch left downwards
 *  Z positive -> rotate counterclockwise
 *  Z negative -> rotate clockwise
 */
void doGyroscopeChange(struct SensorCallback *aSensorCallbackInfo) {
#if defined(SHOW_GYROSCOPE_VALUES_ON_PLOTTER)
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
    // scale value for full scale = SLIDER_BAR_LENGTH at at 3.3
    int tLeftRightValue = aSensorCallbackInfo->ValueY * ((SLIDER_BAR_LENGTH * 5) / 10);
    sPitchLeftRightSliders.lastSliderValue = 0; // draw always slider if value != 0
    setPositiveNegativeSliders(&sPitchLeftRightSliders, tLeftRightValue, 0);

    int tForwardBackwardValue = aSensorCallbackInfo->ValueX * ((SLIDER_BAR_LENGTH * 5) / 10);
    sRollForwardBackwardSliders.lastSliderValue = 0;
    setPositiveNegativeSliders(&sRollForwardBackwardSliders, tForwardBackwardValue, 0);

    uint16_t tYPos = ACCELERATION_SLIDER_CENTER_Y + 2 * TEXT_SIZE_11_HEIGHT;
    dtostrf(aSensorCallbackInfo->ValueX, 4, 1, &sStringBuffer[10]);
    snprintf_P(sStringBuffer, sizeof(sStringBuffer), PSTR("Roll  %s"), &sStringBuffer[10]);
    BlueDisplay1.drawText(GYROSCOPE_PRINT_VALUES_X, tYPos, sStringBuffer, TEXT_SIZE_11, COLOR16_BLACK, COLOR16_WHITE);

    tYPos += TEXT_SIZE_11;
    dtostrf(aSensorCallbackInfo->ValueY, 4, 1, &sStringBuffer[10]);
    snprintf_P(sStringBuffer, sizeof(sStringBuffer), PSTR("Pitch %s"), &sStringBuffer[10]);
    BlueDisplay1.drawText(GYROSCOPE_PRINT_VALUES_X, tYPos, sStringBuffer);

    tYPos += TEXT_SIZE_11;
    dtostrf(aSensorCallbackInfo->ValueZ, 4, 1, &sStringBuffer[10]);
    snprintf_P(sStringBuffer, sizeof(sStringBuffer), PSTR("Yaw   %s"), &sStringBuffer[10]);
    BlueDisplay1.drawText(GYROSCOPE_PRINT_VALUES_X, tYPos, sStringBuffer);
#endif
}

void doSensorChange(uint8_t aSensorType, struct SensorCallback *aSensorCallbackInfo) {
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
    // Screen orientation is fixed to the orientation at connect time
    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL | BD_FLAG_USE_MAX_SIZE | BD_FLAG_SCREEN_ORIENTATION_LOCK_CURRENT, DISPLAY_WIDTH,
    DISPLAY_HEIGHT);

    // FLAG_SENSOR_DELAY_UI -> 60 ms sensor rate, FLAG_SENSOR_DELAY_NORMAL -> 200 ms sensor rate
#if !defined(SHOW_GYROSCOPE_VALUES_ON_PLOTTER)
    registerSensorChangeCallback(FLAG_SENSOR_TYPE_ACCELEROMETER, FLAG_SENSOR_DELAY_NORMAL, FLAG_SENSOR_SIMPLE_FILTER,
            &doSensorChange);
#endif
#if !defined(SHOW_ACCELEROMETER_VALUES_ON_PLOTTER)
    registerSensorChangeCallback(FLAG_SENSOR_TYPE_GYROSCOPE, FLAG_SENSOR_DELAY_NORMAL, FLAG_SENSOR_SIMPLE_FILTER, &doSensorChange);
#endif

#if !(defined(SHOW_ACCELEROMETER_VALUES_ON_PLOTTER) || defined(SHOW_GYROSCOPE_VALUES_ON_PLOTTER))

    /*
     * 4 Slider positioned as a cross
     */
    // Top slider
    SliderAccelerationForward.init(ACCELERATION_SLIDER_LEFT_X, ACCELERATION_SLIDER_CENTER_Y - SLIDER_BAR_LENGTH,
    SLIDER_BAR_WIDTH, SLIDER_BAR_LENGTH, SLIDER_BAR_THRESHOLD, 0, COLOR16_YELLOW, COLOR16_GREEN, FLAG_SLIDER_IS_ONLY_OUTPUT);
//    SliderAccelerationForward.setBarThresholdDefaultColor(COLOR16_BLUE); // since app version 4.3
    SliderAccelerationForward.setBarThresholdColor(COLOR16_BLUE);
    sAccelerationForwardBackwardSliders.negativeSliderPtr = &SliderAccelerationForward;

    // Bottom inverse slider (length is negative)
    SliderAccelerationBackward.init(ACCELERATION_SLIDER_LEFT_X, ACCELERATION_SLIDER_CENTER_Y,
    SLIDER_BAR_WIDTH, -(SLIDER_BAR_LENGTH), SLIDER_BAR_THRESHOLD, 0, COLOR16_YELLOW, COLOR16_GREEN, FLAG_SLIDER_IS_ONLY_OUTPUT);
    SliderAccelerationBackward.setBarThresholdColor(COLOR16_BLUE);
    sAccelerationForwardBackwardSliders.positiveSliderPtr = &SliderAccelerationBackward;

    // slider right from forward
    SliderAccelerationRight.init(ACCELERATION_SLIDER_RIGHT_X, ACCELERATION_SLIDER_TOP_Y, SLIDER_BAR_WIDTH, SLIDER_BAR_LENGTH,
    SLIDER_BAR_THRESHOLD, 0, COLOR16_YELLOW, COLOR16_GREEN, FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT);
    SliderAccelerationRight.setBarThresholdColor(COLOR16_BLUE);
    sAccelerationLeftRightSliders.negativeSliderPtr = &SliderAccelerationRight;

    // Position inverse slider left from forward
    SliderAccelerationLeft.init(ACCELERATION_SLIDER_LEFT_X - SLIDER_BAR_LENGTH, ACCELERATION_SLIDER_TOP_Y, SLIDER_BAR_WIDTH,
            -(SLIDER_BAR_LENGTH), SLIDER_BAR_THRESHOLD, 0, COLOR16_YELLOW, COLOR16_GREEN,
            FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT);
    SliderAccelerationLeft.setBarThresholdColor(COLOR16_BLUE);
    sAccelerationLeftRightSliders.positiveSliderPtr = &SliderAccelerationLeft;

    /*
     * This 4 sliders are thinner and overlay the acceleration sliders
     */
    SliderRollForward.init(ROLL_PITCH_YAW_SLIDER_LEFT_X, ROLL_PITCH_YAW_SLIDER_CENTER_Y - SLIDER_BAR_LENGTH,
    ROLL_PITCH_YAW_SLIDER_BAR_WIDTH, SLIDER_BAR_LENGTH, SLIDER_BAR_THRESHOLD, 0, COLOR16_YELLOW, COLOR16_RED,
            FLAG_SLIDER_IS_ONLY_OUTPUT);
    SliderRollForward.setBarThresholdColor(COLOR16_BLUE);
    sRollForwardBackwardSliders.negativeSliderPtr = &SliderRollForward;

    SliderRollBackward.init(ROLL_PITCH_YAW_SLIDER_LEFT_X, ROLL_PITCH_YAW_SLIDER_CENTER_Y, ROLL_PITCH_YAW_SLIDER_BAR_WIDTH,
            -(SLIDER_BAR_LENGTH), SLIDER_BAR_THRESHOLD, 0, COLOR16_YELLOW, COLOR16_RED, FLAG_SLIDER_IS_ONLY_OUTPUT);
    SliderRollBackward.setBarThresholdColor(COLOR16_BLUE);
    sRollForwardBackwardSliders.positiveSliderPtr = &SliderRollBackward;

    SliderPitchRight.init(ACCELERATION_SLIDER_RIGHT_X, ROLL_PITCH_YAW_SLIDER_TOP_Y, ROLL_PITCH_YAW_SLIDER_BAR_WIDTH,
    SLIDER_BAR_LENGTH, SLIDER_BAR_THRESHOLD, 0, COLOR16_YELLOW, COLOR16_RED,
            FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT);
    SliderPitchRight.setBarThresholdColor(COLOR16_BLUE);
    sPitchLeftRightSliders.positiveSliderPtr = &SliderPitchRight;

    SliderPitchLeft.init(ACCELERATION_SLIDER_LEFT_X - SLIDER_BAR_LENGTH, ROLL_PITCH_YAW_SLIDER_TOP_Y,
    ROLL_PITCH_YAW_SLIDER_BAR_WIDTH, -(SLIDER_BAR_LENGTH), SLIDER_BAR_THRESHOLD, 0, COLOR16_YELLOW, COLOR16_RED,
            FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT);
    SliderPitchLeft.setBarThresholdColor(COLOR16_BLUE);
    sPitchLeftRightSliders.negativeSliderPtr = &SliderPitchLeft;
#endif

    BlueDisplay1.debug(StartMessage);
}

/*
 * Function is called for resize + connect too
 */
void drawGui(void) {
#if !(defined(SHOW_ACCELEROMETER_VALUES_ON_PLOTTER) || defined(SHOW_GYROSCOPE_VALUES_ON_PLOTTER))
    BlueDisplay1.clearDisplay(COLOR16_WHITE);
    SliderAccelerationForward.drawSlider();
    SliderAccelerationBackward.drawSlider();
    SliderAccelerationRight.drawSlider();
    SliderAccelerationLeft.drawSlider();
    SliderRollForward.drawSlider();
    SliderRollBackward.drawSlider();
    SliderPitchRight.drawSlider();
    SliderPitchLeft.drawSlider();
#endif
}
