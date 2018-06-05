#include <avr/sleep.h>
#include <avr/wdt.h>

//void speedTestWith1kCalls(void (*aFunctionUnderTest)(void));

int8_t checkAndTruncateParamValue(int8_t aParam, int8_t aParamMax, int8_t aParamMin);

void blinkLed(uint8_t aLedPin, uint8_t aNumberOfBlinks, uint16_t aBlinkDelay);
void startBlinkLedNonBlocking(int aLedPin, int aNumberOfBlinks, int aBlinkDelay);
bool checkForLedBlinkUpdate(void);
void stopBlinkLed(void);

#define US_DISTANCE_DEFAULT_TIMEOUT 20000
// Timeout of 20000L is 3.4 meter
unsigned int getUSDistanceAsCentiMeter(unsigned int aTimeoutMicros = US_DISTANCE_DEFAULT_TIMEOUT);
unsigned int getUSDistanceAsCentiMeterWithCentimeterTimeout(unsigned int aTimeoutCentimeter);
extern int sUSDistanceCentimeter;
extern volatile unsigned long sUSPulseMicros;

/*
 * Non blocking version
 */
void startUSDistanceAsCentiMeterWithCentimeterTimeoutNonBlocking(unsigned int aTimeoutCentimeter);
bool isUSDistanceMeasureFinished();

/*
 * Simple Servo Library
 * Uses timer1 and Pin 9 + 10 as Output
 */
void initSimpleServoPin9_10();
void setSimpleServoPulseMicrosFor0And180Degree(int a0DegreeValue, int a180DegreeValue);
void setSimpleServoPulse(int aValue, bool aUsePin9, bool aUpdateFast = false);
void setSimpleServoPulsePin9(int aValue); // Channel A
void setSimpleServoPulsePin10(int aValue); // Channel B
