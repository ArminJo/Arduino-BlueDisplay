/**
 * SSD1289.hpp
 *
 * Firmware to control a HY32D 320 x 240, 3.2" Display with a SSD1289 controller
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
 *
 */

#include "SSD1289.h"
#include "LocalDisplayInterface.hpp"
#include "LocalGUI/ThickLine.h" // for drawLineOverlap()

#include "BlueDisplay.h"
#include "main.h" // for StringBuffer
#include "STM32TouchScreenDriver.h"
#include "timing.h"
#include "stm32fx0xPeripherals.h"

#include <string.h>  // for strcat
#include <stdlib.h>  // for malloc


/** @addtogroup Graphic_Library
 * @{
 */
/** @addtogroup SSD1289_basic
 * @{
 */

#define LCD_GRAM_WRITE_REGISTER    0x22

bool isInitializedSSD1289 = false;
volatile uint32_t sDrawLock = 0;
/*
 * For automatic LCD dimming
 */
uint8_t sCurrentBacklightPercent = BACKLIGHT_START_BRIGHTNESS_VALUE;
uint8_t sLastBacklightPercentBeforeDimming; //! for state of backlight before dimming
int sLCDDimDelay; //actual dim delay

//-------------------- Private functions --------------------
void writeCommand(int aRegisterAddress, int aRegisterValue);
bool initalizeDisplay(void);
void setBrightness(int power); //0-100

//-------------------- Constructor --------------------

SSD1289::SSD1289() { // @suppress("Class members should be properly initialized")
}

SSD1289::~SSD1289() {
}

//-------------------- Public --------------------
void SSD1289::init(void) {
//init pins
    SSD1289_IO_initalize();
// init PWM for background LED
    PWM_BL_initalize();
    setBrightness(BACKLIGHT_START_BRIGHTNESS_VALUE);

// deactivate read output control
    HY32D_RD_GPIO_PORT->BSRR = HY32D_RD_PIN;
//initalize display
    if (initalizeDisplay()) {
        isLocalDisplayAvailable = true;
    }
}

void SSD1289::setArea(uint16_t aPositionX, uint16_t aPositionY, uint16_t aXEnd, uint16_t aYEnd) {
    if ((aXEnd >= LOCAL_DISPLAY_WIDTH) || (aYEnd >= LOCAL_DISPLAY_HEIGHT)) {
        assertFailedParamMessage((uint8_t*) __FILE__, __LINE__, aXEnd, aYEnd, "");
    }

    writeCommand(0x44, aPositionY + (aYEnd << 8)); //set ystart, yend
    writeCommand(0x45, aPositionX); //set xStart
    writeCommand(0x46, aXEnd); //set xEnd
// also set cursor to right start position
    writeCommand(0x4E, aPositionY);
    writeCommand(0x4F, aPositionX);
}

void SSD1289::setCursor(uint16_t aPositionX, uint16_t aPositionY) {
    writeCommand(0x4E, aPositionY);
    writeCommand(0x4F, aPositionX);
}

void SSD1289::clearDisplay(uint16_t aColor) {
    unsigned int size;
    setArea(0, 0, LOCAL_DISPLAY_WIDTH - 1, LOCAL_DISPLAY_HEIGHT - 1);

    drawStart();
    for (size = (LOCAL_DISPLAY_HEIGHT * LOCAL_DISPLAY_WIDTH); size != 0; size--) {
        HY32D_DATA_GPIO_PORT->ODR = aColor;
        // Latch data write
        HY32D_WR_GPIO_PORT->BSRR = (uint32_t) HY32D_WR_PIN << 16;
        HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;

    }
    HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;

}

uint16_t SSD1289::getDisplayWidth(void) {
    return LOCAL_DISPLAY_WIDTH;
}

uint16_t SSD1289::getDisplayHeight(void) {
    return LOCAL_DISPLAY_HEIGHT;
}

/**
 * set register address to LCD_GRAM_READ/WRITE_REGISTER
 */
void SSD1289::drawStart(void) {
// CS enable (low)
    HY32D_CS_GPIO_PORT->BSRR = (uint32_t) HY32D_CS_PIN << 16;
// Control enable (low)
    HY32D_DATA_CONTROL_GPIO_PORT->BSRR = (uint32_t) HY32D_DATA_CONTROL_PIN << 16;
// set value
    HY32D_DATA_GPIO_PORT->ODR = LCD_GRAM_WRITE_REGISTER;
// Latch data write
    HY32D_WR_GPIO_PORT->BSRR = (uint32_t) HY32D_WR_PIN << 16;
    HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;
// Data enable (high)
    HY32D_DATA_CONTROL_GPIO_PORT->BSRR = HY32D_DATA_CONTROL_PIN;
}

void SSD1289::draw(color16_t aColor) {
// set value
    HY32D_DATA_GPIO_PORT->ODR = aColor;
// Latch data write
    HY32D_WR_GPIO_PORT->BSRR = (uint32_t) HY32D_WR_PIN << 16;
    HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;
}

void SSD1289::drawStop() {
    HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;
}

void SSD1289::drawPixel(uint16_t aPositionX, uint16_t aPositionY, uint16_t aColor) {
    if ((aPositionX >= LOCAL_DISPLAY_WIDTH) || (aPositionY >= LOCAL_DISPLAY_HEIGHT)) {
        return;
    }

// setCursor
    writeCommand(0x4E, aPositionY);
    writeCommand(0x4F, aPositionX);

    drawStart();
    draw(aColor);
    drawStop();
}

void SSD1289::drawLineRel(uint16_t aStartX, uint16_t aStartY, int16_t aDeltaX, int16_t aDeltaY, color16_t aColor) {
    drawLine(aStartX, aStartY, aStartX + aDeltaX, aStartY + aDeltaY, aColor);
}

void SSD1289::drawLine(uint16_t aPositionX, uint16_t aPositionY, uint16_t aXEnd, uint16_t aYEnd, color16_t aColor) {
    drawLineOverlap(aPositionX, aPositionY, aXEnd, aYEnd, LINE_OVERLAP_NONE, aColor);
}

void SSD1289::fillRect(uint16_t aPositionX, uint16_t aPositionY, uint16_t aXEnd, uint16_t aYEnd, color16_t aColor) {
    uint32_t size;
    uint16_t tmp, i;

    if (aPositionX > aXEnd) {
        tmp = aPositionX;
        aPositionX = aXEnd;
        aXEnd = tmp;
    }
    if (aPositionY > aYEnd) {
        tmp = aPositionY;
        aPositionY = aYEnd;
        aYEnd = tmp;
    }

    if (aXEnd >= LOCAL_DISPLAY_WIDTH) {
        aXEnd = LOCAL_DISPLAY_WIDTH - 1;
    }
    if (aYEnd >= LOCAL_DISPLAY_HEIGHT) {
        aYEnd = LOCAL_DISPLAY_HEIGHT - 1;
    }

    setArea(aPositionX, aPositionY, aXEnd, aYEnd);

    drawStart();
    size = (uint32_t) (1 + (aXEnd - aPositionX)) * (uint32_t) (1 + (aYEnd - aPositionY));
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

void SSD1289::fillRectRel(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidth, uint16_t aHeight, color16_t aColor) {
    LocalDisplay.fillRect(aPositionX, aPositionY, aPositionX + aWidth - 1, aPositionY + aHeight - 1, aColor);
}

void SSD1289::drawCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, uint16_t aColor) {
    int16_t err, x, y;

    err = -aRadius;
    x = aRadius;
    y = 0;

    while (x >= y) {
        drawPixel(aXCenter + x, aYCenter + y, aColor);
        drawPixel(aXCenter - x, aYCenter + y, aColor);
        drawPixel(aXCenter + x, aYCenter - y, aColor);
        drawPixel(aXCenter - x, aYCenter - y, aColor);
        drawPixel(aXCenter + y, aYCenter + x, aColor);
        drawPixel(aXCenter - y, aYCenter + x, aColor);
        drawPixel(aXCenter + y, aYCenter - x, aColor);
        drawPixel(aXCenter - y, aYCenter - x, aColor);

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

void SSD1289::fillCircle(uint16_t aXCenter, uint16_t aYCenter, uint16_t aRadius, uint16_t aColor) {
    int16_t err, x, y;

    err = -aRadius;
    x = aRadius;
    y = 0;

    while (x >= y) {
        drawLine(aXCenter - x, aYCenter + y, aXCenter + x, aYCenter + y, aColor);
        drawLine(aXCenter - x, aYCenter - y, aXCenter + x, aYCenter - y, aColor);
        drawLine(aXCenter - y, aYCenter + x, aXCenter + y, aYCenter + x, aColor);
        drawLine(aXCenter - y, aYCenter - x, aXCenter + y, aYCenter - x, aColor);

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

uint16_t SSD1289::readPixel(uint16_t aPositionX, uint16_t aPositionY) {
    if ((aPositionX >= LOCAL_DISPLAY_WIDTH) || (aPositionY >= LOCAL_DISPLAY_HEIGHT)) {
        return 0;
    }

// setCursor
    writeCommand(0x4E, aPositionY);
    writeCommand(0x4F, aPositionX);

    drawStart();
    uint16_t tValue = 0;
// set port pins to input
    HY32D_DATA_GPIO_PORT->MODER = 0x00000000;
// Latch data read
    HY32D_WR_GPIO_PORT->BRR = HY32D_RD_PIN;
// wait >250ns
    delayNanos(300);

    tValue = HY32D_DATA_GPIO_PORT->IDR;
    HY32D_WR_GPIO_PORT->BSRR = HY32D_RD_PIN;
// set port pins to output
    HY32D_DATA_GPIO_PORT->MODER = 0x55555555;
    HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;

    return tValue;
}

void SSD1289::drawRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, color16_t color) {
    fillRect(x0, y0, x0, y1, color);
    fillRect(x0, y1, x1, y1, color);
    fillRect(x1, y0, x1, y1, color);
    fillRect(x0, y0, x1, y0, color);
}

/*
 * Fast routine for drawing data charts
 * draws a line only from x to x+1
 * first pixel is omitted because it is drawn by preceding line
 * uses setArea instead if drawPixel to speed up drawing
 */
void SSD1289::drawLineFastOneX(uint16_t aPositionX, uint16_t aPositionY, uint16_t aYEnd, color16_t aColor) {
    uint8_t i;
    bool up = true;
//calculate direction
    int16_t deltaY = aYEnd - aPositionY;
    if (deltaY < 0) {
        deltaY = -deltaY;
        up = false;
    }
    if (deltaY <= 1) {
        // constant y or one pixel offset => no line needed
        LocalDisplay.drawPixel(aPositionX + 1, aYEnd, aColor);
    } else {
        // draw line here
        // deltaY1 is == deltaYHalf for even numbers and deltaYHalf -1 for odd Numbers
        uint8_t deltaY1 = (deltaY - 1) >> 1;
        uint8_t deltaYHalf = deltaY >> 1;
        if (up) {
            // for odd numbers, first part of line is 1 pixel shorter than second
            if (deltaY1 > 0) {
                // first pixel was drawn by preceding line :-)
                setArea(aPositionX, aPositionY + 1, aPositionX, aPositionY + deltaY1);
                drawStart();
                for (i = deltaY1; i != 0; i--) {
                    draw(aColor);
                }
                drawStop();
            }
            setArea(aPositionX + 1, aPositionY + deltaY1 + 1, aPositionX + 1, aYEnd);
            drawStart();
            for (i = deltaYHalf + 1; i != 0; i--) {
                draw(aColor);
            }
            drawStop();
        } else {
            // for odd numbers, second part of line is 1 pixel shorter than first
            if (deltaYHalf > 0) {
                setArea(aPositionX, aPositionY - deltaYHalf, aPositionX, aPositionY - 1);
                drawStart();
                for (i = deltaYHalf; i != 0; i--) {
                    draw(aColor);
                }
                drawStop();
            }
            setArea(aPositionX + 1, aYEnd, aPositionX + 1, (aPositionY - deltaYHalf) - 1);
            drawStart();
            for (i = deltaY1 + 1; i != 0; i--) {
                draw(aColor);
            }
            drawStop();
        }
    }
}

/**
 *
 * @param aPositionX
 * @param aPositionY
 * @param aString
 * @param aFontScaleFactor
 * @param aColor
 * @param aBackgroundColor
 */
void SSD1289::drawTextVertical(uint16_t aPositionX, uint16_t aPositionY, const char *aString, uint8_t aFontScaleFactor,
        uint16_t aColor, uint16_t aBackgroundColor) {
    while (*aString != 0) {
        LocalDisplay.drawChar(aPositionX, aPositionY, (char) *aString++, aFontScaleFactor, aColor, aBackgroundColor);
        aPositionY += FONT_HEIGHT * aFontScaleFactor;
        if (aPositionY > LOCAL_DISPLAY_HEIGHT) {
            break;
        }
    }
}

uint16_t drawInteger(uint16_t x, uint16_t y, int val, uint8_t base, uint8_t size, uint16_t color, uint16_t bg_color) {
    char tmp[16 + 1];
    switch (base) {
    case 8:
        sprintf(tmp, "%o", (uint) val);
        break;
    case 10:
        sprintf(tmp, "%i", (uint) val);
        break;
    case 16:
        sprintf(tmp, "%x", (uint) val);
        break;
    }

    return LocalDisplay.drawText(x, y, tmp, size, color, bg_color);
}

/**
 * @param aBrightnessPercent value from 0 to 100
 */
void SSD1289::setBacklightBrightness(uint8_t aBrightnessPercent) {
    if (aBrightnessPercent > BACKLIGHT_MAX_BRIGHTNESS_VALUE) {
        aBrightnessPercent = BACKLIGHT_MAX_BRIGHTNESS_VALUE;
    }
    setBrightness(aBrightnessPercent);
}

void setBrightness(int aLCDBacklightPercent) {
    PWM_BL_setOnRatio(aLCDBacklightPercent);
    sCurrentBacklightPercent = aLCDBacklightPercent;
    sLastBacklightPercentBeforeDimming = aLCDBacklightPercent;
}

/**
 * Value for lcd backlight dimming delay
 */
void setDimdelay(int32_t aTimeMillis) {
    changeDelayCallback(&callbackLCDDimming, aTimeMillis);
    sLCDDimDelay = aTimeMillis;
}

/**
 * restore backlight to value before dimming
 */
void resetBacklightTimeout(void) {
    if (sLastBacklightPercentBeforeDimming != sCurrentBacklightPercent) {
        setBrightness(sLastBacklightPercentBeforeDimming);
    }
    changeDelayCallback(&callbackLCDDimming, sLCDDimDelay);
}

/**
 * Callback routine for SysTick handler
 * Dim LCD after period of touch inactivity
 */
void callbackLCDDimming(void) {
    if (sCurrentBacklightPercent > BACKLIGHT_DIM_VALUE) {
        PWM_BL_setOnRatio(BACKLIGHT_DIM_VALUE);
        sCurrentBacklightPercent = BACKLIGHT_DIM_VALUE;
    }
}

void writeCommand(int aRegisterAddress, int aRegisterValue) {
// CS enable (low)
    HY32D_CS_GPIO_PORT->BSRR = (uint32_t) HY32D_CS_PIN << 16;
// Control enable (low)
    HY32D_DATA_CONTROL_GPIO_PORT->BSRR = (uint32_t) HY32D_DATA_CONTROL_PIN << 16;
// set value
    HY32D_DATA_GPIO_PORT->ODR = aRegisterAddress;
// Latch data write
    HY32D_WR_GPIO_PORT->BSRR = (uint32_t) HY32D_WR_PIN << 16;
    HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;

// Data enable (high)
    HY32D_DATA_CONTROL_GPIO_PORT->BSRR = HY32D_DATA_CONTROL_PIN;
// set value
    HY32D_DATA_GPIO_PORT->ODR = aRegisterValue;
// Latch data write
    HY32D_WR_GPIO_PORT->BSRR = (uint32_t) HY32D_WR_PIN << 16;
    HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;

// CS disable (high)
    HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;
    return;
}

uint16_t readCommand(int aRegisterAddress) {
// CS enable (low)
    HY32D_CS_GPIO_PORT->BSRR = (uint32_t) HY32D_CS_PIN << 16;
// Control enable (low)
    HY32D_DATA_CONTROL_GPIO_PORT->BSRR = (uint32_t) HY32D_DATA_CONTROL_PIN << 16;
// set value
    HY32D_DATA_GPIO_PORT->ODR = aRegisterAddress;
// Latch data write
    HY32D_WR_GPIO_PORT->BSRR = (uint32_t) HY32D_WR_PIN << 16;
    HY32D_WR_GPIO_PORT->BSRR = HY32D_WR_PIN;

// Data enable (high)
    HY32D_DATA_CONTROL_GPIO_PORT->BSRR = HY32D_DATA_CONTROL_PIN;
// set port pins to input
    HY32D_DATA_GPIO_PORT->MODER = 0x00000000;
// Latch data read
    HY32D_WR_GPIO_PORT->BSRR = (uint32_t) HY32D_RD_PIN << 16;
// wait >250ns
    delayNanos(300);
    uint16_t tValue = HY32D_DATA_GPIO_PORT->IDR;
    HY32D_WR_GPIO_PORT->BSRR = HY32D_RD_PIN;
// set port pins to output
    HY32D_DATA_GPIO_PORT->MODER = 0x55555555;
// CS disable (high)
    HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;
    return tValue;
}

bool initalizeDisplay(void) {
// Reset is done by hardware reset button
// Original Code
    writeCommand(0x0000, 0x0001); // Enable LCD Oscillator
    delay(10);
// Check Device Code - 0x8989
    if (readCommand(0x0000) != 0x8989) {
        return false;
    }

    writeCommand(0x0003, 0xA8A4); // Power control A=fosc/4 - 4= Small to medium
    writeCommand(0x000C, 0x0000); // VCIX2 only bit [2:0]
    writeCommand(0x000D, 0x080C); // VLCD63 only bit [3:0]
    writeCommand(0x000E, 0x2B00);
    writeCommand(0x001E, 0x00B0); // Bit7 + VcomH bit [5:0]
    writeCommand(0x0001, 0x293F); // reverse 320

    writeCommand(0x0002, 0x0600); // LCD driver AC setting
    writeCommand(0x0010, 0x0000); // Exit sleep mode
    delay(50);

    writeCommand(0x0011, 0x6038); // 6=65k Color, 38=draw direction -> 3=horizontal increment, 8=vertical increment
//	writeCommand(0x0016, 0xEF1C); // 240 pixel
    writeCommand(0x0017, 0x0003);
    writeCommand(0x0007, 0x0133); // 1=the 2-division LCD drive is performed, 8 Color mode, grayscale
    writeCommand(0x000B, 0x0000);
    writeCommand(0x000F, 0x0000); // Gate Scan Position start (0-319)
    writeCommand(0x0041, 0x0000); // Vertical Scroll Control
    writeCommand(0x0042, 0x0000);
//	writeCommand(0x0048, 0x0000); // 0 is default 1st Screen driving position
//	writeCommand(0x0049, 0x013F); // 13F is default
//	writeCommand(0x004A, 0x0000); // 0 is default 2nd Screen driving position
//	writeCommand(0x004B, 0x0000);  // 13F is default

    delay(10);
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
    return true;
}

/*
 * not checked after reset, only after first calling initalizeDisplay() above
 */
void initalizeDisplay2(void) {
// Reset is done by hardware reset button
    delay(1);
    writeCommand(0x0011, 0x6838); // 6=65k Color, 8 = OE defines the display window 0 =the display window is defined by R4Eh and R4Fh.
//writeCommand(0x0011, 0x6038); // 6=65k Color, 8 = OE defines the display window 0 =the display window is defined by R4Eh and R4Fh.
//Entry Mode setting
    writeCommand(0x0002, 0x0600); // LCD driver AC setting
    writeCommand(0x0012, 0x6CEB); // RAM data write
// power control
    writeCommand(0x0003, 0xA8A4);
    writeCommand(0x000C, 0x0000); //VCIX2 only bit [2:0]
    writeCommand(0x000D, 0x080C); // VLCD63 only bit [3:0] ==
//	writeCommand(0x000D, 0x000C); // VLCD63 only bit [3:0]
    writeCommand(0x000E, 0x2B00); // ==
    writeCommand(0x001E, 0x00B0); // Bit7 + VcomH bit [5:0] ==
    writeCommand(0x0001, 0x293F); // reverse 320

// compare register
//writeCommand(0x0005, 0x0000);
//writeCommand(0x0006, 0x0000);

//writeCommand(0x0017, 0x0103); //Vertical Porch
    delay(1);

    delay(30);
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

    writeCommand(0x002F, 0x12BE);
    writeCommand(0x0023, 0x0000);
    delay(1);
    writeCommand(0x0024, 0x0000);
    delay(1);
    writeCommand(0x0025, 0x8000);

    writeCommand(0x004e, 0x0000); // RAM address set
    writeCommand(0x004f, 0x0000);
    return;
}

void setGamma(int aIndex) {
    switch (aIndex) {
    case 0:
        //old gamma
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
        break;
    case 1:
        // new gamma
        writeCommand(0x0030, 0x0707);
        writeCommand(0x0031, 0x0704);
        writeCommand(0x0032, 0x0204);
        writeCommand(0x0033, 0x0201);
        writeCommand(0x0034, 0x0203);
        writeCommand(0x0035, 0x0204);
        writeCommand(0x0036, 0x0204);
        writeCommand(0x0037, 0x0502);
        writeCommand(0x003A, 0x0302);
        writeCommand(0x003B, 0x0500);
        break;
    default:
        break;
    }
}

/**
 * reads a display line in BMP 16 Bit format. ie. only 5 bit for green
 */
uint16_t* SSD1289::fillDisplayLineBuffer(uint16_t *aBufferPtr, uint16_t yLineNumber) {
// set area is needed!
    setArea(0, yLineNumber, LOCAL_DISPLAY_WIDTH - 1, yLineNumber);
    drawStart();
    uint16_t tValue = 0;
// set port pins to input
    HY32D_DATA_GPIO_PORT->MODER = 0x00000000;
    for (uint16_t i = 0; i <= LOCAL_DISPLAY_WIDTH; ++i) {
        // Latch data read
        HY32D_WR_GPIO_PORT->BSRR = (uint32_t) HY32D_RD_PIN << 16;
        // wait >250ns (and process former value)
        if (i > 1) {
            // skip inital value (=0) and first reading from display (is from last read => scrap)
            // shift red and green one bit down so that every color has 5 bits
            tValue = (tValue & COLOR16_BLUEMASK) | ((tValue >> 1) & ~COLOR16_BLUEMASK);
            *aBufferPtr++ = tValue;
        }
        tValue = HY32D_DATA_GPIO_PORT->IDR;
        HY32D_WR_GPIO_PORT->BSRR = HY32D_RD_PIN;
    }
// last value
    tValue = (tValue & COLOR16_BLUEMASK) | ((tValue >> 1) & ~COLOR16_BLUEMASK);
    *aBufferPtr++ = tValue;
// set port pins to output
    HY32D_DATA_GPIO_PORT->MODER = 0x55555555;
    HY32D_CS_GPIO_PORT->BSRR = HY32D_CS_PIN;
    return aBufferPtr;
}

extern "C" void storeScreenshot(void) {
    bool tIsError = true;
    if (MICROSD_isCardInserted()) {

        FIL tFile;
        FRESULT tOpenResult;
        UINT tCount;
//	int filesize = 54 + 2 * LOCAL_DISPLAY_WIDTH * LOCAL_DISPLAY_HEIGHT;

        unsigned char bmpfileheader[14] = { 'B', 'M', 54, 88, 02, 0, 0, 0, 0, 0, 54, 0, 0, 0 };
        unsigned char bmpinfoheader[40] = { 40, 0, 0, 0, 64, 1, 0, 0, 240, 0, 0, 0, 1, 0, 16, 0 };

//	bmpfileheader[2] = (unsigned char) (filesize);
//	bmpfileheader[3] = (unsigned char) (filesize >> 8);
//	bmpfileheader[4] = (unsigned char) (filesize >> 16);
//	bmpfileheader[5] = (unsigned char) (filesize >> 24);

        RTC_getDateStringForFile(sStringBuffer);
        strcat(sStringBuffer, ".bmp");
        tOpenResult = f_open(&tFile, sStringBuffer, FA_CREATE_ALWAYS | FA_WRITE);
        uint16_t *tBufferPtr;
        if (tOpenResult == FR_OK) {
            uint16_t *tFourDisplayLinesBufferPointer = (uint16_t*) malloc(sizeof(uint16_t) * 4 * DISPLAY_DEFAULT_WIDTH);
            if (tFourDisplayLinesBufferPointer == NULL) {
                failParamMessage(sizeof(uint16_t) * 4 * DISPLAY_DEFAULT_WIDTH, "malloc() fails");
            }
            f_write(&tFile, bmpfileheader, 14, &tCount);
            f_write(&tFile, bmpinfoheader, 40, &tCount);
            // from left to right and from bottom to top
            for (int i = LOCAL_DISPLAY_HEIGHT - 1; i >= 0;) {
                tBufferPtr = tFourDisplayLinesBufferPointer;
                // write 4 lines at a time to speed up I/O
                tBufferPtr = LocalDisplay.fillDisplayLineBuffer(tBufferPtr, i--);
                tBufferPtr = LocalDisplay.fillDisplayLineBuffer(tBufferPtr, i--);
                tBufferPtr = LocalDisplay.fillDisplayLineBuffer(tBufferPtr, i--);
                LocalDisplay.fillDisplayLineBuffer(tBufferPtr, i--);
                // write a display line
                f_write(&tFile, tFourDisplayLinesBufferPointer, LOCAL_DISPLAY_WIDTH * 8, &tCount);
            }
            free(tFourDisplayLinesBufferPointer);
            f_close(&tFile);
            tIsError = false;
        }
    }
    LocalTouchButton::playFeedbackTone(tIsError);
}

/** @} */
/** @} */

