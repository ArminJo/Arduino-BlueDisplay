/*
 * LocalDisplayInterface.h
 *
 *
 *  Copyright (C) 2023  Armin Joachimsmeyer
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

#ifndef _LOCAL_DISPLAY_INTERFACE_H
#define _LOCAL_DISPLAY_INTERFACE_H

#include "Colors.h" // for color16_t
#include "fonts.h"
#include "GUIHelper.h"

#include <stdint.h>
#include <stdbool.h>

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Display
 * @{
 */

class LocalDisplayInterface { // @suppress("Class has a virtual method and non-virtual destructor")
public:

    LocalDisplayInterface();
#if !defined(ARDUINO)
    virtual ~LocalDisplayInterface(); // Destructor requires up to 600 additional bytes of program memory
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
// @formatter:off
    /*
     * Basic hardware functions required for the draw functions below
     */
    virtual void fillRect(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor) {/* Empty implementation for backward compatibility */};
//    virtual void fillRectRel(uint16_t aStartX, uint16_t aStartY, uint16_t aWidth, uint16_t aHeight, uint16_t aColor) {};
    virtual void setArea(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY) {};
    virtual void drawStart() {};
    virtual void draw(color16_t aColor) {};
    virtual void drawStop() {};

    /*
     * Sensible virtual interface functions
     */
//    virtual void clearDisplay(color16_t aColor) {};
//    virtual void drawCircle(uint16_t aCenterX, uint16_t aCenterY, uint16_t aRadius, uint16_t aColor) {};
//    virtual void fillCircle(uint16_t aCenterX, uint16_t aCenterY, uint16_t aRadius, uint16_t aColor) {};
    // not really used, but consuming RAM and program memory
//    virtual void drawPixel(uint16_t x0, uint16_t y0, uint16_t color) {}; // Just the unused entry costs 2 bytes RAM and 160 bytes program memory
//    virtual void drawPixelFast(uint16_t x0, uint8_t y0, uint16_t color) {};
//    virtual void drawLine(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor) {}; // Just the unused entry costs 2 bytes RAM and 70 bytes program memory
//    virtual void drawLineRel(uint16_t aStartX, uint16_t aStartY, int16_t aDeltaX, int16_t aDeltaY, color16_t aColor) {};
//    virtual void drawLineFastOneX(uint16_t aStartX, uint16_t aStartY, uint16_t aEndY, color16_t aColor) {};
//    virtual void drawRect(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color) {};
//
//    virtual void setBacklightBrightness(uint8_t aBrightness) {}; // Just the unused entry costs 2 bytes RAM and 22 bytes program memory

// @formatter:on
#pragma GCC diagnostic pop

    uint16_t drawChar(uint16_t aPositionX, uint16_t aPositionY, char aChar, uint8_t aFontScaleFactor, color16_t aCharacterColor,
            color16_t aBackgroundColor);
    uint16_t drawText(uint16_t aPositionX, uint16_t aPositionY, const char *aText, uint8_t aFontSize, color16_t aTextColor,
            color16_t aBackgroundColor, uint16_t aNumberOfCharacters = 0xFFFF);
#if defined (AVR)
    uint16_t drawTextPGM(uint16_t aPositionX, uint16_t aPositionY, const char *aText, uint8_t aFontSize, color16_t aTextColor,
            color16_t aBackgroundColor, uint16_t aNumberOfCharacters = 0xFFFF);
    uint16_t drawMLTextPGM(uint16_t aPositionX, uint16_t aPositionY, const char *aMultiLineText, uint8_t aFontSize,
            uint16_t aTextColor, uint16_t aBackgroundColor);
    uint16_t drawMLTextPGM(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, const char *aMultiLineText,
            uint8_t aFontSize, uint16_t aTextColor, uint16_t aBackgroundColor);
#endif
    uint16_t drawMLText(uint16_t aPositionX, uint16_t aPositionY, const char *aMultiLineText, uint8_t aFontSize,
            uint16_t aTextColor, uint16_t aBackgroundColor);

};

/** @} */
/** @} */

#endif // _LOCAL_DISPLAY_INTERFACE_H
