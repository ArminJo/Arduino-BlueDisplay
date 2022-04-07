/*
 * BTModuleProgrammer.cpp
 * Program for easy changing name of HC-05 or JDY-31 Bluetooth modules and to set baudrate to 115200.
 * It can also be used to enter AT commands directly to the BT module for extended manual programming.
 *
 * The baudrate is specified on line 87 and 88, change these lines if you need.
 *
 * Serial is used for connection with host.
 * SoftwareSerial is used for connection with BT module.
 * The Bluetooth module is connected at Pin 2 (Arduino RX - BT TX), 2 (Arduino TX - BT RX).
 * Switch to JDY-31 programming connecting pin 4 to ground.
 *
 * 1. Load this sketch onto your Arduino.
 * 2. Disconnect Arduino from power.
 * 3. Connect Arduino rx/tx with HC-05 module tx/rx (crossover!) and do not forget to attach 5 volt to the module.
 * 4. If you have a HC-05 module, connect key pin with 3.3 volt output of Arduino to enter program mode.
 *    On my kind of board (a single sided one) it is sufficient to press the tiny button while powering up.
 *    If the module is in program mode, it blinks 4 seconds on and 4 seconds off.
 * 5. Apply power to Arduino and module.
 * 6. Open the Serial Monitor and follow the instructions.
 *
 * Sample Serial Monitor outputs can be found at https://github.com/ArminJo/Arduino-BlueDisplay/tree/master/examples/BTModuleProgrammer.
 *
 * If you see " ... stk500_getsync(): not in sync .." while reprogramming the Arduino with this program
 * it may help to disconnect the Arduino RX from the HC-05 module TX pin temporarily.
 *
 *
 *  Copyright (C) 2014-2022  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay.
 *  BlueDisplay is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 */

#include <Arduino.h>

#include <SoftwareSerial.h>

SoftwareSerial BTModuleSerial(2, 3); // RX, TX - RX data is not reliable at 115200 baud if millis interrupt is not disabled

#define VERSION_EXAMPLE "3.1"

// To enable JDY-31 programming, connect pin D4 to ground (or to pin D5)
#define JDY_31_SELECT_PIN   4
/*
 * Baud rates supported by the HC-05 module
 */
#define BAUD_STRING_4800      "4800"
#define BAUD_STRING_9600      "9600"
#define BAUD_STRING_19200    "19200"
#define BAUD_STRING_38400    "38400"
#define BAUD_STRING_57600    "57600"
#define BAUD_STRING_115200  "115200"
#define BAUD_STRING_230400  "230400"
#define BAUD_STRING_460800  "460800"
#define BAUD_STRING_921600  "921600"
#define BAUD_STRING_1382400 "1382400"

#define BAUD_4800         4800
#define BAUD_9600         9600
#define BAUD_19200       19200
#define BAUD_38400       38400
#define BAUD_57600       57600
#define BAUD_115200     115200
#define BAUD_230400     230400
#define BAUD_460800     460800
#define BAUD_921600     921600
#define BAUD_1382400   1382400

#define BAUD_JDY31_STRING_9600  "4"
#define BAUD_JDY31_STRING_19200 "5"
#define BAUD_JDY31_STRING_38400 "6"
#define BAUD_JDY31_STRING_57600 "7"
#define BAUD_JDY31_STRING_115200 "8"

/************************************
 ** MODIFY THESE VALUES IF YOU NEED **
 ************************************/
#define MY_HC05_BAUDRATE_STRING     BAUD_STRING_115200
//#define MY_JDY31_BAUDRATE           BAUD_JDY31_STRING_115200
//#define MY_JDY31_BAUDRATE_STRING    BAUD_STRING_115200
#define MY_JDY31_BAUDRATE           BAUD_JDY31_STRING_9600
#define MY_JDY31_BAUDRATE_STRING    BAUD_STRING_9600

char StringBufferForModuleName[] = "AT+NAME=                    ";
#define INDEX_OF_HC05_NAME_IN_BUFFER    8
#define INDEX_OF_JDY31_NAME_IN_BUFFER   7

char StringBuffer[64];
uint16_t readModuleResponseToBuffer(char *aStringBufferPtr);
uint16_t sendWaitAndReceive(const char *aATString);
void waitAndEmptySerialReceiveBuffer(uint16_t aDelayMillis);
uint16_t readStringWithTimeoutFromSerial(char *aStringBufferPtr, uint16_t aTimeoutSeconds);
void delayMilliseconds(unsigned int aMillis);

void doProgramModules();
bool setupHC_05();
bool setupJDY_31();

/*
 * Helper macro for getting a macro definition as string
 */
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

void setup() {
    // initialize the digital pin as an output.
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(115200);
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/|| defined(SERIALUSB_PID) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#endif
    // Just to know which program is running on my Arduino
    Serial.println(F("START " __FILE__ "\r\nVersion " VERSION_EXAMPLE " from " __DATE__));

    pinMode(JDY_31_SELECT_PIN, INPUT_PULLUP);
    pinMode((JDY_31_SELECT_PIN + 1), OUTPUT); // to be able to connect pin 4 to pin 5 on a breadboard
    digitalWrite((JDY_31_SELECT_PIN + 1), LOW);

    Serial.println();
    if (digitalRead(JDY_31_SELECT_PIN) == LOW) {
        Serial.println(F("*********************************"));
        Serial.println(F("JDY-31 programming mode detected."));
        Serial.println(F("*********************************"));
        Serial.println(F("Switch to HC-05 mode by disconnecting pin 4 from ground or from pin 5."));
    } else {
        // Time to release key button for HC-05
        Serial.println(F("********************************"));
        Serial.println(F("HC-05 programming mode detected."));
        Serial.println(F("********************************"));
        Serial.println(F("Switch to JDY-31 mode by connecting pin 4 to ground or to pin 5."));
        Serial.println();
        Serial.println(F("Now you have 5 seconds for releasing HC-05 module key, "
                "which should be pressed at power up to enter program mode."));
        delay(5000);
    }
    Serial.println();
    Serial.println(F("Now we try to connect to module, read version and baud and wait for new name to be entered."));
    Serial.println();

    /*
     * Let the built-in LED blink once
     */
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(300);

    doProgramModules();

    delay(1000);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    delay(300);
}

/*
 * The loop implements manual AT mode
 */
void loop() {
    if (Serial.available()) {
#if defined(TIMSK0)
        // enable millis interrupt
        _SFR_BYTE(TIMSK0) |= _BV(TOIE0);
#endif
        // read AT command and send it to the module
        uint8_t tLength = readStringWithTimeoutFromSerial(StringBuffer, 1);
//        Serial.print(F("Length="));
//        Serial.print(tLength);
//        Serial.print(F(" Command="));
//        Serial.println(StringBuffer);
        if (tLength >= 2 && StringBuffer[0] == 'A' && StringBuffer[1] == 'T') {
            Serial.println(F("Send manual AT command now."));
            sendWaitAndReceive(StringBuffer);
        } else {
            Serial.print(F("Command \""));
            Serial.print(StringBuffer);
            Serial.println(F("\" does not start with \"AT\""));
        }
        waitAndEmptySerialReceiveBuffer(3); // skip 3 character at 9600
    }

    delayMilliseconds(300);
}

void waitAndEmptySerialReceiveBuffer(uint16_t aDelayMillis) {
    delay(aDelayMillis);
    while (Serial.available()) {
        Serial.read();
    }
}

/*
 * @return - length of string received, 0 if timeout happened.
 */
uint16_t readStringWithTimeoutFromSerial(char *aStringBufferPtr, uint16_t aTimeoutSeconds) {
    unsigned long tMillisStart = millis();
    unsigned long tTimeoutMillis = aTimeoutSeconds * 1000;
    uint16_t tStringLength = 0;

    do {
        if (Serial.available()) {
            char tChar = Serial.read();
            if (tChar == '\r' || tChar == '\n') {
                /*
                 * End of string read -> cancel timeout
                 */
                *aStringBufferPtr = '\0';
                break;
            }
            *aStringBufferPtr++ = tChar;
            tStringLength++;
        }
    } while (millis() < tMillisStart + tTimeoutMillis); // overflow proof
    return tStringLength;
}

void doProgramModules() {

    bool tDoInitJDY31 = (digitalRead(JDY_31_SELECT_PIN) == LOW);
    bool hasSuccess = false;

    if (tDoInitJDY31) {
        Serial.println(F("JDY-31 module selected.\r\n"));
        BTModuleSerial.begin(BAUD_9600); // SPP-C default speed at delivery
        Serial.println(F("Start with baudrate 9600 for JDY-31 - factory default"));
        hasSuccess = setupJDY_31();
    } else {
        Serial.println(F("HC-05 module selected.\r\n"));
        BTModuleSerial.begin(BAUD_38400); // HC-05 default speed in AT command mode
        Serial.println(F("Start with baudrate 38400 for HC-05 - factory default for AT command mode"));
        hasSuccess = setupHC_05();
    }

    Serial.println();
    Serial.print(F("Programming "));
    if (!hasSuccess) {
        Serial.print(F("skipped or not"));
    }

    Serial.println(F(" successful. You may now:"));
    Serial.println(F("- Press reset for a new try."));
    Serial.println(F("- Enter \"AT+<Command>\"."));
    Serial.println();
    waitAndEmptySerialReceiveBuffer(1); // dummy wait 1 ms
}

void delayMilliseconds(unsigned int aMillis) {
    for (unsigned int i = 0; i < aMillis; ++i) {
        delayMicroseconds(1000);
    }
}

bool checkForOK(uint8_t aReturnedBytes) {
    return (aReturnedBytes == 4 && StringBuffer[0] == 'O' && StringBuffer[1] == 'K');
}

bool setupHC_05() {
    Serial.println(F("Setup HC-05 module."));
    int tReturnedBytes = sendWaitAndReceive("AT");

    /*
     * Check if "OK\n\r" returned
     */
    if (checkForOK(tReturnedBytes)) {

        Serial.println(F("Module attached OK."));
        Serial.println();
        Serial.println(F("Get version"));
        sendWaitAndReceive("AT+VERSION");

        Serial.println();
        Serial.println(F("Get current baud"));
        sendWaitAndReceive("AT+UART");

        Serial.println();
        Serial.println(F("Get current name"));
        sendWaitAndReceive("AT+NAME");

        Serial.println();
        Serial.println(F("Get current PIN"));
        sendWaitAndReceive("AT+PSWD");

        Serial.println();
        Serial.println(F("Get current Role (0->Slave, 1->Master, 2->Slave-Loop)"));
        sendWaitAndReceive("AT+ROLE");

        Serial.println();
        Serial.println(F("Get current Cmode (0->connect to fixed bind address, 1->connect to all)"));
        sendWaitAndReceive("AT+CMODE");

        Serial.println();
        Serial.println(F("Get Bind address"));
        sendWaitAndReceive("AT+BIND");

        Serial.println();
        Serial.println(F("Get own Address"));
        sendWaitAndReceive("AT+ADDR");
        Serial.println();

        Serial.println(
                F(
                        "Enter new module name to set this name and to set baudrate to " MY_HC05_BAUDRATE_STRING " - you will be asked for confirmation."));
        Serial.println(F("Or enter empty string to skip (and enter direct AT mode)."));
        Serial.println(F("Factory reset command is \"AT+ORGL\"."));
        Serial.println(F("Timeout is 60 seconds."));
        Serial.println();
        waitAndEmptySerialReceiveBuffer(3); // 3 ms is sufficient for reading 3 character at 9600
        uint8_t tLength = readStringWithTimeoutFromSerial(&StringBufferForModuleName[INDEX_OF_HC05_NAME_IN_BUFFER], 60);
        if (tLength > 0) {
            Serial.println();
            Serial.print(F("Confirm setting to factory reset and setting name of the module to \""));
            Serial.print(&StringBufferForModuleName[INDEX_OF_HC05_NAME_IN_BUFFER]);
            Serial.println(F("\" and baudrate to " MY_HC05_BAUDRATE_STRING));
            Serial.println(F("by entering any character or press reset or remove power to cancel."));
            Serial.println();
            waitAndEmptySerialReceiveBuffer(3); // read 3 character at 9600

            while (!Serial.available()) {
                delay(1);
            }

            /**
             * program HC05 Module
             */
//            // reset to original state. This leaves programming mode for Version 3.0-20170601
//            Serial.println(F("Reset to default"));
//            sendWaitAndReceive("AT+ORGL");
//            Serial.println();
            // Test comminication
            int tReturnedBytes = sendWaitAndReceive("AT");
            if (!checkForOK(tReturnedBytes)) {
                return false;
            }

            // Set name
            Serial.print(F("Set name to \""));
            Serial.print(&StringBufferForModuleName[INDEX_OF_HC05_NAME_IN_BUFFER]);
            Serial.println('"');
            tReturnedBytes = sendWaitAndReceive(StringBufferForModuleName);
            Serial.println();
            if (!checkForOK(tReturnedBytes)) {
                return false;
            }

            // Set baud / 1 stop bit / no parity
            Serial.println(F("Set baud to " MY_HC05_BAUDRATE_STRING));
            tReturnedBytes = sendWaitAndReceive("AT+UART=" MY_HC05_BAUDRATE_STRING ",0,0");
            if (!checkForOK(tReturnedBytes)) {
                return false;
            }

            Serial.println();
            Serial.println(F("Successful programmed HC-05 module."));
            waitAndEmptySerialReceiveBuffer(1);

            return true;
        }
    } else {
        Serial.println(F("No valid response to AT command. Program mode must be enabled to get a response!"));
    }
    return false;
}

bool setupJDY_31() {
    Serial.println(F("Setup JDY module."));
// must start with sending "AT", sending "AT+BAUD" at start gives no response.
    sendWaitAndReceive("AT");
    int tReturnedBytes = sendWaitAndReceive("AT+BAUD");
    if (tReturnedBytes != 9) {
        /*
         * Try another baud rate, since the module starts at the last programmed baud rate
         */
        Serial.println();
        Serial.println(F("No valid response, try 115200 baud."));
        BTModuleSerial.begin(BAUD_115200);
        sendWaitAndReceive("AT");
        tReturnedBytes = sendWaitAndReceive("AT+BAUD");
    }
    if (tReturnedBytes == 9 && StringBuffer[0] == '+' && StringBuffer[1] == 'B') {

        Serial.println(F("Module attached, get version"));
        sendWaitAndReceive("AT+VERSION");

        Serial.println(F("Get current name"));
        sendWaitAndReceive("AT+NAME");
        Serial.println();

        Serial.println(
                F(
                        "Enter new module name to factory reset and set name and set baudrate to " MY_JDY31_BAUDRATE " - you will be asked for confirmation."));
        Serial.println(F("Or enter empty string to skip (and enter direct AT mode)."));
        Serial.println(F("Timeout is 60 seconds."));
        Serial.println();
        waitAndEmptySerialReceiveBuffer(3); // 3 ms is sufficient for reading 3 character at 9600
        uint8_t tLength = readStringWithTimeoutFromSerial(&StringBufferForModuleName[INDEX_OF_JDY31_NAME_IN_BUFFER], 60);
        if (tLength > 0) {
            Serial.println();
            Serial.print(F("Enter any character to set name of the module to "));
            Serial.print(&StringBufferForModuleName[INDEX_OF_JDY31_NAME_IN_BUFFER]);
            Serial.println(F(" and set baudrate to 115200 or press reset or remove power."));
            waitAndEmptySerialReceiveBuffer(3); // read 3 character at 9600

            while (!Serial.available()) {
                delay(1);
            }

            /**
             * program JDY Module
             * The module is automatically in command (AT) mode when not paired,
             * and it enters the serial emulation mode automatically when paired.
             */
            // reset to original state
            Serial.println();
            Serial.println(F("Reset module to default"));
            sendWaitAndReceive("AT+DEFAULT");

            BTModuleSerial.begin(BAUD_9600);
            Serial.println(F("Set communication to 9600 baud."));
            delay(300);

            // Set name
            Serial.print(F("Set name to \""));
            Serial.print(&StringBufferForModuleName[INDEX_OF_JDY31_NAME_IN_BUFFER]);
            Serial.println('"');

            sendWaitAndReceive(StringBufferForModuleName);

            // Set baud
            Serial.println(F("Set baud to " MY_JDY31_BAUDRATE));
            sendWaitAndReceive("AT+BAUD" MY_JDY31_BAUDRATE);

            BTModuleSerial.begin(BAUD_115200);
            Serial.println(F("Set communication to 115200 baud."));
            delay(300);

            Serial.println(F("Get new name"));
            sendWaitAndReceive("AT+NAME");

            Serial.println(F("Successful programmed JDY module."));
            return true;
        }
    } else {
        Serial.println(F("No valid response to AT command."));
    }
    return false;
}

/*
 * Try for 100 milliseconds to read input from software serial.
 * millis() interrupt is disabled here!
 */
uint16_t readModuleResponseToBuffer(char *aStringBufferPtr) {
    uint16_t tDelayCount = 0;
    uint16_t tReturnedBytes = 0;
// wait for 300 milliseconds to read input
    while (tDelayCount < 500) {
        int tReturnedBytesPerRead = BTModuleSerial.available();
        if (tReturnedBytesPerRead > 0) {
            /*
             * response received -> reset delay count and read data to buffer
             */
            tDelayCount = 0;
            tReturnedBytes += tReturnedBytesPerRead;
            for (uint_fast8_t i = 0; i < tReturnedBytesPerRead; ++i) {
                char tChar = BTModuleSerial.read();
                /*
                 * Convert special character
                 */
                if (tChar >= ' ') {
                    *aStringBufferPtr++ = tChar;
                } else if (tChar == '\r') {
                    *aStringBufferPtr++ = '\\';
                    *aStringBufferPtr++ = 'r';
                } else if (tChar == '\n') {
                    *aStringBufferPtr++ = '\\';
                    *aStringBufferPtr++ = 'n';
                }
                /*
                 * Let space for 2 chars
                 */
                while (aStringBufferPtr > &StringBuffer[(sizeof(StringBuffer) - 2)]) {
                    aStringBufferPtr--;
                }
            }
        }
        delayMicroseconds(1000);
        tDelayCount++;
    }
    *aStringBufferPtr = '\0';

    return tReturnedBytes;
}

uint16_t sendWaitAndReceive(const char *aATString) {
    Serial.print(F("Sent: \""));
    Serial.print(aATString);
    Serial.println(F("\\r\\n\""));
    Serial.flush(); // required in order not to disturb SoftwareSerial

#if defined(TIMSK0)
    // disable millis interrupt
    _SFR_BYTE(TIMSK0) &= ~_BV(TOIE0);
#endif
    BTModuleSerial.println(aATString);
    /*
     * Around 1 ms for sending the command at 115200
     * For JDY-31 at115200 I measured 1 ms delay between end of the command and start of the answer
     */
    uint16_t tReturnedBytes = readModuleResponseToBuffer(StringBuffer);
#if defined(TIMSK0)
// enable millis interrupt
    _SFR_BYTE(TIMSK0) |= _BV(TOIE0);
#endif

    Serial.print(F("Received: \""));
    Serial.print(StringBuffer);
    Serial.println('"');
    return tReturnedBytes;

}
