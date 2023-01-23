/*
 * MI0283QT2.h
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

#ifndef _MI0283QT2_H
#define _MI0283QT2_H

#include <stdint.h>

#include "LocalDisplayInterface.h" // for LOCAL_DISPLAY_HEIGHT and LOCAL_DISPLAY_WIDTH

#include "Colors.h" // for color16_t

#define BACKLIGHT_START_BRIGHTNESS_VALUE     50
#define BACKLIGHT_MAX_BRIGHTNESS_VALUE      100

class MI0283QT2: public LocalDisplayInterface, Print { // @suppress("Class has a virtual method and non-virtual destructor")
public:

    MI0283QT2();
    void init(uint8_t aSPIClockDivider); //2 4 8 16 32
    void setBacklightBrightness(uint8_t aBrightnessPercent); //0-100

    void setOrientation(uint16_t o); //0 90 180 270
    uint16_t getDisplayWidth(void);
    uint16_t getDisplayHeight(void);
    void setCursor(uint16_t x, uint16_t y);

    /*
     * Basic hardware functions required for the draw functions below
     */
    void fillRect(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor);
    void setArea(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY);
    void drawStart();
    void draw(color16_t aColor);
    void drawStop();

    void drawPixel(uint16_t x0, uint16_t y0, uint16_t color);
    void drawPixelFast(uint16_t x0, uint8_t y0, uint16_t color);
    void drawLine(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor);
    void drawLineRel(uint16_t aStartX, uint16_t aStartY, int16_t aDeltaX, int16_t aDeltaY, color16_t aColor);
    void drawLineFastOneX(uint16_t aStartX, uint16_t aStartY, uint16_t aEndY, color16_t aColor);
    void drawRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color);
    void fillRectRel(uint16_t aStartX, uint16_t aStartY, uint16_t aWidth, uint16_t aHeight, uint16_t aColor);

    void drawCircle(uint16_t aCenterX, uint16_t aCenterY, uint16_t aRadius, uint16_t aColor);
    void fillCircle(uint16_t aCenterX, uint16_t aCenterY, uint16_t aRadius, uint16_t aColor);

    void clearDisplay(uint16_t aColor);

    void printOptions(uint8_t size, uint16_t color, uint16_t bg_color);
    void printClear(void);
    void printXY(uint16_t x, uint16_t y);
    uint16_t printGetX(void);
    uint16_t printGetY(void);
    void printPGM(PGM_P s);
    virtual size_t write(uint8_t c);
    virtual size_t write(const char *s);
    virtual size_t write(const uint8_t *s, size_t size);

    static void reset(void);

private:
    // for (untested) print functions
    uint8_t p_size;
    uint16_t p_fg, p_bg;
    uint16_t p_x, p_y;

    static void wr_cmd(uint8_t reg, uint8_t param);
    static void wr_data(uint16_t data);
    static void wr_spi(uint8_t data);
};

extern MI0283QT2 LocalDisplay; // The instance provided by the class itself

#endif //_MI0283QT2_H
