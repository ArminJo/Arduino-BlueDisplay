/*
 * ServoExample.cpp
 *
 *  BlueDisplay demo which behaves differently if connected or not.
 *  The accelerometer sensor of the android display is used to control two servos in a frame which holds a laser.
 *  If no BlueDisplay connection available, the servo first marks the border and then moves randomly in this area (Cat Mover).
 *  With "Auto move" you can do Cat Mover also online.
 *  This is an example for using a fullscreen GUI.
 *
 *
 *  Zero -> the actual sensor position is taken as the servos 90/90 degree position.
 *  Bias (reverse of Zero) -> take actual servos position as position for horizontal sensors position.
 *  Move -> moves randomly in the programmed border. Currently horizontal 45 to 135 and vertical 0 to 45
 *
 *
 *  Copyright (C) 2015-2023  Armin Joachimsmeyer
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

#include "ServoEasing.hpp" // for smooth auto moving

/*
 * Settings to configure the BlueDisplay library and to reduce its size
 */
//#define BLUETOOTH_BAUD_RATE BAUD_115200   // Activate this, if you have reprogrammed the HC05 module for 115200, otherwise 9600 is used as baud rate
#define DO_NOT_NEED_BASIC_TOUCH_EVENTS    // Disables basic touch events down, move and up. Saves 620 bytes program memory and 36 bytes RAM
#define DO_NOT_NEED_LONG_TOUCH_DOWN_AND_SWIPE_EVENTS  // Disables LongTouchDown and SwipeEnd events. Saves up to 88 bytes program memory and 4 bytes RAM.
//#define ONLY_CONNECT_EVENT_REQUIRED         // Disables reorientation, redraw and SensorChange events
//#define BD_USE_SIMPLE_SERIAL                 // Do not use the Serial object. Saves up to 1250 bytes program memory and 185 bytes RAM, if Serial is not used otherwise
//#define BD_USE_USB_SERIAL                    // Activate it, if you want to force using Serial instead of Serial1 for direct USB cable connection* to your smartphone / tablet.
#include "BlueDisplay.hpp"
#include "BlueDisplayUtils.hpp" // for printVCCAndTemperaturePeriodically()

#if defined(ESP32)
#define HORIZONTAL_SERVO_PIN     5 // Compatible with ServoEasing
#define VERTICAL_SERVO_PIN      18
#define LASER_POWER_PIN          2 // = LED_BUILTIN
#else
#define LASER_POWER_PIN          5 // uses timer0
// These pins are used by Timer 2 and can be also used without overhead by using LightweightServo library https://github.com/ArminJo/LightweightServo.
#define HORIZONTAL_SERVO_PIN    10
#define VERTICAL_SERVO_PIN       9
#endif

struct ServoControlStruct {
    int16_t minDegree;
    int16_t maxDegree;
};
ServoControlStruct ServoHorizontalControl;
ServoControlStruct ServoVerticalControl;

ServoEasing ServoHorizontal;
ServoEasing ServoVertical;

/*
 * Buttons
 */
BDButton TouchButtonServosStartStop;
void doServosStartStop(BDButton *aTheTouchedButton, int16_t aValue);
void resetOutputs(void);
bool sDoSensorMove;  // true move servos according tom sensor input

BDButton TouchButtonSetZero;
void doSetZero(BDButton *aTheTouchedButton, int16_t aValue);
#define CALLS_FOR_ZERO_ADJUSTMENT 8
int tSensorChangeCallCount;
float sXZeroCompensationValue; // This is the value of the sensor which is taken as "Zero" (90 servo) position
float sYZeroCompensationValue;

BDButton TouchButtonSetBias;
void doSetBias(BDButton *aTheTouchedButton, int16_t aValue);
float sXBiasValue = 0;
float sYBiasValue = 0;
float sLastSensorXValue;
float sLastSensorYValue;

BDButton TouchButtonAutoMove;
void doEnableAutoMove(BDButton *aTheTouchedButton, int16_t aValue);
bool sDoAutoMove;  // true move servos randomly - Cat mover

/*
 * Slider
 */
#define SLIDER_BACKGROUND_COLOR COLOR16_YELLOW
#define SLIDER_BAR_COLOR COLOR16_GREEN
#define SLIDER_THRESHOLD_COLOR COLOR16_BLUE

/*
 * Laser
 */
BDSlider SliderLaserPower;
// storage for laser analogWrite and laser slider value for next start if we stop laser
uint8_t sLaserPowerValue;
uint16_t sLastLaserSliderValue;
void doLaserPowerSlider(BDSlider *aTheTouchedSlider, int16_t aValue);

/*
 * Vertical
 */
BDSlider SliderUp;
BDSlider SliderDown;
int sLastSliderVerticalValue = 0;
int sLastVerticalValue = 0;

/*
 * Horizontal
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
int sCurrentDisplayWidth;
int sCurrentDisplayHeight;
int sSliderSize;
#define VALUE_X_SLIDER_DEAD_BAND (sSliderSize/2)
int sSliderHeight;
int sSliderWidth;
#define SLIDER_LEFT_RIGHT_THRESHOLD (sSliderWidth/4)
int sTextSize;

// a string buffer for any purpose...
char sStringBuffer[128];

// Callback handler for (re)connect and resize
void initDisplay(void);
void drawGui(void);

void doSensorChange(uint8_t aSensorType, struct SensorCallback *aSensorCallbackInfo);
uint8_t getRandomValue(ServoControlStruct *aServoControlStruct, ServoEasing *aServoEasing);

// PROGMEM messages sent by BlueDisplay1.debug() are truncated to 32 characters :-(, so must use RAM here
const char StartMessage[] = "START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_BLUE_DISPLAY;
const char ServoInfoMessage[] =
        "Horizontal servo pin=" STR(HORIZONTAL_SERVO_PIN) ", vertical servo pin=" STR(VERTICAL_SERVO_PIN) ", laser pin=" STR(LASER_POWER_PIN);

/*******************************************************************************************
 * Program code starts here
 *******************************************************************************************/

void setup() {
// initialize the digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(LASER_POWER_PIN, OUTPUT);

#if defined(ESP32)
#define Serial Serial0 //  From ESP32 HardwareSerial.h. if not using CDC on Boot, Arduino Serial is the UART0 device
    Serial.begin(115200);
    Serial.println(StartMessage);
    Serial.flush();
    initSerial("ESP-BD_Example");
    Serial.println("Start ESP32 BT-client with name \"ESP-BD_Example\"");
#else
    initSerial();
#endif

    /*
     * Set up servos
     */
    ServoHorizontal.attach(HORIZONTAL_SERVO_PIN);
    ServoVertical.attach(VERTICAL_SERVO_PIN);
    ServoHorizontal.setEasingType(EASE_QUADRATIC_IN_OUT);
    ServoVertical.setEasingType(EASE_QUADRATIC_IN_OUT);

    ServoHorizontalControl.minDegree = 45;
    ServoHorizontalControl.maxDegree = 135;
    ServoVerticalControl.minDegree = 0;
    ServoVerticalControl.maxDegree = 45;

    resetOutputs();

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
    // Just to know which program is running on my Arduino
    Serial.println(StartMessage);
    Serial.println(ServoInfoMessage);
#elif !defined(BD_USE_SIMPLE_SERIAL)
    // If using simple serial on first USART we cannot use Serial.print, since this uses the same interrupt vector as simple serial.
    if (!BlueDisplay1.isConnectionEstablished()) {
#  if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/ \
    || defined(SERIALUSB_PID)  || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_attiny3217)
        delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#  endif
        // If connection is enabled, this message was already sent as BlueDisplay1.debug()
        Serial.println(StartMessage);
        Serial.println(ServoInfoMessage);
    }
#endif

    delay(2000);

    if (!BlueDisplay1.isConnectionEstablished()) {
        sDoAutoMove = true; // no start button available to start auto move example, so do "autostart" here
        /*
         * show border of area which can be reached by laser
         */
#if !defined(BD_USE_SIMPLE_SERIAL) || defined(BD_USE_SERIAL1) || defined(ESP32)
        // If using simple serial on first USART we cannot use Serial.print, since this uses the same interrupt vector as simple serial.
        Serial.println(F("Not connected to BlueDisplay -> mark border of area and then do auto move."));
#endif
        /*
         * show border of area which can be reached by laser
         */
        ServoHorizontal.write(ServoHorizontalControl.minDegree);
        ServoVertical.write(ServoVerticalControl.minDegree);
        delay(500);
        analogWrite(LASER_POWER_PIN, 255);
        ServoHorizontal.easeTo(ServoHorizontalControl.maxDegree, 50);
        ServoVertical.easeTo(ServoVerticalControl.maxDegree, 50);
        ServoHorizontal.easeTo(ServoHorizontalControl.minDegree, 50);
        ServoVertical.easeTo(ServoVerticalControl.minDegree, 50);
    }
}

void loop() {

    uint32_t tMillis = millis();

    if (sDoAutoMove) {
        /*
         * Start random auto-moves, if enabled or no Bluetooth connection available
         */
        if (!ServoHorizontal.isMoving()) {
            // start new move
            delay(random(500));
            uint8_t tNewHorizontal = getRandomValue(&ServoHorizontalControl, &ServoHorizontal);
            uint8_t tNewVertical = getRandomValue(&ServoVerticalControl, &ServoVertical);
            int tSpeed = random(10, 90);
#if !defined(BD_USE_SIMPLE_SERIAL) || defined(BD_USE_SERIAL1) || defined(ESP32)
            // If using simple serial on first USART we cannot use Serial.print, since this uses the same interrupt vector as simple serial.
#  if !defined(BD_USE_SERIAL1) && !defined(ESP32)
            // If we do not use Serial1 for BlueDisplay communication, we must check if we are not connected and therefore Serial is available for info output.
            if (!BlueDisplay1.isConnectionEstablished()) {
#  endif
                Serial.print(F("Move to H="));
                Serial.print(tNewHorizontal);
                Serial.print(F(" V="));
                Serial.print(tNewVertical);
                Serial.print(F(" S="));
                Serial.println(tSpeed);
#  if !defined(BD_USE_SERIAL1) && !defined(ESP32)
            }
#  endif
#endif
            ServoHorizontal.setEaseTo(tNewHorizontal, tSpeed);
            ServoVertical.setEaseTo(tNewVertical, tSpeed);
            synchronizeAllServosAndStartInterrupt();
        }
    } else {
        /*
         * Stop servo output if stop requested or if connection lost
         */
        if (sDoSensorMove && (tMillis - sMillisOfLastReveivedEvent) > SENSOR_RECEIVE_TIMEOUT_MILLIS) {
            disableServoEasingInterrupt();
            resetOutputs();
        }
    }

#if defined(__AVR__)
    if (BlueDisplay1.isConnectionEstablished()) {
        /*
         * Print VCC and temperature each second
         */
        printVCCAndTemperaturePeriodically(BlueDisplay1, sCurrentDisplayWidth / 4, sTextSize, sTextSize,
        VCC_INFO_PERIOD_MILLIS);
    }
#endif

    /*
     * Check if receive buffer contains an event
     */
    checkAndHandleEvents();
}

void initDisplay(void) {
    /*
     * handle display size
     */
    sCurrentDisplayWidth = BlueDisplay1.getHostDisplayWidth();
    sCurrentDisplayHeight = BlueDisplay1.getHostDisplayHeight();
#if !defined(BD_USE_SIMPLE_SERIAL) && (defined(BD_USE_SERIAL1) || defined(ESP32))
    Serial.print("MaxDisplayWidth=");
    Serial.print(sCurrentDisplayWidth);
    Serial.print(" MaxDisplayHeight=");
    Serial.println(sCurrentDisplayHeight);
#endif
    if (sCurrentDisplayWidth < sCurrentDisplayHeight) {
        // Portrait -> change to landscape 3/2 format
        sCurrentDisplayHeight = (sCurrentDisplayWidth / 3) * 2;
    }
    /*
     * compute layout values
     */
    sSliderSize = sCurrentDisplayWidth / 32;
    // 3/8 of sCurrentDisplayHeight
    sSliderHeight = (sCurrentDisplayHeight / 4) + (sCurrentDisplayHeight / 8);
    sSliderWidth = sSliderHeight - VALUE_X_SLIDER_DEAD_BAND;

    sTextSize = sCurrentDisplayHeight / 9;

    BlueDisplay1.setFlagsAndSize(BD_FLAG_FIRST_RESET_ALL, sCurrentDisplayWidth, sCurrentDisplayHeight);

#if !defined(BD_USE_SIMPLE_SERIAL) && (defined(BD_USE_SERIAL1) || defined(ESP32))
    Serial.print("RequestedDisplayWidth=");
    Serial.print(sCurrentDisplayWidth);
    Serial.print(" RequestedDisplayHeight=");
    Serial.println(sCurrentDisplayHeight);
#endif
    tSensorChangeCallCount = 0;
    // Since landscape has 2 orientations, let the user choose the right one.
    BlueDisplay1.setScreenOrientationLock(FLAG_SCREEN_ORIENTATION_LOCK_CURRENT);

    uint16_t tSliderSize = sCurrentDisplayHeight / 2;

    /*
     * Laser power slider
     */
    SliderLaserPower.init(0, sCurrentDisplayHeight / 8, sSliderSize * 4, tSliderSize, (tSliderSize * 2) / 3, tSliderSize / 2,
    SLIDER_BACKGROUND_COLOR, SLIDER_BAR_COLOR, FLAG_SLIDER_VERTICAL_SHOW_NOTHING, &doLaserPowerSlider);
    SliderLaserPower.setCaptionProperties(sCurrentDisplayHeight / 16, FLAG_SLIDER_VALUE_CAPTION_ALIGN_LEFT_BELOW, 4, COLOR16_RED,
    COLOR16_WHITE);
    SliderLaserPower.setCaption("Laser");
    doLaserPowerSlider(nullptr, tSliderSize / 2); // set according to initial slider bar length

    /*
     * 4 Slider
     */
// Position Slider at middle of screen
    SliderUp.init((sCurrentDisplayWidth - sSliderSize) / 2, (sCurrentDisplayHeight / 2) - sSliderHeight + sSliderSize, sSliderSize,
            sSliderHeight, sSliderHeight, 0, SLIDER_BACKGROUND_COLOR, SLIDER_BAR_COLOR, FLAG_SLIDER_IS_ONLY_OUTPUT);
    SliderUp.setBarThresholdColor(SLIDER_THRESHOLD_COLOR);

    SliderDown.init((sCurrentDisplayWidth - sSliderSize) / 2, (sCurrentDisplayHeight / 2) + sSliderSize, sSliderSize,
            -(sSliderHeight), sSliderHeight, 0, SLIDER_BACKGROUND_COLOR, SLIDER_BAR_COLOR, FLAG_SLIDER_IS_ONLY_OUTPUT);
    SliderDown.setBarThresholdColor(SLIDER_THRESHOLD_COLOR);

// Position slider right from vertical one at middle of screen
    SliderRight.init((sCurrentDisplayWidth + sSliderSize) / 2, ((sCurrentDisplayHeight - sSliderSize) / 2) + sSliderSize,
            sSliderSize, sSliderWidth,
            SLIDER_LEFT_RIGHT_THRESHOLD, 0, SLIDER_BACKGROUND_COLOR, SLIDER_BAR_COLOR,
            FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT);
    SliderRight.setBarThresholdColor( SLIDER_THRESHOLD_COLOR);

// Position inverse slider left from vertical one at middle of screen
    SliderLeft.init(((sCurrentDisplayWidth - sSliderSize) / 2) - sSliderWidth,
            ((sCurrentDisplayHeight - sSliderSize) / 2) + sSliderSize, sSliderSize, -(sSliderWidth), SLIDER_LEFT_RIGHT_THRESHOLD, 0,
            SLIDER_BACKGROUND_COLOR, SLIDER_BAR_COLOR, FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_IS_ONLY_OUTPUT);
    SliderLeft.setBarThresholdColor(SLIDER_THRESHOLD_COLOR);

    /*
     * Buttons
     */
    TouchButtonServosStartStop.init(0, sCurrentDisplayHeight - sCurrentDisplayHeight / 4, sCurrentDisplayWidth / 4,
            sCurrentDisplayHeight / 4,
            COLOR16_BLUE, "Start", sTextSize, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, sDoSensorMove,
            &doServosStartStop);
    TouchButtonServosStartStop.setTextForValueTrue(F("Stop"));

    TouchButtonSetBias.init(sCurrentDisplayWidth - sCurrentDisplayWidth / 4,
            (sCurrentDisplayHeight - sCurrentDisplayHeight / 2) - sCurrentDisplayHeight / 32, sCurrentDisplayWidth / 4,
            sCurrentDisplayHeight / 4, COLOR16_RED, "Bias", sTextSize, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doSetBias);

    TouchButtonSetZero.init(sCurrentDisplayWidth - sCurrentDisplayWidth / 4, sCurrentDisplayHeight - sCurrentDisplayHeight / 4,
            sCurrentDisplayWidth / 4, sCurrentDisplayHeight / 4, COLOR16_RED, "Zero", sTextSize, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0,
            &doSetZero);

    TouchButtonAutoMove.init(sCurrentDisplayWidth - sCurrentDisplayWidth / 4,
            sCurrentDisplayHeight / 4 - sCurrentDisplayHeight / 16, sCurrentDisplayWidth / 4, sCurrentDisplayHeight / 4,
            COLOR16_BLUE, "Auto\nmove", sTextSize, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, sDoAutoMove,
            &doEnableAutoMove);
    TouchButtonAutoMove.setTextForValueTrue(F("Stop"));

    BlueDisplay1.debug(StartMessage);
    delay(2000);
    BlueDisplay1.debug(ServoInfoMessage);
}

void drawGui(void) {
    BlueDisplay1.clearDisplay();
    SliderUp.drawSlider();
    SliderDown.drawSlider();
    SliderRight.drawSlider();
    SliderLeft.drawSlider();
    SliderLaserPower.drawSlider();

    TouchButtonSetZero.drawButton();
    TouchButtonSetBias.drawButton();
    TouchButtonAutoMove.drawButton();
    TouchButtonServosStartStop.drawButton();
}

uint8_t getRandomValue(ServoControlStruct *aServoControlStruct, ServoEasing *aServoEasing) {
    /*
     * get new different value
     */
    uint8_t tNewTargetAngle;
    do {
        tNewTargetAngle = random(aServoControlStruct->minDegree, aServoControlStruct->maxDegree);
    } while (tNewTargetAngle == aServoEasing->MicrosecondsOrUnitsToDegree(aServoEasing->mCurrentMicrosecondsOrUnits)); // do not accept current angle as new value
    return tNewTargetAngle;
}

#pragma GCC diagnostic ignored "-Wunused-parameter"

/*
 * Use logarithmic scale: 0 -> 0, (sCurrentDisplayHeight / 2) -> 255
 */
void doLaserPowerSlider(BDSlider *aTheTouchedSlider, int16_t aValue) {
    sLastLaserSliderValue = aValue;
    float tValue = aValue;
    tValue = (tValue * 5) / sCurrentDisplayHeight; // gives 0-2.5 for 0 - sCurrentDisplayHeight/2
// 950 byte program memory required for pow() and log10f()
    tValue = pow(10, tValue);
    if (tValue > 255) {
        tValue = 255;
    }
    sLaserPowerValue = tValue;
//    BlueDisplay1.debug("Laser=", sLaserPowerValue);
    analogWrite(LASER_POWER_PIN, sLaserPowerValue);
}

/*
 * Handle Start/Stop
 */
void doServosStartStop(BDButton *aTheTouchedButton, int16_t aValue) {
    sDoSensorMove = aValue;
    if (sDoSensorMove) {
        /*
         * Do start
         */
        if (sDoAutoMove) {
            // first stop auto moves
            sDoAutoMove = false;
            TouchButtonAutoMove.setValueAndDraw(false);
        }
        registerSensorChangeCallback(FLAG_SENSOR_TYPE_ACCELEROMETER, FLAG_SENSOR_DELAY_UI, FLAG_SENSOR_NO_FILTER, &doSensorChange);
        sMillisOfLastReveivedEvent = millis();
        analogWrite(LASER_POWER_PIN, sLaserPowerValue);
        SliderLaserPower.setValueAndDrawBar(sLastLaserSliderValue);
    } else {
        /*
         * Do stop
         */
        registerSensorChangeCallback(FLAG_SENSOR_TYPE_ACCELEROMETER, FLAG_SENSOR_DELAY_UI, FLAG_SENSOR_NO_FILTER, nullptr);
        if (!sDoAutoMove) {
            resetOutputs();
        }
    }
}

/*
 * Stop output signals
 */
void resetOutputs(void) {
    ServoHorizontal.write(90);
    ServoVertical.write(90);
    analogWrite(LASER_POWER_PIN, 0);
    if (BlueDisplay1.isConnectionEstablished()) {
        SliderLaserPower.setValueAndDrawBar(0);
    }
}

/*
 * The current sensor position is taken as the 90/90 degree position
 */
void doSetZero(BDButton *aTheTouchedButton, int16_t aValue) {
// wait for end of touch vibration
    delay(10);
    tSensorChangeCallCount = 0;
}

void doEnableAutoMove(BDButton *aTheTouchedButton, int16_t aValue) {
    sDoAutoMove = aValue;
    if (sDoAutoMove) {
        if (sDoSensorMove) {
            // first stop sensor moves
            doServosStartStop(nullptr, false);
            TouchButtonServosStartStop.setValueAndDraw(false);
        }
        ServoHorizontal.setEaseTo(90, 10);
        ServoVertical.startEaseTo(90, 10);
    }
}

/*
 * take current position as horizontal one
 */
void doSetBias(BDButton *aTheTouchedButton, int16_t aValue) {
// wait for end of touch vibration
    delay(10);
    // this will do the trick
    sXBiasValue = sLastSensorXValue;
    sYBiasValue = sLastSensorYValue;
    // show message in order to see the effect
    BlueDisplay1.clearDisplay();
    BlueDisplay1.drawText(0, sTextSize + getTextAscend(sTextSize), "old position is taken \rfor horizontal input\r", sTextSize,
    COLOR16_BLACK, COLOR16_GREEN);
    delayMillisWithCheckAndHandleEvents(2500);
    drawGui();
}

/*
 * Values are between +10 at 90 degree (canvas top is up) and -10 (canvas bottom is up)
 */
void processVerticalSensorValue(float aSensorValue) {

    int tVerticalServoValue = (aSensorValue + sYBiasValue - sYZeroCompensationValue) * 12;
// overflow handling
    if (tVerticalServoValue > 90) {
        tVerticalServoValue = 90;
    } else if (tVerticalServoValue < -90) {
        tVerticalServoValue = -90;
    }
    ServoVertical.write(tVerticalServoValue + 90);

// Active slider detection
    BDSlider tActiveSlider;
    BDSlider tInactiveSlider;
    if (tVerticalServoValue >= 0) {
        tActiveSlider = SliderUp;
        tInactiveSlider = SliderDown;
    } else {
        tActiveSlider = SliderDown;
        tInactiveSlider = SliderUp;
        tVerticalServoValue = -tVerticalServoValue;
    }

// use this as delay between deactivating one channel and activating the other
    int tSliderValue = (tVerticalServoValue * sSliderHeight) / 90;
    if (sLastSliderVerticalValue != tSliderValue) {
        sLastSliderVerticalValue = tSliderValue;
        tActiveSlider.setValueAndDrawBar(tSliderValue);
        tInactiveSlider.setValueAndDrawBar(0);
        if (sLastVerticalValue != tVerticalServoValue) {
            sLastVerticalValue = tVerticalServoValue;
            snprintf(sStringBuffer, sizeof(sStringBuffer), "%3d", tVerticalServoValue);
            SliderDown.printValue(sStringBuffer);
        }
    }
}

/*
 * Values are between +10 at 90 degree (canvas right is up) and -10 (canvas left is up)
 */
void processHorizontalSensorValue(float aSensorValue) {

    int tHorizontalValue = (aSensorValue + sXBiasValue - sXZeroCompensationValue) * 18;
// overflow handling
    if (tHorizontalValue > 90) {
        tHorizontalValue = 90;
    } else if (tHorizontalValue < -90) {
        tHorizontalValue = -90;
    }
    ServoHorizontal.write(tHorizontalValue + 90);

// left right handling
    BDSlider tActiveSlider;
    BDSlider tInactiveSlider;
    if (tHorizontalValue >= 0) {
        tActiveSlider = SliderLeft;
        tInactiveSlider = SliderRight;
    } else {
        tActiveSlider = SliderRight;
        tInactiveSlider = SliderLeft;
        tHorizontalValue = -tHorizontalValue;
    }

// scale value for full scale =SLIDER_WIDTH at at 30 degree
    tHorizontalValue = (tHorizontalValue * sSliderHeight) / 90;

// dead band handling for slider
    if (tHorizontalValue > VALUE_X_SLIDER_DEAD_BAND) {
        tHorizontalValue = tHorizontalValue - VALUE_X_SLIDER_DEAD_BAND;
    } else {
        tHorizontalValue = 0;
    }

// use this as delay between deactivating one pin and activating the other
    if (sLastLeftRightValue != tHorizontalValue) {
        sLastLeftRightValue = tHorizontalValue;
        tActiveSlider.setValueAndDrawBar(tHorizontalValue);
        tInactiveSlider.setValueAndDrawBar(0);
    }
}

/*
 * Sensor callback handler
 */
void doSensorChange(uint8_t aSensorType, struct SensorCallback *aSensorCallbackInfo) {
    static float sXZeroValueAdded;
    static float sYZeroValueAdded;
    if (tSensorChangeCallCount < CALLS_FOR_ZERO_ADJUSTMENT) {
        if (tSensorChangeCallCount == 0) {
            // init values
            sXZeroValueAdded = 0;
            sYZeroValueAdded = 0;
        }
        // Add for zero adjustment
        sXZeroValueAdded += aSensorCallbackInfo->ValueX;
        sYZeroValueAdded += aSensorCallbackInfo->ValueY;
    } else if (tSensorChangeCallCount == CALLS_FOR_ZERO_ADJUSTMENT) {
        // compute and set zero value
        sXZeroCompensationValue = sXZeroValueAdded / CALLS_FOR_ZERO_ADJUSTMENT;
        sYZeroCompensationValue = sYZeroValueAdded / CALLS_FOR_ZERO_ADJUSTMENT;
        sXBiasValue = 0;
        sYBiasValue = 0;
        BlueDisplay1.playTone(24);
    } else {
        /*
         * regular operation here!
         */
        tSensorChangeCallCount = CALLS_FOR_ZERO_ADJUSTMENT + 1; // to prevent overflow
#if defined(DEBUG)
        void printSensorInfo(struct SensorCallback* aSensorCallbackInfo) {
            dtostrf(aSensorCallbackInfo->ValueX, 7, 4, &sStringBuffer[50]);
            dtostrf(aSensorCallbackInfo->ValueY, 7, 4, &sStringBuffer[60]);
            dtostrf(aSensorCallbackInfo->ValueZ, 7, 4, &sStringBuffer[70]);
            dtostrf(sYZeroCompensationValue, 7, 4, &sStringBuffer[80]);
        #pragma GCC diagnostic ignored "-Wformat-truncation=" // We know, each argument is a string of size 7
            snprintf(sStringBuffer, sizeof sStringBuffer, "X=%s Y=%s Z=%s Zero=%s", &sStringBuffer[50], &sStringBuffer[60],
                    &sStringBuffer[70], &sStringBuffer[80]);
            BlueDisplay1.drawText(0, sTextSize, sStringBuffer, sTextSize, COLOR16_BLACK, COLOR16_GREEN);
        }
#endif
        if (sDoSensorMove) {
            processVerticalSensorValue(aSensorCallbackInfo->ValueY);
            processHorizontalSensorValue(aSensorCallbackInfo->ValueX);
            sLastSensorXValue = aSensorCallbackInfo->ValueX;
            sLastSensorYValue = aSensorCallbackInfo->ValueY;
        }
    }
    sMillisOfLastReveivedEvent = millis();
    tSensorChangeCallCount++;
}

