# Arduino-Simple-DSO

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Hit Counter](https://hitcounter.pythonanywhere.com/count/tag.svg?url=https%3A%2F%2Fgithub.com%2FArminJo%2FArduino-Simple-DSO)](https://github.com/brentvollebregt/hit-counter)

## SUMMARY
This DSO needs only a standard Arduino-Uno or Arduino-Nano, a HC-05 Bluetooth module a few resistors and capacitators and this software.

| Simple DSO with no attenuator on breadboard | DSO Chart screen |
| :-: | :-: |
| ![DSO with passive attenuator on breadboard](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/pictures/ArduinoDSO_simple.jpg) | ![DSO chart screen](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/pictures/Chart.jpg) |

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
- 3 different types of external attenuator, detected by software.
  - no attenuator (pin 8+9 left open).
  - passive attenuator with /1, /10, /100 attenuation (pin 8 connected to ground).
  - active attenuator (pin 9 connected to ground) - still experimental.
- Using 1.1 volt internal reference. 5 volt (VCC) is also selectable and is useful if no attenuator is attached.

- Integrated frequency generator using 16 bit Timer1. Frequency from 119 mHz (8.388 second) to 8 MHz

- Integrated PWM Waveform generator for sinus, triangle and sawtooth using 16 bit Timer1. Frequency from 1.9 mHz to 7.8 kHz

## Bill of material
1. Arduino Nano
2. Breadboard 400 points
3. Resistors
   - Resistors for the simple (0-5 volt) version: 1x 10 kOhm, 2x 100 k, 1x 4 M or more.
   - Resistors for the 3 range (0-110 volt) version: 1x 2.2 kOhm, 2x 10 k, 3x 100 k, 2x 220 k, 2x 1 M, 1x 4 M or greater
4. Capacitors
   - Capacitors for the simple version: 1x 100 nF / 10 volt (or more)
   - Capacitors for the 3 range (0-110 volt) version: 4x 100 nF / 100 volt (or more), 6.8 uF
5. Jumper wires

Optional for Bluetooth connection
6. HC-05 Bluetooth module
7. Shottky diode e.g. BAT42


# INSTRUCTIONS FOR USE
The DSO software has 4 pages.
1. The start page which shows all the hidden buttons available at the chart page.
2. The chart page which shows the data and info line(s).
3. The settings page.
4. The frequency generator page.

## CHART PAGE
Here you see the current data. This page has two modes, the **acquisition** (measurement running) and the **analyze** (data stored) mode.
The horizontal violet line is the trigger level line and the two light green lines in analyze mode are the maximum and minimum level lines.
On this page you can:
- Start and stop a measurement.
- Start a single one time measurement (**Singleshot**).
- Toggle the history mode.
- Go to the settings page.
- Change **timebase** by **horizontal swiping** in acquisition mode.
- Scroll through the 3 screen data buffer by **horizontal swiping** in analyze mode.
- If manual range selection enabled, change the range by **vertical swiping**.
- Change the info display from short to long to no info by **short touching** on the display. See below for info display reference.
- Display the hidden buttons by doing a **long touch**.
- Touch the **light yellow vertical bar** at the left to enable the voltage picker line (which is drawn destructive).
- If enabled, trigger value can be set by touching the **light violet vertical bar** in the 4. left grid.
If switching info mode, the chart content will be restored.

## SETTINGS PAGE GUI
On this page you have all buttons to modify the **DSO acquisition mode**, to select the different **ADC channels** and for **page navigation**
above the last button row the minimum stack size, the supply voltage and the internal chip temperature is shown.
The stack size is needed for testing different buffer size values during development and the temperature may be quite inaccurate.
- **History** -> **red** history off, **green** history on, i.e. old chart data is not deleted, it stays as a light green trace. This button is also available (invisible) at the chart page.
- Slope - **Slope A** -> trigger on ascending slope, **Slope D** -> trigger on descending slope.
- **Back** -> Back to chart page.
- **Trigger delay** -> Trigger delay can be numerical specified from 4 us to 64.000.000 us (64 seconds, if you really want). Microseconds resolution is used for values below 64.000.
- Trigger - the trigger value can be set on the chart page by touching the light violet vertical bar in the 4. left grid.
  - **Trigger auto** -> let the DSO compute the trigger value using the average of the last measurement.
  - **Trigger man timeout** -> use manual trigger value, but with timeout, i.e. if trigger condition not met, new data is shown after timeout.
      This is not really a manual trigger level mode, but it helps to find the right trigger value.
  -**Trigger man** -> use manual trigger value, but without timeout, i.e. if trigger condition not met, no new data is shown.
  -**Trigger free** means free running trigger, i.e. trigger condition is always met.
  -**Trigger ext** uses pin 2 as external trigger source.

- Input selector
  - **%1**   -> Pin A0 with no attenuator, only a 10k Ohm protection resistor.
  - **%10**  -> Pin A1 with an 1 to 10 attenuator.
  - **%100** -> Pin A2 with an 1 to 100 attenuator.
  - **CH 3** -> sequences through the channels **CH 4**, **Temp** (internal temperature sensor), **VRef** (internal reference).
- **Frequency Generator** -> go to frequency generator page.
- Range - There a 3 ranges, 0-2.5 , 0-5 and 0-10 volt
  - **Range auto** -> let the DSO choose the range based on the minimum and maximum values of the last measurement.
  - **Range man** -> select the range by **vertical swiping** on the chart page.
- Offset
  - **Offset 0V** -> Range starts at 0 volt.
  - **Offset auto** -> For small signals with a high DC offset. If range is not the lowest one and the signal has a DC offset, then a lower range is choosen and the display offset is adapted to show the complete signal
  - **Offset man**
- **DC** / **AC**
  - Since the internal ADC converter has only a DC input, **AC** offsets the inputs DC value to 1/2 reference using Arduino pin A5 and compensates this offset for the display.
    Therefore you must apply the signal using a capacitor!
- Reference voltage
  - **Ref 1.1V** (recommended if having attenuators) -> uses the internal 1.1 volt reference for the ADC.
  - **Ref VCC** -> uses VCC (5 volt supply) as reference for the ADC.

## INFO LINE REFERENCE
### SHORT INFO
- Arithmetic-average and peak to peak voltage of actual chart (In analyze mode, chart is longer than the display!)
- Frequency
- Timebase for div (31 pixel)

## LONG INFO OUTPUT
First line
- Timebase for div (31 pixel)
- Slope of trigger
- Input channel: (0-5), T->AVR-temperature, R->1.1 volt-internal-reference G->internal-ground
- Minimum, arithmetic-average, max and peak to peak voltage of actual chart (In hold mode, chart is longer than display!)
- Trigger level
- Reference used: 5=5 volt,  1=1.1 volt-internal-reference

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
|SINE: clip to minimum 8 samples per period => 128 us / 7812.5 Hz       |7,421 mHz|
|SAWTOOTH: clip to minimum 16 samples per period => 256 us / 3906.25 Hz |3.725 mHz|
|TRIANGLE: clip to minimum 32 samples per period => 512 us / 1953.125 Hz|1.866 mHz|

### RC-Filter suggestions
- Simple: 2k2 Ohm and 100 nF
- 2nd order (good for sine and triangle): 1 kOhm and 100 nF -> 4k7 Ohm and 22 nF
- 2nd order (better for sawtooth):        1 kOhm and 22 nF  -> 4k7 Ohm and 4.7 nF

**Do not run DSO acquisition and non square wave waveform generation at the same time.**
Because of the interrupts at 62 kHz rate, DSO is almost not usable during non square wave waveform generation
and waveform frequency is not stable and decreased, since not all TIMER1 OVERFLOW interrupts are handled.

# SCREENSHOTS
DSO start screen
![DSO start screen](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/pictures/Welcome.jpg)

| DSO Chart screen | DSO Chart screen with long info |
| :-: | :-: |
| ![DSO chart screen](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/pictures/Chart.jpg) | ![DSO chart screen with long info](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/pictures/Chart_Long_Info.jpg) |
| DSO settings menu | DSO frequency / waveform generator menu |
| ![DSO settings menu](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/pictures/Settings_Passive_Attenuator.jpg) | ![Frequency / waveform generator menu](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/pictures/Frequency.jpg) |
| ![DSO at work](https://github.com/ArminJo/android-blue-display/blob/gh-pages/pictures/DSO+Tablet.jpg) |  |

# SCHEMATICS
| SIMPLE 1 RANGE VERSION | 3 RANGE VERSION |
| :-: | :-: |
| ![Fritzing schematic](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/fritzing/Arduino_Nano_DSO_simple_Schaltplan.png) | ![Fritzing schematic](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/fritzing/Arduino_Nano_DSO_BT_full_Schaltplan.png) |
| ![Fritzing breadboard](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/fritzing/Arduino_Nano_DSO_simple_Steckplatine.png) | ![Fritzing breadboard](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/fritzing/Arduino_Nano_DSO_BT_full_Steckplatine.png) |
| ![DSO with passive attenuator on breadboard](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/pictures/ArduinoDSO_simple.jpg) | ![DSO with passive attenuator on breadboard](https://github.com/ArminJo/Arduino-Simple-DSO/blob/master/pictures/ArduinoDSO.jpg) |

# Revision History
### Version 3.2 - 11/2019
- Clear data buffer at start and at switching inputs.
- Multiline button caption.

### Version 3.1
- Stop response improved for fast mode.
- Value computation for ultra fast modes fixed.
- millis() timer compensation formula fixed.
- AC/DC button and info line handling improved.

