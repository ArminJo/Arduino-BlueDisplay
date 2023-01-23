/**
 * MI0283QT2.hpp
 *
 * Firmware to control a MI0283QT 320 x 240, 2.8" Display with a HX8347D controller
 * https://shop.watterott.com/MI0283QT-Adapter-v1-inkl-28-LCD-Touchpanel
 *
 *  Based on MI0283QT2.cpp  https://github.com/watterott/Arduino-Libs/blob/master/MI0283QT2/MI0283QT2.cpp
 *
 *
 *  Copyright (C) 2012-2023  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay https://github.com/ArminJo/android-blue-display.
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
 */

#include "digitalWriteFast.h"
#include "MI0283QT2.h"

#include "LocalDisplayInterface.hpp"

MI0283QT2 LocalDisplay; // The instance provided by the class itself

#define PRINT_STARTX    (2)
#define PRINT_STARTY    (2)

// SPI addresses
#define LCD_DATA        (0x72)
#define LCD_REGISTER    (0x70)

//#define SOFTWARE_SPI
#ifdef __AVR_ATmega32U4__
#define SOFTWARE_SPI
#endif

#if (defined(__AVR_ATmega1280__) || \
     defined(__AVR_ATmega1281__) || \
     defined(__AVR_ATmega2560__) || \
     defined(__AVR_ATmega2561__))      //--- Arduino Mega ---
#define SOFTWARE_SPI
# define LED_PIN        (9) //PH6: OC2B
# define RST_PIN        (8)
# define MI0283QT2_CS_PIN (7)
# if defined(SOFTWARE_SPI)
#  define MOSI_PIN      (11)
#  define MISO_PIN      (12)
#  define CLK_PIN       (13)
# else
#  define MOSI_PIN      (51)
#  define MISO_PIN      (50)
#  define CLK_PIN       (52)
# endif

#elif (defined(__AVR_ATmega644__) || \
       defined(__AVR_ATmega644P__))    //--- Arduino 644 (www.mafu-foto.de) ---
# define LED_PIN        (3) //PB3: OC0
# define RST_PIN        (12)
# define MI0283QT2_CS_PIN (13)
# define MOSI_PIN       (5)
# define MISO_PIN       (6)
# define CLK_PIN        (7)

#else                                  //--- Arduino Uno ---
# define LED_PIN        (9) //PB1: OC1
#if defined(SUPPORT_HY32D)
# define DC_PIN         (8) // use reset pin for Data/Command
#else
# define RST_PIN        (8)
#endif
# define MI0283QT2_CS_PIN (7)
# define MOSI_PIN       (11)
# define MISO_PIN       (12)
# define CLK_PIN        (13)

#endif

#define LED_ENABLE()    digitalWriteFast(LED_PIN, HIGH)
#define LED_DISABLE()   digitalWriteFast(LED_PIN, LOW)

#if defined(SUPPORT_HY32D)
#define DC_DATA()       digitalWriteFast(DC_PIN, HIGH)
#define DC_COMMAND()    digitalWriteFast(DC_PIN,LOW )
#else
#define RST_DISABLE()   digitalWriteFast(RST_PIN, HIGH)
#define RST_ENABLE()    digitalWriteFast(RST_PIN, LOW)
#endif

#define MI0283QT2_CS_DISABLE()    digitalWriteFast(MI0283QT2_CS_PIN, HIGH)
#define MI0283QT2_CS_ENABLE()     digitalWriteFast(MI0283QT2_CS_PIN, LOW)

#define MOSI_HIGH()     digitalWriteFast(MOSI_PIN, HIGH)
#define MOSI_LOW()      digitalWriteFast(MOSI_PIN, LOW)

#define MISO_READ()     digitalReadFast(MISO_PIN)

#define CLK_HIGH()      digitalWriteFast(CLK_PIN, HIGH)
#define CLK_LOW()       digitalWriteFast(CLK_PIN, LOW)

//-------------------- Constructor --------------------

MI0283QT2::MI0283QT2() {   // @suppress("Class members should be properly initialized")
}

//-------------------- Public --------------------

void MI0283QT2::init(uint8_t aSPIClockDivider) {
    //init pins
    pinMode(LED_PIN, OUTPUT);
    digitalWriteFast(LED_PIN, LOW);
    setBacklightBrightness(BACKLIGHT_START_BRIGHTNESS_VALUE);
#if defined(SUPPORT_HY32D)
    pinMode(DC_PIN, OUTPUT);
    digitalWriteFast(DC_PIN, LOW);
#else
    pinMode(RST_PIN, OUTPUT);
    digitalWriteFast(RST_PIN, LOW);
#endif
    pinMode(MI0283QT2_CS_PIN, OUTPUT);
    digitalWriteFast(MI0283QT2_CS_PIN, HIGH);
    pinMode(CLK_PIN, OUTPUT);
    pinMode(MOSI_PIN, OUTPUT);
    pinMode(MISO_PIN, INPUT);
    digitalWriteFast(MISO_PIN, HIGH);
    //pull-up

#if !defined(SOFTWARE_SPI)
    //SS has to be output or input with pull-up
# if (defined(__AVR_ATmega1280__) || \
      defined(__AVR_ATmega1281__) || \
      defined(__AVR_ATmega2560__) || \
      defined(__AVR_ATmega2561__))     //--- Arduino Mega ---
#  define SS_PORTBIT (0) //PB0
# elif (defined(__AVR_ATmega644__) || \
        defined(__AVR_ATmega644P__))   //--- Arduino 644 ---
#  define SS_PORTBIT (4) //PB4
# else                                 //--- Arduino Uno ---
#  define SS_PORTBIT (2) //PB2
# endif
    if (!(DDRB & (1 << SS_PORTBIT))) //SS is input
    {
        PORTB |= (1 << SS_PORTBIT); //pull-up on
    }

    delay(1); // Is required, otherwise the SPI does not start after power up, only after reset
    //init hardware spi
    switch (aSPIClockDivider) {
    case 2:
        SPCR = (1 << SPE) | (1 << MSTR); //enable SPI, Master, clk=Fcpu/4
        SPSR = (1 << SPI2X); //clk*2 = Fcpu/2
        break;
    case 4:
        SPCR = (1 << SPE) | (1 << MSTR); //enable SPI, Master, clk=Fcpu/4
        SPSR = (0 << SPI2X); //clk*2 = off
        break;
    case 8:
        SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0); //enable SPI, Master, clk=Fcpu/16
        SPSR = (1 << SPI2X); //clk*2 = Fcpu/8
        break;
    case 16:
        SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0); //enable SPI, Master, clk=Fcpu/16
        SPSR = (0 << SPI2X); //clk*2 = off
        break;
    case 32:
        SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1); //enable SPI, Master, clk=Fcpu/64
        SPSR = (1 << SPI2X); //clk*2 = Fcpu/32
        break;
    }
#endif

    //reset display
    reset();

    p_size = 0;
    p_fg = 0X0000; //COLOR16_BLACK;
    p_bg = 0XFFFF; //COLOR16_WHITE;

#if !defined(SUPPORT_HY32D)
    // write upper y values to zero
    wr_cmd(0x06, 0); //set y0 upper byte
    wr_cmd(0x08, 0); //set y1 upper byte
#endif

    return;
}

/**
 * @param aBrightnessPercent value from 0 to 100
 */
void MI0283QT2::setBacklightBrightness(uint8_t aBrightnessPercent) {
    if (aBrightnessPercent == 0) { //off
        analogWrite(LED_PIN, 0);
        LED_DISABLE();
    } else if (aBrightnessPercent >= 100) { //100%
        analogWrite(LED_PIN, 255);
        LED_ENABLE();
    } else { //1...99%
        analogWrite(LED_PIN, (uint16_t) aBrightnessPercent * 255 / 100);
    }
}

void MI0283QT2::setOrientation(uint16_t o) {
#if !defined(SUPPORT_HY32D)
    switch (o) {
    case 0:
//        lcd_orientation = 0;
//        LOCAL_DISPLAY_WIDTH = 320;
//        LOCAL_DISPLAY_HEIGHT = 240;
        wr_cmd(0x16, 0x00A8); //MY=1 MX=0 MV=1 ML=0 BGR=1
        break;

    case 90:
//        lcd_orientation = 90;
//        LOCAL_DISPLAY_WIDTH = 240;
//        LOCAL_DISPLAY_HEIGHT = 320;
        wr_cmd(0x16, 0x0008); //MY=0 MX=0 MV=0 ML=0 BGR=1
        break;

    case 180:
//        lcd_orientation = 180;
//        LOCAL_DISPLAY_WIDTH = 320;
//        LOCAL_DISPLAY_HEIGHT = 240;
        wr_cmd(0x16, 0x0068); //MY=0 MX=1 MV=1 ML=0 BGR=1
        break;

    case 270:
//        lcd_orientation = 270;
//        LOCAL_DISPLAY_WIDTH = 240;
//        LOCAL_DISPLAY_HEIGHT = 320;
        wr_cmd(0x16, 0x00C8); //MY=1 MX=0 MV=1 ML=0 BGR=1
        break;
    }

    setArea(0, 0, LOCAL_DISPLAY_WIDTH - 1, LOCAL_DISPLAY_HEIGHT - 1);

    p_x = PRINT_STARTX;
    p_y = PRINT_STARTY;

#endif
}

uint16_t MI0283QT2::getDisplayWidth(void) {
    return LOCAL_DISPLAY_WIDTH;
}

uint16_t MI0283QT2::getDisplayHeight(void) {
    return LOCAL_DISPLAY_HEIGHT;
}

void MI0283QT2::setArea(uint16_t aXStart, uint16_t aYStart, uint16_t aXEnd, uint16_t aYEnd) {
    if ((aXEnd >= LOCAL_DISPLAY_WIDTH) || (aYEnd >= LOCAL_DISPLAY_HEIGHT)) {
        return;
    }
#if defined(SUPPORT_HY32D)
    writeCommand(0x44, aYStart + (aYEnd << 8)); //set ystart, yend
    writeCommand(0x45, aXStart); //set xStart
    writeCommand(0x46, aXEnd); //set xEnd
// also set cursor to right start position
    writeCommand(0x4E, aYStart);
    writeCommand(0x4F, aXStart);
#else
    wr_cmd(0x03, (aXStart >> 0)); //set aXStart
    wr_cmd(0x02, (aXStart >> 8)); //set aXStart
    wr_cmd(0x05, (aXEnd >> 0)); //set aXEnd
    wr_cmd(0x04, (aXEnd >> 8)); //set x1
    wr_cmd(0x07, (aYStart >> 0)); //set aYStart
//  wr_cmd(0x06, (aYStart >> 8)); //set aYStart
    wr_cmd(0x09, (aYEnd >> 0)); //set aYEnd
//  wr_cmd(0x08, (aYEnd >> 8)); //set aYEnd

#endif
}

void MI0283QT2::setCursor(uint16_t x, uint16_t y) {
    setArea(x, y, x, y);

    return;
}

void MI0283QT2::clearDisplay(color16_t aColor) {
    uint16_t size;

    setArea(0, 0, LOCAL_DISPLAY_WIDTH - 1, LOCAL_DISPLAY_HEIGHT - 1);

    drawStart();
    for (size = (320UL * 240UL / 8UL); size != 0; size--) {
        draw(aColor); //1
        draw(aColor); //2
        draw(aColor); //3
        draw(aColor); //4
        draw(aColor); //5
        draw(aColor); //6
        draw(aColor); //7
        draw(aColor); //8
    }
    drawStop();

    return;
}

void MI0283QT2::drawStart() {
#if defined(SUPPORT_HY32D)
    MI0283QT2_CS_ENABLE();
    DC_COMMAND();
    wr_spi(0x22); // #define LCD_GRAM_WRITE_REGISTER    0x22
    MI0283QT2_CS_DISABLE();

    MI0283QT2_CS_ENABLE();
    DC_DATA();

#else
    MI0283QT2_CS_ENABLE();
    wr_spi(LCD_REGISTER);
    wr_spi(0x22);
    MI0283QT2_CS_DISABLE();

    MI0283QT2_CS_ENABLE();
    wr_spi(LCD_DATA);
#endif
}

inline void MI0283QT2::draw(color16_t aColor) {
    wr_spi(aColor >> 8);
    wr_spi(aColor);
}

inline void MI0283QT2::drawStop(void) {
    MI0283QT2_CS_DISABLE();
}

void MI0283QT2::drawPixel(uint16_t aPositionX, uint16_t aPositionY, color16_t aColor) {
    if ((aPositionX >= LOCAL_DISPLAY_WIDTH) || (aPositionY >= LOCAL_DISPLAY_HEIGHT)) {
        return;
    }

    setArea(aPositionX, aPositionY, aPositionX, aPositionY);

    drawStart();
    draw(aColor);
    drawStop();
}

/*
 * needs an TFTDisplay.setArea(0, 0, LOCAL_DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1) first.
 */
void MI0283QT2::drawPixelFast(uint16_t aPositionX, uint8_t aPositionY, color16_t aColor) {

#if defined(SUPPORT_HY32D)
    setArea(aPositionX, aPositionY, aPositionX, aPositionY);
#else
    uint8_t xUpper;
    static uint8_t lastXUpper;

    // set area fast
    wr_cmd(0x03, (aPositionX >> 0)); //set x low byte
    xUpper = (aPositionX >> 8);
    //set x upper byte only when changed
    if (xUpper != lastXUpper) {
        wr_cmd(0x02, xUpper);
        lastXUpper = xUpper;
    }
    wr_cmd(0x07, (aPositionY >> 0)); //set y low byte - y high byte is always 0
#endif
    drawStart();
    draw(aColor);
    drawStop();

    return;
}

/*
 * Fast routine for drawing data charts
 * draws a line only from x to x+1
 * first pixel is omitted because it is drawn by preceeding line
 * uint16_t aStartX, uint16_t aStartY, uint16_t aEndY, color16_t aColor
 */
void MI0283QT2::drawLineFastOneX(uint16_t aStartX, uint16_t aStartY, uint16_t aEndY, color16_t aColor) {
    uint8_t i;
    bool up = true;
    //calculate direction
    int16_t deltaY = aEndY - aStartY;
    if (deltaY < 0) {
        deltaY = -deltaY;
        up = false;
    }
    if (deltaY <= 1) {
        // constant y or one pixel offset => no line needed
        drawPixel(aStartX + 1, aEndY, aColor);
    } else {
        // draw line here
        // deltaY1 is == deltaYHalf for even numbers and deltaYHalf -1 for odd Numbers
        uint8_t deltaY1 = (deltaY - 1) >> 1;
        uint8_t deltaYHalf = deltaY >> 1;
        if (up) {
            // for odd numbers, first part of line is 1 pixel shorter than second
            if (deltaY1 > 0) {
                // first pixel was drawn by preceeding line :-)
                setArea(aStartX, aStartY + 1, aStartX, aStartY + deltaY1);
                drawStart();
                for (i = deltaY1; i != 0; i--) {
                    draw(aColor);
                }
                drawStop();
            }
            setArea(aStartX + 1, aStartY + deltaY1 + 1, aStartX + 1, aEndY);
            drawStart();
            for (i = deltaYHalf + 1; i != 0; i--) {
                draw(aColor);
            }
            drawStop();
        } else {
            // for odd numbers, second part of line is 1 pixel shorter than first
            if (deltaYHalf > 0) {
                setArea(aStartX, aStartY - deltaYHalf, aStartX, aStartY - 1);
                drawStart();
                for (i = deltaYHalf; i != 0; i--) {
                    draw(aColor);
                }
                drawStop();
            }
            setArea(aStartX + 1, aEndY, aStartX + 1, (aStartY - deltaYHalf) - 1);
            drawStart();
            for (i = deltaY1 + 1; i != 0; i--) {
                draw(aColor);
            }
            drawStop();
        }
    }
}

void MI0283QT2::drawLineRel(uint16_t aStartX, uint16_t aStartY, int16_t aDeltaX, int16_t aDeltaY, color16_t aColor) {
    drawLine(aStartX, aStartY, aStartX + aDeltaX, aStartY + aDeltaY, aColor);
}
void MI0283QT2::drawLine(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor) {
    int16_t dx, dy, dx2, dy2, err, stepx, stepy;

    if (aStartX >= LOCAL_DISPLAY_WIDTH) {
        aStartX = LOCAL_DISPLAY_WIDTH - 1;
    }
    if (aEndX >= LOCAL_DISPLAY_WIDTH) {
        aEndX = LOCAL_DISPLAY_WIDTH - 1;
    }
    if (aStartY >= LOCAL_DISPLAY_HEIGHT) {
        aStartY = LOCAL_DISPLAY_HEIGHT - 1;
    }
    if (aEndY >= LOCAL_DISPLAY_HEIGHT) {
        aEndY = LOCAL_DISPLAY_HEIGHT - 1;
    }

    if ((aStartX == aEndX) || (aStartY == aEndY)) //horizontal or vertical line
            {
        fillRect(aStartX, aStartY, aEndX, aEndY, aColor);
    } else {
        //calculate direction
        dx = aEndX - aStartX;
        dy = aEndY - aStartY;
        if (dx < 0) {
            dx = -dx;
            stepx = -1;
        } else {
            stepx = +1;
        }
        if (dy < 0) {
            dy = -dy;
            stepy = -1;
        } else {
            stepy = +1;
        }
        dx2 = dx << 1;
        dy2 = dy << 1;
        //draw line
        drawPixel(aStartX, aStartY, aColor);
        if (dx > dy) {
            err = dy2 - dx;
            while (aStartX != aEndX) {
                if (err >= 0) {
                    aStartY += stepy;
                    err -= dx2;
                }
                aStartX += stepx;
                err += dy2;
                drawPixel(aStartX, aStartY, aColor);
            }
        } else {
            err = dx2 - dy;
            while (aStartY != aEndY) {
                if (err >= 0) {
                    aStartX += stepx;
                    err -= dy2;
                }
                aStartY += stepy;
                err += dx2;
                drawPixel(aStartX, aStartY, aColor);
            }
        }
    }
}

void MI0283QT2::drawRect(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor) {
    fillRect(aStartX, aStartY, aStartX, aEndY, aColor);
    fillRect(aStartX, aEndY, aEndX, aEndY, aColor);
    fillRect(aEndX, aStartY, aEndX, aEndY, aColor);
    fillRect(aStartX, aStartY, aEndX, aStartY, aColor);
}

void MI0283QT2::fillRectRel(uint16_t aStartX, uint16_t aStartY, uint16_t aWidth, uint16_t aHeight, color16_t aColor) {
    fillRect(aStartX, aStartY, aStartX + aWidth - 1, aStartY + aHeight - 1, aColor);
}

/**
 * Fill rectangle starting upper left with aStartX, aStartY
 * and ending lower right at aEndX, aEndY including these values
 */
void MI0283QT2::fillRect(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor) {
    uint32_t size;
    uint16_t tmp, i;

    /*
     * Swap if values are not as required
     * Costs 27 bytes
     */
    if (aStartX > aEndX) {
        tmp = aStartX;
        aStartX = aEndX;
        aEndX = tmp;
    }
    if (aStartY > aEndY) {
        tmp = aStartY;
        aStartY = aEndY;
        aEndY = tmp;
    }

    if (aEndX >= LOCAL_DISPLAY_WIDTH) {
        aEndX = LOCAL_DISPLAY_WIDTH - 1;
    }
    if (aEndY >= LOCAL_DISPLAY_HEIGHT) {
        aEndY = LOCAL_DISPLAY_HEIGHT - 1;
    }

    setArea(aStartX, aStartY, aEndX, aEndY);

    drawStart();
    size = (uint32_t) (1 + (aEndX - aStartX)) * (uint32_t) (1 + (aEndY - aStartY));
    tmp = size / 8;
    if (tmp != 0) {
        for (i = tmp; i != 0; i--) {
            draw(aColor); //1
            draw(aColor); //2
            draw(aColor); //3
            draw(aColor); //4
            draw(aColor); //5
            draw(aColor); //6
            draw(aColor); //7
            draw(aColor); //8
        }
        for (i = size - tmp; i != 0; i--) {
            draw(aColor);
        }
    } else {
        for (i = size; i != 0; i--) {
            draw(aColor);
        }
    }
    drawStop();
}

void MI0283QT2::drawCircle(uint16_t aCenterX, uint16_t aCenterY, uint16_t aRadius, color16_t aColor) {
    int16_t err, x, y;

    err = -aRadius;
    x = aRadius;
    y = 0;

    while (x >= y) {
        drawPixel(aCenterX + x, aCenterY + y, aColor);
        drawPixel(aCenterX - x, aCenterY + y, aColor);
        drawPixel(aCenterX + x, aCenterY - y, aColor);
        drawPixel(aCenterX - x, aCenterY - y, aColor);
        drawPixel(aCenterX + y, aCenterY + x, aColor);
        drawPixel(aCenterX - y, aCenterY + x, aColor);
        drawPixel(aCenterX + y, aCenterY - x, aColor);
        drawPixel(aCenterX - y, aCenterY - x, aColor);

        err += y;
        y++;
        err += y;
        if (err >= 0) {
            x--;
            err -= x;
            err -= x;
        }
    }
}

void MI0283QT2::fillCircle(uint16_t aCenterX, uint16_t aCenterY, uint16_t aRadius, color16_t aColor) {
    int16_t err, x, y;

    err = -aRadius;
    x = aRadius;
    y = 0;

    while (x >= y) {
        drawLine(aCenterX - x, aCenterY + y, aCenterX + x, aCenterY + y, aColor);
        drawLine(aCenterX - x, aCenterY - y, aCenterX + x, aCenterY - y, aColor);
        drawLine(aCenterX - y, aCenterY + x, aCenterX + y, aCenterY + x, aColor);
        drawLine(aCenterX - y, aCenterY - x, aCenterX + y, aCenterY - x, aColor);

        err += y;
        y++;
        err += y;
        if (err >= 0) {
            x--;
            err -= x;
            err -= x;
        }
    }
}

void MI0283QT2::printOptions(uint8_t size, color16_t aColor, color16_t bg_aColor) {
    p_size = size;
    p_fg = aColor;
    p_bg = bg_aColor;
}

void MI0283QT2::printClear(void) {
    clearDisplay(p_bg);

    p_x = PRINT_STARTX;
    p_y = PRINT_STARTY;
}

void MI0283QT2::printXY(uint16_t x, uint16_t y) {
    p_x = x;
    p_y = y;
}

uint16_t MI0283QT2::printGetX(void) {
    return p_x;
}

uint16_t MI0283QT2::printGetY(void) {
    return p_y;
}

void MI0283QT2::printPGM(PGM_P aString) {
    uint16_t x = p_x, y = p_y, x_last;
    char tChar;

    tChar = pgm_read_byte(aString++);
    while (tChar != 0) {
        if (tChar == '\n') //new line
                {
            x = PRINT_STARTX;
            y += (FONT_HEIGHT * p_size) + 1;
            if (y >= LOCAL_DISPLAY_HEIGHT) {
                y = PRINT_STARTY;
            }
        } else if (tChar == '\r') //skip
                {
            //do nothing
        } else {
            x_last = x;
            x = drawChar(x, y, tChar, p_size, p_fg, p_bg);
            if (x > LOCAL_DISPLAY_WIDTH) {
                fillRect(x_last, y, LOCAL_DISPLAY_WIDTH - 1, y + (FONT_HEIGHT * p_size), p_bg);
                x = PRINT_STARTX;
                y += (FONT_HEIGHT * p_size) + 1;
                if (y >= LOCAL_DISPLAY_HEIGHT) {
                    y = PRINT_STARTY;
                }
                x = drawChar(x, y, tChar, p_size, p_fg, p_bg);
            }
        }
        tChar = pgm_read_byte(aString++);
    }

    p_x = x;
    p_y = y;
}

size_t MI0283QT2::write(uint8_t c) {
    uint16_t x = p_x, y = p_y;

    if (c == '\n') {
        x = PRINT_STARTX;
        y += (FONT_HEIGHT * p_size) + 1;
        if (y >= LOCAL_DISPLAY_HEIGHT) {
            y = PRINT_STARTY;
        }
    } else if (c == '\r') //skip
            {
        //do nothing
    } else {
        x = drawChar(x, y, c, p_size, p_fg, p_bg);
        if (x > LOCAL_DISPLAY_WIDTH) {
            fillRect(p_x, y, LOCAL_DISPLAY_WIDTH - 1, y + (FONT_HEIGHT * p_size), p_bg);
            x = PRINT_STARTX;
            y += (FONT_HEIGHT * p_size) + 1;
            if (y >= LOCAL_DISPLAY_HEIGHT) {
                y = PRINT_STARTY;
            }
            x = drawChar(x, y, c, p_size, p_fg, p_bg);
        }
    }

    p_x = x;
    p_y = y;

    return 1;
}

size_t MI0283QT2::write(const char *s) {
    size_t len = 0;

    while (*s) {
        write((uint8_t) *s++);
        len++;
    }

    return len;
}

size_t MI0283QT2::write(const uint8_t *s, size_t size) {
    size_t len = 0;

    while (size != 0) {
        write((uint8_t) *s++);
        size--;
        len++;
    }

    return len;
}

//-------------------- Private --------------------

void MI0283QT2::reset() {
    //SPI speed-down
#if !defined(SOFTWARE_SPI)
    uint8_t spcr, spsr;
    spcr = SPCR;
    SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR0); //enable SPI, Master, clk=Fcpu/16
    spsr = SPSR;
    SPSR = (1 << SPI2X); //clk*2 -> clk=Fcpu/8
#endif

#if defined(SUPPORT_HY32D)
    // Reset is done by hardware reset button
    // Original Code
    writeCommand(0x0000, 0x0001); // Enable LCD Oscillator
    _delay_ms(10);

    writeCommand(0x0003, 0xA8A4); // Power control A=fosc/4 - 4= Small to medium
    writeCommand(0x000C, 0x0000); // VCIX2 only bit [2:0]
    writeCommand(0x000D, 0x080C); // VLCD63 only bit [3:0]
    writeCommand(0x000E, 0x2B00);
    writeCommand(0x001E, 0x00B0); // Bit7 + VcomH bit [5:0]
    writeCommand(0x0001, 0x293F); // reverse 320

    writeCommand(0x0002, 0x0600); // LCD driver AC setting
    writeCommand(0x0010, 0x0000); // Exit sleep mode
    _delay_ms(50);

    writeCommand(0x0011, 0x6038); // 6=65k Color, 38=draw direction -> 3=horizontal increment, 8=vertical increment
    //  writeCommand(0x0016, 0xEF1C); // 240 pixel
    writeCommand(0x0017, 0x0003);
    writeCommand(0x0007, 0x0133); // 1=the 2-division LCD drive is performed, 8 Color mode, grayscale
    writeCommand(0x000B, 0x0000);
    writeCommand(0x000F, 0x0000); // Gate Scan Position start (0-319)
    writeCommand(0x0041, 0x0000); // Vertical Scroll Control
    writeCommand(0x0042, 0x0000);
    //  writeCommand(0x0048, 0x0000); // 0 is default 1st Screen driving position
    //  writeCommand(0x0049, 0x013F); // 13F is default
    //  writeCommand(0x004A, 0x0000); // 0 is default 2nd Screen driving position
    //  writeCommand(0x004B, 0x0000);  // 13F is default

    _delay_ms(10);
    //gamma control
    writeCommand(0x0030, 0x0707);
    writeCommand(0x0031, 0x0204);
    writeCommand(0x0032, 0x0204);
    writeCommand(0x0033, 0x0502);
    writeCommand(0x0034, 0x0507);
    writeCommand(0x0035, 0x0204);
    writeCommand(0x0036, 0x0204);
    writeCommand(0x0037, 0x0502);
    writeCommand(0x003A, 0x0302);
    writeCommand(0x003B, 0x0302);

    writeCommand(0x0025, 0x8000); // Frequency Control 8=65Hz 0=50HZ E=80Hz
#else
    //reset
    MI0283QT2_CS_DISABLE();
    RST_ENABLE();
    delay(50);
    RST_DISABLE();
    delay(50);

    //driving ability
    wr_cmd(0xEA, 0x0000);
    wr_cmd(0xEB, 0x0020);
    wr_cmd(0xEC, 0x000C);
    wr_cmd(0xED, 0x00C4);
    wr_cmd(0xE8, 0x0040);
    wr_cmd(0xE9, 0x0038);
    wr_cmd(0xF1, 0x0001);
    wr_cmd(0xF2, 0x0010);
    wr_cmd(0x27, 0x00A3);

    //power voltage
    wr_cmd(0x1B, 0x001B);
    wr_cmd(0x1A, 0x0001);
    wr_cmd(0x24, 0x002F);
    wr_cmd(0x25, 0x0057);

    //VCOM offset
    wr_cmd(0x23, 0x008D); //for flicker adjust

    //power on
    wr_cmd(0x18, 0x0036);
    wr_cmd(0x19, 0x0001); //start osc
    wr_cmd(0x01, 0x0000); //wakeup
    wr_cmd(0x1F, 0x0088);
    _delay_ms(5);
    wr_cmd(0x1F, 0x0080);
    _delay_ms(5);
    wr_cmd(0x1F, 0x0090);
    _delay_ms(5);
    wr_cmd(0x1F, 0x00D0);
    _delay_ms(5);

    //color selection
    wr_cmd(0x17, 0x0005); //0x0005=65k, 0x0006=262k

    //panel characteristic
    wr_cmd(0x36, 0x0000);

    //display on
    wr_cmd(0x28, 0x0038);
    delay(40);
    wr_cmd(0x28, 0x003C);

    //display options
    LocalDisplay.setOrientation(0);
#endif

    //restore SPI settings
#if !defined(SOFTWARE_SPI)
    SPCR = spcr;
    SPSR = spsr;
#endif

}

#if defined(SUPPORT_HY32D)
void MI0283QT2::writeCommand(uint8_t aRegisterAddress, uint16_t aRegisterValue) {
    MI0283QT2_CS_ENABLE();
    DC_COMMAND();
    wr_spi(aRegisterAddress);
    MI0283QT2_CS_DISABLE();
    MI0283QT2_CS_ENABLE();
    DC_DATA();
    wr_spi(aRegisterValue >> 8);
    wr_spi(aRegisterValue);
    MI0283QT2_CS_DISABLE();
}
#else
void MI0283QT2::wr_cmd(uint8_t reg, uint8_t param) {
    MI0283QT2_CS_ENABLE();
    wr_spi(LCD_REGISTER);
    wr_spi(reg);
    MI0283QT2_CS_DISABLE();

    MI0283QT2_CS_ENABLE();
    wr_spi(LCD_DATA);
    wr_spi(param);
    MI0283QT2_CS_DISABLE();
}
#endif

//void MI0283QT2::wr_data(uint16_t data) {
//    MI0283QT2_CS_ENABLE();
//    wr_spi(LCD_DATA);
//    wr_spi(data >> 8);
//    wr_spi(data);
//    MI0283QT2_CS_DISABLE();
//
//    return;
//}

void MI0283QT2::wr_spi(uint8_t data) {
#if defined(SOFTWARE_SPI)
    uint8_t mask;

    for (mask = 0x80; mask != 0; mask >>= 1) {
        CLK_LOW();
        if (mask & data) {
            MOSI_HIGH();
        } else {
            MOSI_LOW();
        }
        CLK_HIGH();
    }
    CLK_LOW();

#else
    SPDR = data;
    while (!(SPSR & (1 << SPIF)))
        ;
#endif
}
