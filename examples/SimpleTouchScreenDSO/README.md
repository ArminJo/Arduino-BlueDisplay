# Arduino-Simple-DSO

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

## SUMMARY
This DSO needs only a standard Arduino-Uno or Arduino-Nano, a HC-05 Bluetooth module a few resistors and capacitators and this software.

| Simple DSO with no attenuator on breadboard | DSO Chart screen |
| :-: | :-: |
| ![DSO with passive attenuator on breadboard](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/media/ArduinoDSO_Simple.jpg) | ![DSO chart screen](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/media/Chart.jpg) |

## Features
- 150 kSamples per second with good quality.
- 300 kSamples per second with acceptable quality because of internal ADC limitations.
- Full touch screen control of all parameters.
- AC Measurement supported by using (passive) external attenuator circuit (see below).
- Automatic trigger level, range and offset selection.
- Manual trigger level and range select.
- Trigger delay.
- External trigger.
- 1120 Byte data buffer - 3.5 times display size.
- Display of min, max, average and peak to peak values.
- Display of period and frequency.
- 3 different types of external attenuator detected by software.
  - no attenuator (pin 8+9 left open).
  - passive attenuator with /1, /10, /100 attenuation (pin 8 connected to ground).
  - active attenuator (pin 9 connected to ground) - still experimental.
- Using 1.1 Volt internal reference. 5 Volt (VCC) also selectable which is useful if no attenuator is attached.

- Integrated frequency generator using 16 bit Timer1. Frequency from 119 mHz (8.388 second) to 8MHz

- Integrated PWM Waveform generator for sinus, triangle and sawtooth using 16 bit Timer1. Frequency from 1.9 mHz to 7.8kHz

## Bill of material
1. Arduino Nano
2. HC-05 Bluetooth module
3. Breadboard 400 points
4. Resistors
   - Resistors for the simple (0-5Volt) version: 1x 10k, 2x 100k, 1x 4M or more.
   - Resistors for the 3 range (0-110Volt) version: 1x 2.2k, 2x 10k, 3x 100k, 2x 220k, 2x 1M, 1x 4M or greater
5. Capacitators
   - Capacitators for the simple version: 1x 100nF / 10V (or more)
   - Capacitators for the 3 range (0-110Volt) version: 4x 100nF / 100V (or more), 6.8 uF
6. Shottky diode e.g. BAT42
7. Jumper wires

# DOCUMENTATION

## SHORT INFO OUTPUT
- Arithmetic-average and peak to peak voltage of actual chart (In hold mode, chart is longer than display!)
- Frequency
- Timebase for div (31 pixel)

## LONG INFO OUTPUT
First line
- Timebase for div (31 pixel)
- Slope of trigger
- Input channel: (0-5), T->AVR-temperature, R->1.1Volt-internal-reference G->internal-ground
- Minimum, arithmetic-average, max and peak to peak voltage of actual chart (In hold mode, chart is longer than display!)
- Trigger level
- Reference used: 5=5V 1  1=1.1Volt-internal-reference

Second line
- Frequency
- Period
- first interval  (pulse for slope ascending)
- second interval (pause for slope ascending)

## TOUCH
Short touch switches info output, long touch shows active GUI elements.

## Waveform PWM output

|Maximum values                                                      | Minimum values|
| :--- | :--- |
|SINE: clip to minimum 8 samples per period => 128us / 7812.5Hz       |7,421mHz|
|SAWTOOTH: clip to minimum 16 samples per period => 256us / 3906.25Hz |3.725mHz|
|TRIANGLE: clip to minimum 32 samples per period => 512us / 1953.125Hz|1.866mHz|

### RC-Filter suggestions
- Simple: 2k2 Ohm and 100 nF
- 2nd order (good for sine and triangle): 1 kOhm and 100 nF -> 4k7 Ohm and 22 nF
- 2nd order (better for sawtooth):        1 kOhm and 22 nF  -> 4k7 Ohm and 4.7 nF

**Do not run DSO acquisition and non square wave waveform generation at the same time.**
Because of the interrupts at 62kHz rate, DSO is almost not usable during non square wave waveform generation 
and waveform frequency is not stable and decreased, since not all TIMER1 OVERFLOW interrupts are handled.

# SCREENSHOTS
DSO start screen
![DSO start screen](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/media/Welcome.jpg)

| DSO Chart screen | DSO Chart screen with long info |
| :-: | :-: |
| ![DSO chart screen](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/media/Chart.jpg) | ![DSO chart screen with long info](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/media/Chart_Long_Info.jpg) |
| DSO settings menu | DSO frequency / waveform generator menu |
| ![DSO settings menu](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/media/Settings_Passive_Attenuator.jpg) | ![Frequency / waveform generator menu](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/media/Frequency.jpg) |
| ![DSO at work](https://github.com/ArminJo/android-blue-display/blob/gh-pages/pictures/DSO+Tablet.jpg) |  |

# SCHEMATICS
| SIMPLE 1 RANGE VERSION | 3 RANGE VERSION |
| :-: | :-: |
| ![Fritzing schematic](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/fritzing/Arduino_Nano_DSO_Simple_Schaltplan.png) | ![Fritzing schematic](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/fritzing/Arduino_Nano_DSO_Schaltplan.png) |
| ![Fritzing breadboard](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/fritzing/Arduino_Nano_DSO_Simple_Steckplatine.png) | ![Fritzing breadboard](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/fritzing/Arduino_Nano_DSO_Steckplatine.png) |
| ![DSO with passive attenuator on breadboard](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/media/ArduinoDSO_Simple.jpg) | ![DSO with passive attenuator on breadboard](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/media/ArduinoDSO.jpg) |

# Revision History
### Version 1.0.0
Initial Arduino library version

