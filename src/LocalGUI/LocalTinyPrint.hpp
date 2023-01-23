/*
 * LocalTinyPrint.hpp
 * Implementation of tinyPrint for local display
 *
 *  Local display interface used:
 *      LocalDisplay.clearDisplay()
 *      LocalDisplay.drawChar()
 *      LOCAL_DISPLAY_WIDTH
 *
 *  Copyright (C) 2014-2023  Armin Joachimsmeyer
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

#ifndef _LOCAL_TINY_PRINT_HPP
#define _LOCAL_TINY_PRINT_HPP

#include "LocalGUI/LocalTinyPrint.h"

uint8_t sPrintSize = 1;
uint16_t sPrintColor = COLOR16_WHITE;
uint16_t sPrintBackgroundColor = COLOR16_BLACK;
int sPrintX = 0;
int sPrintY = 0;
bool sClearOnNewScreen = true;

void printSetOptions(uint8_t aPrintSize, uint16_t aPrintColor, uint16_t aPrintBackgroundColor,
bool aClearOnNewScreen) {
    sPrintSize = aPrintSize;
    sPrintColor = aPrintColor;
    sPrintBackgroundColor = aPrintBackgroundColor;
    sClearOnNewScreen = aClearOnNewScreen;
}

void printClearScreen() {
    LocalDisplay.clearDisplay(sPrintBackgroundColor);
    sPrintX = 0;
    sPrintY = 0;
}

/**
 *
 * @param aPosX in pixel coordinates
 * @param aPosY in pixel coordinates
 */
void printSetPosition(int aPosX, int aPosY) {
    sPrintX = aPosX;
    sPrintY = aPosY;
}

void printSetPositionColumnLine(int aColumnNumber, int aLineNumber) {
    sPrintX = aColumnNumber * TEXT_SIZE_11_WIDTH;
    if (sPrintX >= (DISPLAY_DEFAULT_WIDTH - TEXT_SIZE_11_WIDTH)) {
        sPrintX = 0;
    }
    sPrintY = aLineNumber * TEXT_SIZE_11_HEIGHT;
    if (sPrintY >= (DISPLAY_DEFAULT_HEIGHT - TEXT_SIZE_11_HEIGHT)) {
        sPrintY = 0;
    }
}

int printNewline() {
    int tPrintY = sPrintY + TEXT_SIZE_11_HEIGHT;
    if (tPrintY >= DISPLAY_DEFAULT_HEIGHT) {
        // wrap around to top of screen
        tPrintY = 0;
        if (sClearOnNewScreen) {
            LocalDisplay.clearDisplay(sPrintBackgroundColor);
        }
    }
    sPrintX = 0;
    return tPrintY;
}


/**
 * draw aNumberOfCharacters from string and clip at display border
 * @return uint16_t start x for next character - next x Parameter
 */
int drawNText(uint16_t x, uint16_t y, const char *s, int aNumberOfCharacters, uint8_t aTextSize, uint16_t aTextColor, uint16_t bg_color) {
    while (*s != 0 && --aNumberOfCharacters > 0) {
        x = LocalDisplay.drawChar(x, y, (char) *s++, aTextSize, aTextColor, bg_color);
        if (x > LOCAL_DISPLAY_WIDTH) {
            break;
        }
    }
    return x;
}

// need external StringBuffer to save RAM space
//extern char sStringBuffer[];
//int myPrintf(const char *aFormat, ...) {
//    va_list argp;
//    va_start(argp, aFormat);
//    int tLength = vsnprintf(sStringBuffer, sizeof sStringBuffer, aFormat, argp);
//    va_end(argp);
//    myPrint(sStringBuffer, tLength);
//    return tLength;
//}

/**
 * Prints string starting at actual display position and sets new position
 * Handles leading spaces, newlines and word wrap
 * @param StringPointer
 * @param aLength Maximal length of string to be printed
 */
extern "C" void myPrint(const char *StringPointer, int aLength) {
    char tChar;
    const char *tWordStart = StringPointer;
    const char *tPrintBufferStart = StringPointer;
    const char *tPrintBufferEnd = StringPointer + aLength;
    int tLineLengthInChars = DISPLAY_DEFAULT_WIDTH / (TEXT_SIZE_11_WIDTH * sPrintSize);
    bool doFlushAndNewline = false;
    int tColumn = sPrintX / TEXT_SIZE_11_WIDTH;
    while (true) {
        tChar = *StringPointer++;

        // check for terminate condition
        if (tChar == '\0' || StringPointer > tPrintBufferEnd) {
            // flush "buffer"
            sPrintX = drawNText(sPrintX, sPrintY, tPrintBufferStart, StringPointer - tPrintBufferStart, 0.6, sPrintColor,
                    sPrintBackgroundColor);
            // handling of newline - after end of string
            if (sPrintX >= DISPLAY_DEFAULT_WIDTH) {
                sPrintY = printNewline();
            }
            break;
        }
        if (tChar == '\n') {
            // new line -> start of a new word
            tWordStart = StringPointer;
            // signal flush and newline
            doFlushAndNewline = true;
        } else if (tChar == '\r') {
            // skip but start of a new word
            tWordStart = StringPointer;
        } else if (tChar == ' ') {
            // start of a new word
            tWordStart = StringPointer;
            if (tColumn == 0) {
                // skip from printing if first character in line
                tPrintBufferStart = StringPointer;
            }
        } else {
            if (tColumn >= (DISPLAY_DEFAULT_WIDTH / TEXT_SIZE_11_WIDTH)) {
                // character does not fit in line -> print it at next line
                doFlushAndNewline = true;
                int tWordlength = (StringPointer - tWordStart);
                if (tWordlength > tLineLengthInChars) {
                    //word too long for a line just print char on next line
                    // just draw "buffer" to old line, make newline and process character again
                    StringPointer--;
                } else {
                    // draw buffer till word start, make newline and process word again
                    StringPointer = tWordStart;
                }
            }
        }
        if (doFlushAndNewline) {
            drawNText(sPrintX, sPrintY, tPrintBufferStart, StringPointer - tPrintBufferStart, sPrintSize, sPrintColor,
                    sPrintBackgroundColor);
            tPrintBufferStart = StringPointer;
            sPrintY = printNewline();
            sPrintX = 0; // set it explicitly since compiler may hold sPrintX in register
            tColumn = 0;
            doFlushAndNewline = false;
        }
        tColumn += sPrintSize;
    }
}

#endif // _LOCAL_TINY_PRINT_HPP
