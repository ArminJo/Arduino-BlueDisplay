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

/*
 * Basic hardware functions required from the device driver for the draw functions
 *
 * void fillRect(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY, color16_t aColor);
 * void fillRectRel(uint16_t aStartX, uint16_t aStartY, uint16_t aWidth, uint16_t aHeight, uint16_t aColor);
 * void setArea(uint16_t aStartX, uint16_t aStartY, uint16_t aEndX, uint16_t aEndY);
 * void drawStart();
 * void draw(color16_t aColor);
 * void drawStop();
 */

#ifndef _LOCAL_DISPLAY_INTERFACE_H
#define _LOCAL_DISPLAY_INTERFACE_H

#include "BlueDisplay.h" // for __FlashStringHelper, SUPPORT_REMOTE_AND_LOCAL_DISPLAY etc.

#include "Colors.h" // for color16_t
#include "GUIHelper.h"

#include <stdint.h>
#include <stdbool.h>

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Display
 * @{
 */

#if defined(USE_HX8347D)
#include "HX8347D.h" // for LOCAL_DISPLAY_HEIGHT and LOCAL_DISPLAY_WIDTH
class LocalDisplayInterface: public HX8347D // @suppress("Class has a virtual method and non-virtual destructor")
#elif defined(USE_SSD1289)
#include "SSD1289.h" // for LOCAL_DISPLAY_HEIGHT and LOCAL_DISPLAY_WIDTH
class LocalDisplayInterface: public SSD1289  // @suppress("Class has a virtual method and non-virtual destructor")
#else
#error One of USE_HX8347D or USE_SSD1289 must be defined. Use e.g. #include "LocalHX8347DDisplay.hpp"
#endif
{
public:

    /*
     * Using this class as an interface with virtual functions costs around 1 k of program space.
     * It is the overhead for accessing the virtual functions.
     * The compiler option -fdevirtualize  -fdevirtualize-speculatively has no effect for the Arduino Compiler version used.
     */
    LocalDisplayInterface();
#if !defined(ARDUINO)
    virtual ~LocalDisplayInterface(); // Destructor requires up to 600 additional bytes of program memory
#endif

    uint16_t drawChar(uint16_t aPositionX, uint16_t aPositionY, char aChar, uint8_t aFontScaleFactor, color16_t aCharacterColor,
            color16_t aBackgroundColor);
    uint16_t drawText(uint16_t aPositionX, uint16_t aPositionY, const char *aText, uint8_t aFontSize, color16_t aTextColor,
            color16_t aBackgroundColor, uint16_t aNumberOfCharacters = 0xFFFF);
    uint16_t drawText(uint16_t aPositionX, uint16_t aPositionY, const __FlashStringHelper *aPGMString, uint16_t aFontSize,
            color16_t aTextColor, color16_t aBackgroundColor, uint16_t aNumberOfCharacters = 0xFFFF);
    uint16_t drawMLText(uint16_t aPositionX, uint16_t aPositionY, const char *aMultiLineText, uint8_t aFontSize, uint16_t aTextColor,
            uint16_t aBackgroundColor, bool isPGMText = false);
    uint16_t drawMLText(uint16_t aPositionX, uint16_t aPositionY, const __FlashStringHelper *aPGMMultiLineText, uint8_t aFontSize,
            uint16_t aTextColor, uint16_t aBackgroundColor);

};

extern LocalDisplayInterface LocalDisplay; // The instance provided by the class itself

/** @} */
/** @} */

#endif // _LOCAL_DISPLAY_INTERFACE_H
