<div align = center>

# SimpleTouchScreenDSO
Simple DSO Software for Arduino Uno/MEGA with a Touchscreen Shield.

[![Badge License: GPLv3](https://img.shields.io/badge/License-GPLv3-brightgreen.svg)](https://www.gnu.org/licenses/gpl-3.0)
 &nbsp; &nbsp; 
[![Badge Version](https://img.shields.io/github/v/release/ArminJo/SimpleTouchScreenDSO?include_prereleases&color=yellow&logo=DocuSign&logoColor=white)](https://github.com/ArminJo/SimpleTouchScreenDSO/releases/latest)
 &nbsp; &nbsp; 
[![Badge Commits since latest](https://img.shields.io/github/commits-since/ArminJo/SimpleTouchScreenDSO/latest?color=yellow)](https://github.com/ArminJo/SimpleTouchScreenDSO/commits/master)
 &nbsp; &nbsp; 
[![Badge Build Status](https://github.com/ArminJo/SimpleTouchScreenDSO/workflows/TestCompile/badge.svg)](https://github.com/ArminJo/SimpleTouchScreenDSO/actions)
 &nbsp; &nbsp; 
![Badge Hit Counter](https://visitor-badge.laobi.icu/badge?page_id=ArminJo_SimpleTouchScreenDSO)
<br/>
<br/>
[![Stand With Ukraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/badges/StandWithUkraine.svg)](https://stand-with-ukraine.pp.ua)

</div>

<br/>

## No dedicated hardware, just off the shelf components + c software

Just add:
- A **piezo buzzer** for audio feedback of touches.
- One **capacitator, 2 resistors and a switch** to have an AC input.
- **few more resistors, capacitors for compensating and a input range select switch** for an attenuator.

## Features:

 - 300 kSamples per second
 - Timebase 1-2-5 from 100 microseconds to 500 milliseconds per div (31 pixel)
 - Automatic trigger,range and offset value selection
 - Single shot mode
 - 580 Byte sample buffer
 - Min, max, average and peak-to-peak voltage display
 - Fast draw mode. Up to 45 screens per second in pixel mode
 - Switching between fast pixel and slower line display mode
 - All settings can be changed during measurement
 - All AVR ADC input channels selectable
 - Touch trigger level select
 - Touch voltage picker
 - 1.1 or 5 Volt reference selectable
 
## Hardware:

 - [Arduino Uno Rev ](https://shop.watterott.com/Arduino-Uno-R3)
 - [mSD-Shield](https://shop.watterott.com/mSD-Shield-v2-microSD-RTC-Datenlogger-Shield)
 - [MI0283QT-2 Adapter](https://shop.watterott.com/MI0283QT-Adapter-v1-inkl-28-LCD-Touchpanel)
 
# Safety circuit range select and AC/DC switch

```
                ADC INPUT PIN
                      /\
                      |                                   VREF----|R 2.2M|---+---|R 2.2M|---GND
                      |                                                      |
                      |                                                      |
            \   o-----+                                                      o\   o
             \        |                                                        \
    AC/DC     \       =  C 0.1uF                                      AC/DC     \
    Switch     \      |                                               Switch     \
                o-----+                                                           o
                      |                                                           |
                      |                                                           |
                      +---|R 1K  |-----<  INPUT DIRECT >------------o   \         |
                      |                                                  \        |
                      |                                                   \       |
                      +---|R 2.2M|-+---<  INPUT 10 VOLT >------o           \      |
                      +---|R 2.2M|-+                                        \     |
                      +------||----+                    Range select switch  \    |
                      |  app. 80 pF (adjustable trimmer)                      o---+------ A0
                      |                                                           |
  ATTENUATOR          +---|R 3.3M|-+---<  INPUT 20 VOLT >------o                  | Safety circuit
                      +------||----+                              VCC(5V)---|<|---+---|<|---GND
                      |  app. 25 pF
                      |
                      +---|R 4.7M|-|R 4.7M|--+---<  INPUT 50 VOLT >-o
                      |                      |
                      +-----------||---------+
                               app. 10 pF
```
![Hardware](https://github.com/ArminJo/SimpleTouchScreenDSO/blob/master/pictures/Hardware.jpg)
![StartScreen](https://github.com/ArminJo/SimpleTouchScreenDSO/blob/master/pictures/StartScreen.jpg)
![Settings](https://github.com/ArminJo/SimpleTouchScreenDSO/blob/master/pictures/Settings.jpg)
![5kHzSine](https://github.com/ArminJo/SimpleTouchScreenDSO/blob/master/pictures/5kHzSine.jpg)
![Linearity-100us / div](https://github.com/ArminJo/SimpleTouchScreenDSO/blob/master/pictures/Linearity-100.JPG)
![Linearity-200us / div](https://github.com/ArminJo/SimpleTouchScreenDSO/blob/master/pictures/Linearity-200.JPG)
![slowTimebase](https://github.com/ArminJo/SimpleTouchScreenDSO/blob/master/pictures/slowTimebase.jpg)
