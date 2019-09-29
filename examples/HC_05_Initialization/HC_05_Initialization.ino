/*
 * HC-05Initialization.cpp
 *
 * Simple helper program to reconfigure your HC-05 module.
 *
 * 0. Modify the lines 71 and 72.
 * 1. Load this sketch to your Arduino.
 * 2. Disconnect Arduino from power.
 * 3. Connect Arduino rx/tx with HC-05 module tx/rx (crossover!) and do not forget to attach 5 Volt to the module.
 * 4. Connect key pin of HC-05 module with 3.3 Volt line (of Arduino).
 *    On my kind of board (a single sided one) it is sufficient to press the tiny button while powering up.
 *    If the module is in programming state, it blinks 4 seconds on and 4 seconds off.
 * 5. Apply power to Arduino and module.
 * 6. Wait for build in LED to blink continuously (This could take up to 10 seconds).
 *    If it only blinks twice (after 8 seconds for boot flickering), check your wiring or just try it again at step 5 or 4.
 *    You can see also messages on the serial monitor with 38400 Baud.
 * 7. Disconnect Arduino and module from power.
 * 8. If connected, disconnect key pin of HC-05 module from 3.3 Volt line of Arduino.
 * 9. Congratulation, you're done.
 *
 * If you see " ... stk500_getsync(): not in sync .." while reprogramming the Arduino
 * it may help to disconnect the Arduino RX from the Hc-05 module TX pin temporarily.
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
 */

#include <Arduino.h>

#include <SoftwareSerial.h>

SoftwareSerial BTModuleSerial(3, 2); // RX, TX - RX data is not very reliable at 115200 baud

#define VERSION_EXAMPLE "2.0"

// remove comment to enable JDY programming OR connect pin D2 to ground
//#define JDY_31_MODULE
#define SPP_C_DETECT_PIN 4
/*
 * Baud rates supported by the HC-05 module
 */
#define BAUD_STRING_4800 "4800"
#define BAUD_STRING_9600 "9600"
#define BAUD_STRING_19200 "19200"
#define BAUD_STRING_38400 "38400"
#define BAUD_STRING_57600 "57600"
#define BAUD_STRING_115200 "115200"
#define BAUD_STRING_230400 "230400"
#define BAUD_STRING_460800 "460800"
#define BAUD_STRING_921600 " 921600"
#define BAUD_STRING_1382400 "1382400"

#define BAUD_4800 (4800)
#define BAUD_9600 (9600)
#define BAUD_19200 (19200)
#define BAUD_38400 (38400)
#define BAUD_57600 (57600)
#define BAUD_115200 (115200)
#define BAUD_230400 (230400)
#define BAUD_460800 (460800)
#define BAUD_921600 ( 921600)
#define BAUD_1382400 (1382400)

#define BAUD_JDY31_STRING_9600 "4"
#define BAUD_JDY31_STRING_19200 "5"
#define BAUD_JDY31_STRING_38400 "6"
#define BAUD_JDY31_STRING_57600 "7"
#define BAUD_JDY31_STRING_115200 "8"

/************************************
 ** MODIFY THESE VALUES IF YOU NEED **
 ************************************/
#define MY_HC05_NAME "HC-05-DSO1"
#define MY_HC05_BAUDRATE BAUD_STRING_115200

#define MY_JDY31_NAME "JDY-31-Third"
#define MY_JDY31_BAUDRATE BAUD_JDY31_STRING_115200

int LED = 13;   // LED pin
bool signalSuccess = false;

char StringBuffer[128];
int readResponseToBuffer(char * aStringBuffer);
void initHC_05();
void initJDY_31();

void setup() {
    // initialize the digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
    while (!Serial)
        ; //delay for Leonardo
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ "\r\nVersion " VERSION_EXAMPLE " from " __DATE__));

    pinMode(SPP_C_DETECT_PIN, INPUT_PULLUP);

    delay(3000);
    digitalWrite(LED, HIGH);
    delay(1000);
    digitalWrite(LED, LOW);
    delay(300);

    bool tInitJDY31 = (digitalRead(SPP_C_DETECT_PIN) == LOW);
#ifdef JDY_31_MODULE
    tInitJDY31 = true;
#else
    if (tInitJDY31) {
        /*
         * Must choose right baud rate since trying different baud rates does not work with JDY module :-(
         */
        BTModuleSerial.begin(BAUD_9600);  // SPP-C default speed at delivery
//        BTModuleSerial.begin(BAUD_115200);  // My target baud rate or already programmed modules
        initJDY_31();
    } else {
        BTModuleSerial.begin(BAUD_38400);  // HC-05 default speed in AT command mode
        initHC_05();
    }
#endif

    if (strlen(StringBuffer) > 0) {
        Serial.println(StringBuffer);
    }

    digitalWrite(LED, HIGH);
    delay(1000);
    digitalWrite(LED, LOW);
    delay(300);
}

void loop() {
    digitalWrite(LED, HIGH);
    if (signalSuccess) {
        delay(300);
        digitalWrite(LED, LOW);
        delay(300);
    } else {
        readResponseToBuffer(StringBuffer);
        delay(300);
    }
}

void initHC_05() {
    Serial.println("Initialize HC-05 module.");
    Serial.println("Sending \"AT\" to module...");
    BTModuleSerial.println("AT");
    delay(300);

    int tReturnedBytes = readResponseToBuffer(StringBuffer);

    if (tReturnedBytes > 0) {
        Serial.print("Received: ");
        Serial.println(StringBuffer);
        Serial.println();
    }

    /*
     * Check if "OK\n\r" returned
     */
    if (tReturnedBytes == 4) {
        if (StringBuffer[0] == 'O' && StringBuffer[1] == 'K') {
            /**
             * program HC05 Module
             */

            // reset to original state
            Serial.println("HC-05 module attached, reset to default with \"AT+ORGL\"...");
            BTModuleSerial.println("AT+ORGL");
            delay(300);
            readResponseToBuffer(StringBuffer);
            Serial.print("Received: ");
            Serial.println(StringBuffer);
            Serial.println();

            // Set name
            Serial.println("Set name to \"" MY_HC05_NAME "\" with \"AT+NAME=" MY_HC05_NAME "\"...");
            BTModuleSerial.println("AT+NAME=" MY_HC05_NAME);
            delay(300);
            readResponseToBuffer(StringBuffer);
            Serial.print("Received: ");
            Serial.println(StringBuffer);
            Serial.println();

            // Set baud / 1 stop bit / no parity
            Serial.println("Set baud to " MY_HC05_BAUDRATE " with \"AT+UART=" MY_HC05_BAUDRATE ",0,0\"...");
            BTModuleSerial.println("AT+UART=" MY_HC05_BAUDRATE ",0,0");
            delay(300);
            readResponseToBuffer(StringBuffer);
            Serial.print("Received: ");
            Serial.println(StringBuffer);
            Serial.println();
            signalSuccess = true;
            Serial.println("Successful initialized HC-05 module.");

        }
    } else {
        Serial.print("No response to AT command, # of returned bytes are:");
        Serial.println(tReturnedBytes);
    }
}

void initJDY_31() {
    Serial.println("Initialize JDY module.");
    Serial.println("Sending \"AT+BAUD\" to module...");
    BTModuleSerial.println("AT+BAUD");
    delay(300);

    int tReturnedBytes = readResponseToBuffer(StringBuffer);
    if (tReturnedBytes > 0) {
        Serial.print("Received: ");
        Serial.println(StringBuffer);
    }

    /*
     * This does not work
     */
//    if (tReturnedBytes != 9) {
//        /*
//         * Try another baud rate, since the module starts at the last programmed baud rate
//         */
//        Serial.println("No valid response, try 115200 baud...");
//
//        BTModuleSerial.begin(BAUD_115200);
//        BTModuleSerial.println("0000");
//        BTModuleSerial.println();
//        Serial.println("Resending \"AT+BAUD\" to module...");
//        BTModuleSerial.println("AT+BAUD");
//        delay(300);
//        readResponseToBuffer(StringBuffer);
//        Serial.print("Received: ");
//        Serial.println(StringBuffer);
//
//    }
    if (tReturnedBytes == 9) {
        if (StringBuffer[0] == '+' && StringBuffer[1] == 'B') {
            /**
             * program JDY Module
             */
            // reset to original state
            Serial.println("JDY module attached, reset to default with \"AT+DEFAULT\"...");

            BTModuleSerial.println("AT+DEFAULT");
            delay(300);
            readResponseToBuffer(StringBuffer);
            Serial.print("Received: ");
            Serial.println(StringBuffer);
            Serial.println();

            // Set name
            Serial.println("Set name to \"" MY_JDY31_NAME "\" with \"AT+NAME" MY_JDY31_NAME "\"...");
            BTModuleSerial.println("AT+NAME" MY_JDY31_NAME);
            delay(300);
            readResponseToBuffer(StringBuffer);
            Serial.print("Received: ");
            Serial.println(StringBuffer);
            Serial.println();

            // Set baud
            Serial.println("Set baud to " MY_JDY31_BAUDRATE " with \"AT+BAUD" MY_JDY31_BAUDRATE "\"...");
            BTModuleSerial.println("AT+BAUD" MY_JDY31_BAUDRATE);
            delay(300);
            readResponseToBuffer(StringBuffer);
            Serial.print("Received: ");
            Serial.println(StringBuffer);
            Serial.println();

            signalSuccess = true;
            Serial.println("Successful initialized JDY31 module.");

        }
    } else {
        Serial.print("No response to AT command, # of returned bytes are:");
        Serial.println(tReturnedBytes);
    }
}

int readResponseToBuffer(char * aStringBuffer) {
    int tReturnedBytes = BTModuleSerial.available();
    int i;
    for (i = 0; i < tReturnedBytes; ++i) {
        StringBuffer[i] = BTModuleSerial.read();
    }
    StringBuffer[i + 1] = '\0';

    return tReturnedBytes;
}
