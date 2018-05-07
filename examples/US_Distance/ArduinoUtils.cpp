/*
 *  ArduinoUtils.cpp
 *
 *  - Simple Blink LED function and a non blocking version
 *  - US Sensor (HC-SR04) functions especially non blocking functions using pin change interrupts
 *  - Simple Servo implementation only for pin 9 and 10 using less resources than standard one
 *
 *  Copyright (C) 2016  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.

 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#include <Arduino.h>
#include "ArduinoUtils.h"

/*
 * Function for speedTest
 * calling a function consisting of just __asm__ volatile ("nop"); gives 0 to 1 micro second
 * Use of Serial. makes it incompatible with BlueDisplay library.
 */
//void speedTestWith1kCalls(void (*aFunctionUnderTest)(void)) {
//    uint32_t tMillisStart = millis();
//    for (uint8_t i = 0; i < 100; ++i) {
//        // unroll 10 times
//        aFunctionUnderTest();
//        aFunctionUnderTest();
//        aFunctionUnderTest();
//        aFunctionUnderTest();
//        aFunctionUnderTest();
//        aFunctionUnderTest();
//        aFunctionUnderTest();
//        aFunctionUnderTest();
//        aFunctionUnderTest();
//        aFunctionUnderTest();
//    }
//    uint32_t tMillisNeeded = millis() - tMillisStart;
//    Serial.print(F("Function call takes "));
//    if (tMillisNeeded > 1000000) {
//        Serial.print(tMillisNeeded / 1000);
//        Serial.print(",");
//        Serial.print((tMillisNeeded % 1000) / 100);
//        Serial.print(F(" milli"));
//    } else {
//        Serial.print(tMillisNeeded);
//        Serial.print(F(" micro"));
//    }
//    Serial.println(F(" seconds."));
//}

int8_t checkAndTruncateParamValue(int8_t aParam, int8_t aParamMax, int8_t aParamMin) {
    if (aParam > aParamMax) {
        aParam = aParamMax;
    } else if (aParam < aParamMin) {
        aParam = aParamMin;
    }
    return aParam;
}

/*
 * Simple blinkLed function
 */
void blinkLed(uint8_t aLedPin, uint8_t aNumberOfBlinks, uint16_t aBlinkDelay) {
    for (int i = 0; i < aNumberOfBlinks; i++) {
        digitalWrite(aLedPin, HIGH);
        delay(aBlinkDelay);
        digitalWrite(aLedPin, LOW);
        delay(aBlinkDelay);
    }
}

/*
 * Non blocking version of blinkLed
 */
static struct nonBlockingBlinkState {
    uint8_t sLedPin;
    uint8_t sNextLedState;
    int sNumberOfBlinks;
    int sBlinkDelay;
    unsigned long sMillisOfNextBlinkAction;
} sNonBlockingBlinkState;

void startBlinkLedNonBlocking(int aLedPin, int aNumberOfBlinks, int aBlinkDelay) {
    sNonBlockingBlinkState.sLedPin = aLedPin;
    sNonBlockingBlinkState.sNumberOfBlinks = aNumberOfBlinks;
    sNonBlockingBlinkState.sBlinkDelay = aBlinkDelay;
    digitalWrite(aLedPin, HIGH);
    sNonBlockingBlinkState.sNextLedState = LOW;
    sNonBlockingBlinkState.sMillisOfNextBlinkAction = millis() + aBlinkDelay;
}

bool checkForLedBlinkUpdate(void) {
    if (sNonBlockingBlinkState.sNumberOfBlinks == 0) {
        return false;
    }
    unsigned long tMillis = millis();
    /*
     * Check if time of next action is reached
     */
    if (sNonBlockingBlinkState.sMillisOfNextBlinkAction <= tMillis) {
        digitalWrite(sNonBlockingBlinkState.sLedPin, sNonBlockingBlinkState.sNextLedState);
        if (sNonBlockingBlinkState.sNextLedState == LOW) {
            sNonBlockingBlinkState.sNumberOfBlinks--;
            /*
             * Check for end
             */
            if (sNonBlockingBlinkState.sNumberOfBlinks == 0) {
                return false;
            } else {
                sNonBlockingBlinkState.sNextLedState = HIGH;
            }
        } else {
            sNonBlockingBlinkState.sNextLedState = LOW;
        }
        sNonBlockingBlinkState.sMillisOfNextBlinkAction = tMillis + sNonBlockingBlinkState.sBlinkDelay;
    }
    return true;
}

void stopBlinkLed(void) {
    sNonBlockingBlinkState.sNumberOfBlinks = 0;
    digitalWrite(sNonBlockingBlinkState.sLedPin, LOW);
}

/********************************************
 * US SENSOR STUFF - HC SR04
 *******************************************/

// must not be constant, since then we get an undefined reference error at link time
extern uint8_t TRIGGER_OUT_PIN;
extern uint8_t ECHO_IN_PIN;

// Outcomment the line according to the ECHO_IN_PIN if using the non blocking version
//#define USE_PIN_CHANGE_INTERRUPT_D0_TO_D7  // using PCINT2_vect - PORT D
//#define USE_PIN_CHANGE_INTERRUPT_D8_TO_D13 // using PCINT0_vect - PORT B - Pin 13 is feedback output
//#define USE_PIN_CHANGE_INTERRUPT_A0_TO_A5  // using PCINT1_vect - PORT C

/*
 * This version only blocks for ca. 12 microseconds for code + generation of trigger pulse
 * Be sure to have the right interrupt vector below
 */
#if (defined(USE_PIN_CHANGE_INTERRUPT_D0_TO_D7) | defined(USE_PIN_CHANGE_INTERRUPT_D8_TO_D13) | defined(USE_PIN_CHANGE_INTERRUPT_A0_TO_A5))
volatile bool sUSValueIsValid = false;
unsigned long sMicrosAtStartOfPulse;
uint16_t sTimeoutMicros;
volatile unsigned long sMicrosOfPulse;

// common code for all interrupt handler
void handlePCInterrupt(uint8_t atPortState) {
    if (atPortState > 0) {
        // start of pulse
        sMicrosAtStartOfPulse = micros();
    } else {
        // end of pulse
        sMicrosOfPulse = micros() - sMicrosAtStartOfPulse;
        sUSValueIsValid = true;
    }
    // echo to output 13
    digitalWrite(13, atPortState);
}
#endif

#if defined(USE_PIN_CHANGE_INTERRUPT_D0_TO_D7)
/*
 * pin change interrupt for D0 to D7 here.
 * state of pin is echoed to output 13 for debugging purpose
 */
ISR (PCINT2_vect) {
    // check pin
    uint8_t tPortState = digitalPinToPort(ECHO_IN_PIN) && bit((digitalPinToPCMSKbit(ECHO_IN_PIN)));
    handlePCInterrupt(tPortState);
}
#endif

#if defined(USE_PIN_CHANGE_INTERRUPT_D8_TO_D13)
/*
 * pin change interrupt for D8 to D13 here.
 * state of pin is echoed to output 13 for debugging purpose
 */
ISR (PCINT0_vect) {
    // check pin
    uint8_t tPortState = digitalPinToPort(ECHO_IN_PIN) && bit((digitalPinToPCMSKbit(ECHO_IN_PIN)));
    handlePCInterrupt(tPortState);
}
#endif

#if defined(USE_PIN_CHANGE_INTERRUPT_A0_TO_A5)
/*
 * pin change interrupt for A0 to A5 here.
 * state of pin is echoed to output 13 for debugging purpose
 */
ISR (PCINT1_vect) {
    // check pin
    uint8_t tPortState = digitalPinToPort(ECHO_IN_PIN) && bit((digitalPinToPCMSKbit(ECHO_IN_PIN)));
    handlePCInterrupt(tPortState);
}
#endif

#if (defined(USE_PIN_CHANGE_INTERRUPT_D0_TO_D7) | defined(USE_PIN_CHANGE_INTERRUPT_D8_TO_D13) | defined(USE_PIN_CHANGE_INTERRUPT_A0_TO_A5))

void getUSDistanceAsCentiMeterWithCentimeterTimeoutNonBlocking(uint8_t aTimeoutCentimeter) {
    // need minimum 10 usec Trigger Pulse
    digitalWrite(TRIGGER_OUT_PIN, HIGH);
    sUSValueIsValid = false;
    sTimeoutMicros = aTimeoutCentimeter * 59;
    *digitalPinToPCMSK(ECHO_IN_PIN) |= bit(digitalPinToPCMSKbit(ECHO_IN_PIN));// enable pin for pin change interrupt
    // net 2 registers exists only once!
    PCICR |= bit(digitalPinToPCICRbit(ECHO_IN_PIN));// enable interrupt for the group
    PCIFR |= bit(digitalPinToPCICRbit(ECHO_IN_PIN));// clear any outstanding interrupt
    sMicrosOfPulse = 0;

#ifdef DEBUG
    delay(2); // to see it on scope
#else
    delayMicroseconds(10);
#endif
    // falling edge starts measurement and generates first interrupt
    digitalWrite(TRIGGER_OUT_PIN, LOW);
}

/*
 * Used to check by polling.
 * If ISR interrupts these code, everything is fine, even if we get a timeout and a no null result
 * since we are interested in the result and not in very exact interpreting of the timeout.
 */
bool isUSDistanceIsMeasureFinished() {
    if (sUSValueIsValid) {
        return true;
    }
    if (micros() - sMicrosAtStartOfPulse >= sTimeoutMicros) {
        // Timeout happened value will be 0
        *digitalPinToPCMSK(ECHO_IN_PIN) &= ~(bit(digitalPinToPCMSKbit(ECHO_IN_PIN)));// disable pin for pin change interrupt
        return true;
    }
    return false;
}
#endif

/*
 * End non blocking implementation
 * Start of standard blocking implementation using pulseIn()
 */

int sLastDistance;
unsigned int getUSDistanceAsCentiMeterWithCentimeterTimeout(unsigned int aTimeoutCentimeter) {
// 58,48 us per centimeter (forth and back)
    // Must be the reciprocal of formula below
    unsigned int tTimeoutMicros = (aTimeoutCentimeter - 1) * 58;
    return getUSDistanceAsCentiMeter(tTimeoutMicros);
}
/*
 * returns aTimeoutMicros if timeout happens
 * timeout of 5850 micros is equivalent to 1m
 */
unsigned int getUSDistanceAsCentiMeter(unsigned int aTimeoutMicros) {
// need minimum 10 usec Trigger Pulse
    digitalWrite(TRIGGER_OUT_PIN, HIGH);
#ifdef DEBUG
    delay(2); // to see it on scope
#else
    delayMicroseconds(50); // 10 micros seems a bit to short. I have modules which have problems with 10us.
#endif
// falling edge starts measurement
    digitalWrite(TRIGGER_OUT_PIN, LOW);

    /*
     * Get echo length. 58,48 us per centimeter (forth and back)
     * => 50cm gives 2900 us, 2m gives 11900 us
     */
    unsigned int tPulseLength = pulseIn(ECHO_IN_PIN, HIGH, aTimeoutMicros);
    if (tPulseLength == 0) {
        // timeout happened
        tPulseLength = aTimeoutMicros;
    }
// +1cm was measured at working device
    unsigned int tDistance = (tPulseLength / 58) + 1;
    sLastDistance = tDistance;
    return tDistance;
}

#if defined (__AVR_ATmega328P__) || defined (__AVR_ATmega328__)
/********************************************
 * SIMPLE SERVO STUFF
 *******************************************/
#define COUNT_FOR_20_MILLIS 40000

/*
 * Variables to enable adjustment for different servo types
 * resolution is 1/2 microseconds, so values are twice the microseconds!!!
 * 1088 and 4800 are values compatible with standard arduino values
 * 1000 and 5200 are values for a SG90 MicroServo
 */
int sServoPulseWidthFor0Degree = 1088;
int sServoPulseWidthFor180Degree = 4800;

/*
 * Use 16 bit timer1 for generating 2 servo signals entirely by hardware without any interrupts.
 * The 2 servo signals are tied to pin 9 and 10 of an 328.
 * Attention - both pins are set to OUTPUT here!
 */
void initSimpleServoPin9_10() {
    /*
     * Periods below 20 ms gives problems with long signals i.e. the positioning is not possible
     */
    DDRB |= _BV(DDB1) | _BV(DDB2);                // set pins OC1A = PortB1 -> PIN 9 and OC1B = PortB2 -> PIN 10 to output direction
    TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11); // FastPWM Mode mode TOP determined by ICR1 - non-inverting Compare Output mode
    TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS11);                // set prescaler to 8, FastPWM Mode mode continued
    ICR1 = COUNT_FOR_20_MILLIS;                // set period to 20 ms
    OCR1A = 3000;                // set count to 1500 us - 90 degree
    OCR1B = 3000;                // set count to 1500 us - 90 degree
    TCNT1 = 0;   // reset timer
}

/*
 * If value is below 180 then assume degree, otherwise assume microseconds
 * if aUpdateFast then enable starting a new output pulse if more than 5 ms since last one, some servo might react faster in this mode.
 */
void setSimpleServoPulse(int aValue, bool aUsePin9, bool aUpdateFast) {
    if (aValue <= 180) {
        // modify and use outcommented line for saving 4 bytes ram and n bytes flash
        //aValue = map(aValue, 0, 180, 1088, 4800);
        aValue = map(aValue, 0, 180, sServoPulseWidthFor0Degree, sServoPulseWidthFor180Degree);
    } else {
        // since the resolution is 1/2 of microsecond
        aValue *= 2;
    }
    if (aUpdateFast) {
        uint16_t tTimerCount = TCNT1;
        if (tTimerCount > 10000) {
            // more than 5 ms since last pulse -> start a new one
            TCNT1 = COUNT_FOR_20_MILLIS - 1;
        }
    }
    if (aUsePin9) {
        OCR1A = aValue;
    } else {
        OCR1B = aValue;
    }
}

/*
 * Set the mapping pulse width values for 0 and 180 degree
 */
void setSimpleServoPulseMicrosFor0And180Degree(int a0DegreeValue, int a180DegreeValue) {
    // *2 since internal values are meant for the 1/2 microseconds resolution of timer
    sServoPulseWidthFor0Degree = a0DegreeValue * 2;
    sServoPulseWidthFor180Degree = a180DegreeValue * 2;
}

/*
 * Pin 9 / Channel A. If value is below 180 then assume degree, otherwise assume microseconds
 */
void setSimpleServoPulsePin9(int aValue) {
    setSimpleServoPulse(aValue, true, true);
}

/*
 * Pin 10 / Channel B
 */
void setSimpleServoPulsePin10(int aValue) {
    setSimpleServoPulse(aValue, false, true);
}
#endif

