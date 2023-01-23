/*
 * LocalTinyPrint.h
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

#ifndef _LOCAL_TINY_PRINT_H
#define _LOCAL_TINY_PRINT_H

#include <stdint.h>
#include <stdbool.h>

void printSetOptions(uint8_t aPrintSize, uint16_t aPrintColor, uint16_t aPrintBackgroundColor, bool aClearOnNewScreen);
int printNewline(void);
void printClearScreen(void);
void printSetPosition(int aPosX, int aPosY);
void printSetPositionColumnLine(int aColumnNumber, int aLineNumber);
int printNewline(void);
//int myPrintf(const char *aFormat, ...);

#ifdef __cplusplus
extern "C" {
#endif
void myPrint(const char *StringPointer, int aLength);
#ifdef __cplusplus
}
#endif

#endif // _LOCAL_TINY_PRINT_H
