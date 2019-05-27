# Arduino-Simple-DSO

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)

This is the eclipse project repository. This DSO-code is also available as an Arduino example sketch under [Arduino-BlueDisplay](https://github.com/ArminJo/Arduino-BlueDisplay).

## SUMMARY
The DSO needs only a standard Arduino-Uno or Arduino-Micro, a HC-05 Bluetooth module and this software.

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
DSO Chart screen
![DSO chart screen](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/media/Chart.jpg)
DSO Chart screen with long info
![DSO chart screen with long info](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/media/Chart_Long_Info.jpg)
DSO settings menu
![DSO settings menu](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/media/Settings_Passive_Attenuator.jpg)
DSO frequency / waveform generator menu
![Frequency / waveform generator menu](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/media/Frequency.jpg)

# SCHEMATICS
Schematic
![Fritzing schematic](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/fritzing/Arduino_Nano_DSO_Schaltplan.png)
Breadboard schematic
![Fritzing breadboard](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/fritzing/Arduino_Nano_DSO_Steckplatine.png)
DSO with passive attenuator on breadboard
![DSO with passive attenuator on breadboard](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/media/ArduinoDSO.jpg)


# SIMPLE VERSION
Simple Schematic
![Fritzing schematic](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/fritzing/Arduino_Nano_DSO_Simple_Schaltplan.png)
Simple Breadboard schematic
![Fritzing breadboard](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/fritzing/Arduino_Nano_DSO_Simple_Steckplatine.png)
Simple DSO with no attenuator on breadboard
![DSO with passive attenuator on breadboard](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/media/ArduinoDSO_Simple.jpg)
