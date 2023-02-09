/*
 * LocalDisplayInterface.hpp
 * Implementation of simple device independent display control and draw functions for local display
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

#ifndef _LOCAL_DISPLAY_INTERFACE_HPP
#define _LOCAL_DISPLAY_INTERFACE_HPP

#include "LocalDisplayInterface.h"
#if defined(USE_HX8347D)
#include "HX8347D.hpp"   // The implementation of the local display must be included first since it defines LOCAL_DISPLAY_HEIGHT etc.
#elif defined(USE_SSD1289)
#include "SSD1289.hpp"  // The implementation of the local display must be included first since it defines LOCAL_DISPLAY_HEIGHT etc.
#endif

LocalDisplayInterface LocalDisplay; // The instance provided by the class itself

LocalDisplayInterface::LocalDisplayInterface() {  // @suppress("Class members should be properly initialized")
}
#if !defined(ARDUINO)
LocalDisplayInterface::~LocalDisplayInterface() { // Destructor requires up to 600 additional bytes of program memory
}
#endif
/**
 * Draw character in the rectangle starting upper left with aPositionX, aPositionY
 * and ending lower right at (aPositionX + FONT_WIDTH - 1), (aPositionY + FONT_HEIGHT - 1)
 * @param aPositionX left position
 * @param aPositionY upper position
 * @param bg_color start x for next character / x + (FONT_WIDTH * size)
 * @param aFontScaleFactor Scale factor for fixed font used
 *
 * @return aPositionX for next character
 */
uint16_t LocalDisplayInterface::drawChar(uint16_t aPositionX, uint16_t aPositionY, char aChar, uint8_t aFontScaleFactor,
        uint16_t aCharacterColor, uint16_t aBackgroundColor) {

#if !defined(AVR)
    /*
     * check if a draw in routine which uses setArea() is already executed
     */
    uint32_t tLock;
    do {
        tLock = __LDREXW(&sDrawLock);
        tLock++;
    } while (__STREXW(tLock, &sDrawLock));

    if (tLock != 1) {
        // here in ISR, but interrupted process was still in drawChar()
        sLockCount++;
        // first approach skip drawing and return input x value
        return aPositionX;
    }
#endif

    uint16_t tReturnValue;
#if FONT_WIDTH <= 8
    uint8_t tFontRawData, tBitMask;
    const uint8_t *tFontRawPointer;
#elif FONT_WIDTH <= 16
    uint16_t tFontRawData, tBitMask;
    const uint16_t *tFontRawPointer;
#elif FONT_WIDTH <= 32
    uint32_t tFontRawData, tBitMask;
    const uint32_t *tFontRawPointer;
#endif

// characters below 20 are not printable
    if (aChar < 0x20) {
        aChar = 0x20;
    }

#ifdef FONT_END7F
    aChar = aChar & 0x7F;  // mask highest bit
#endif
#if defined(AVR)
#if FONT_WIDTH <= 8
    tFontRawPointer = &font_PGM[(aChar - FONT_START) * (8 * FONT_HEIGHT / 8)];
#elif FONT_WIDTH <= 16
    tFontRawPointer = &font_PGM[(aChar - FONT_START) * (16 * FONT_HEIGHT / 8)];
#elif FONT_WIDTH <= 32
    tFontRawPointer = &font_PGM[(aChar - FONT_START) * (32 * FONT_HEIGHT / 8)];
#endif
#else
#if FONT_WIDTH <= 8
    tFontRawPointer = &font[(aChar - FONT_START) * (8 * FONT_HEIGHT / 8)];
#elif FONT_WIDTH <= 16
    tFontRawPointer = &font[(aChar - FONT_START) * (16 * FONT_HEIGHT / 8)];
#elif FONT_WIDTH <= 32
    tFontRawPointer = &font[(aChar - FONT_START) * (32 * FONT_HEIGHT / 8)];
#endif
#endif
    if (aFontScaleFactor <= 1) {
        /*
         * Draw font direct, no scale factor
         */

        tReturnValue = aPositionX + FONT_WIDTH;
        if ((aPositionY + FONT_HEIGHT) > LOCAL_DISPLAY_HEIGHT) {
            tReturnValue = LOCAL_DISPLAY_WIDTH + 1;
        }

        /*
         * Do not draw if X or Y overflow
         */
        if (tReturnValue <= LOCAL_DISPLAY_WIDTH) {
            setArea(aPositionX, aPositionY, (aPositionX + FONT_WIDTH - 1), (aPositionY + FONT_HEIGHT - 1));
            drawStart();
            for (uint8_t i = FONT_HEIGHT; i != 0; i--) {
#if defined(AVR)
#  if FONT_WIDTH <= 8
                tFontRawData = pgm_read_byte(tFontRawPointer);
                tFontRawPointer += 1;
#  elif FONT_WIDTH <= 16
                tFontRawData = pgm_read_word(tFontRawPointer);
                tFontRawPointer+=2;
#  elif FONT_WIDTH <= 32
                tFontRawData = pgm_read_dword(tFontRawPointer);
                tFontRawPointer+=4;
#endif
#else
                tFontRawData = *tFontRawPointer++;
#endif

                for (tBitMask = (1 << (FONT_WIDTH - 1)); tBitMask != 0; tBitMask >>= 1) {
                    if (tFontRawData & tBitMask) {
                        draw(aCharacterColor);
                    } else {
                        draw(aBackgroundColor);
                    }
                }
            }
            drawStop();
        }
    } else {
        /*
         * Draw font increased by scale factor
         */
        tReturnValue = aPositionX + (FONT_WIDTH * aFontScaleFactor);
        if ((aPositionY + (FONT_HEIGHT * aFontScaleFactor)) > LOCAL_DISPLAY_HEIGHT) {
            tReturnValue = LOCAL_DISPLAY_WIDTH + 1;
        }
        if (tReturnValue <= LOCAL_DISPLAY_WIDTH) {
            setArea(aPositionX, aPositionY, (aPositionX + (FONT_WIDTH * aFontScaleFactor) - 1),
                    (aPositionY + (FONT_HEIGHT * aFontScaleFactor) - 1));
            drawStart();
            for (uint8_t tFontLine = FONT_HEIGHT; tFontLine != 0; tFontLine--) {
#if defined(AVR)
#  if FONT_WIDTH <= 8
                tFontRawData = pgm_read_byte(tFontRawPointer);
                tFontRawPointer += 1;
#  elif FONT_WIDTH <= 16
                tFontRawData = pgm_read_word(tFontRawPointer);
                tFontRawPointer+=2;
#  elif FONT_WIDTH <= 32
                tFontRawData = pgm_read_dword(tFontRawPointer);
                tFontRawPointer+=4;
#endif
#else
                tFontRawData = *tFontRawPointer++;
#endif
                for (uint8_t i = aFontScaleFactor; i != 0; i--) {
                    for (tBitMask = (1 << (FONT_WIDTH - 1)); tBitMask != 0; tBitMask >>= 1) {
                        if (tFontRawData & tBitMask) {
                            for (uint8_t j = aFontScaleFactor; j != 0; j--) {
                                draw(aCharacterColor);
                            }
                        } else {
                            for (uint8_t j = aFontScaleFactor; j != 0; j--) {
                                draw(aBackgroundColor);
                            }
                        }
                    }
                }
            }
            drawStop();
        }
    }

#if !defined(AVR)
    sDrawLock = 0;
# endif

    return tReturnValue;
}

/**
 * Draw text with character upper left at aPositionX, aPositionY
 * Interpretation of aPositionY is different from the BlueDisplay functions, which use the aPositionY as baseline position,
 *    so use getTextAscend(<aTextSize>)) for aPositionX to start at the upper display border.
 * @param aPositionX left position
 * @param aPositionY upper position
 *
 * @return aPositionX for next character
 */
uint16_t LocalDisplayInterface::drawText(uint16_t aPositionX, uint16_t aPositionY, const char *aText, uint8_t aFontSize,
        uint16_t aTextColor, uint16_t aBackgroundColor, uint16_t aNumberOfCharacters) {

    uint16_t tPositionX = aPositionX;
    auto tNumberOfCharacters = strnlen(aText, aNumberOfCharacters);
    uint8_t tFontScaleFactor = getFontScaleFactorFromTextSize(aFontSize);

    while (tNumberOfCharacters-- != 0) {
        tPositionX = drawChar(tPositionX, aPositionY, *aText++, tFontScaleFactor, aTextColor, aBackgroundColor);
        if (tPositionX > LOCAL_DISPLAY_WIDTH) {
            break;
        }
    }
    return tPositionX;
}

uint16_t LocalDisplayInterface::drawText(uint16_t aPositionX, uint16_t aPositionY, const __FlashStringHelper *aPGMString,
        uint16_t aFontSize, color16_t aTextColor, color16_t aBackgroundColor, uint16_t aNumberOfCharacters) {
#if defined (AVR)
    uint16_t tPositionX = aPositionX;
    const char *tPGMString = reinterpret_cast<const char*>(aPGMString);
    auto tNumberOfCharacters = strnlen_P(tPGMString, aNumberOfCharacters);
    char tChar = pgm_read_byte(tPGMString++);
    uint8_t tFontScaleFactor = getFontScaleFactorFromTextSize(aFontSize);

    while (tNumberOfCharacters-- != 0) {
        tPositionX = drawChar(tPositionX, aPositionY, tChar, tFontScaleFactor, aTextColor, aBackgroundColor);
        if (tPositionX > LOCAL_DISPLAY_WIDTH) {
            break;
        }
        tChar = pgm_read_byte(tPGMString++);
    }
    return tPositionX;
#else
    return drawText(aPositionX, aPositionY, reinterpret_cast<const char*>(aPGMString), aFontSize, aTextColor, aBackgroundColor);
#endif

}

/**
 *
 * @param aPositionX left position
 * @param aPositionY upper position
 *
 * @return aPositiony for next text
 */
uint16_t LocalDisplayInterface::drawMLText(uint16_t aPositionX, uint16_t aPositionY, const char *aMultiLineText, uint8_t aFontSize,
        uint16_t aTextColor, uint16_t aBackgroundColor, bool isPGMText) {
    uint16_t tPositionX = aPositionX;
    uint16_t tPositionY = aPositionY;

    char tChar;
    auto tPointerToCurrentChar = aMultiLineText;
    uint8_t tFontScaleFactor = getFontScaleFactorFromTextSize(aFontSize);
    uint8_t tEffectiveFontHeight = FONT_HEIGHT * tFontScaleFactor;

    if (aBackgroundColor != COLOR16_NO_BACKGROUND) {
        // Clear first line until end of display (else only overwrite)
        fillRect(aPositionX, aPositionY, LOCAL_DISPLAY_WIDTH - 1, aPositionY + tEffectiveFontHeight - 1, aBackgroundColor);
    }

    uint16_t tMaximumNumberOfCharsInOneLine = (LOCAL_DISPLAY_WIDTH - aPositionX) / (FONT_WIDTH * tFontScaleFactor);
    const char *tWordStartPointer = tPointerToCurrentChar;
    while ((tPositionY + tEffectiveFontHeight) < LOCAL_DISPLAY_HEIGHT) {
#if defined(AVR)
        if (isPGMText) {
            tChar = pgm_read_byte(tPointerToCurrentChar++);
        } else
#endif
        {
            tChar = *tPointerToCurrentChar++;
        }

        if (tChar == '\0') {
            break; // end of string
        } else if (tChar == '\n') {
            // new line -> update position and optionally clear line
            tPositionX = aPositionX;
            tPositionY += tEffectiveFontHeight + 1;
            if (aBackgroundColor != COLOR16_NO_BACKGROUND) {
                // Clear next line until end of display (else only overwrite)
                fillRect(tPositionX, tPositionY, LOCAL_DISPLAY_WIDTH - 1, tPositionY + tEffectiveFontHeight - 1, aBackgroundColor);
            }
            continue;
        } else if (tChar == '\r') {
            // skip character
            continue;
        } else if (tChar == ' ') {
            //start of a new word
            tWordStartPointer = tPointerToCurrentChar;
//            const char *tWordEndPointer = strchrnul(tWordStartPointer,' ');
            // TODO compute if word fits in rest of line and print with drawText(, ..., length);
            if (tPositionX == aPositionX) {
                // skip space character if it is at start of line
                continue;
            }
        }

        if (tChar) {
            // check if we do not overflow at right display border
            if ((tPositionX + (FONT_WIDTH * tFontScaleFactor)) > LOCAL_DISPLAY_WIDTH - 1) {
                // Overflow, must start at new line
                if (tChar == ' ') {
                    // skip space character and start a new line
                    tPositionX = aPositionX;
                    tPositionY += tEffectiveFontHeight + 1;
                    fillRect(tPositionX, tPositionY, LOCAL_DISPLAY_WIDTH - 1, tPositionY + tEffectiveFontHeight - 1,
                            aBackgroundColor);
                } else {
                    uint16_t tWordLength = (tPointerToCurrentChar - tWordStartPointer);
                    if (tWordLength > tMaximumNumberOfCharsInOneLine) {
                        // word was too long to fit on one line, so just continue printing on next line

                        tPositionX = aPositionX;
                        tPositionY += tEffectiveFontHeight + 1;
                        fillRect(tPositionX, tPositionY, LOCAL_DISPLAY_WIDTH - 1, tPositionY + tEffectiveFontHeight - 1,
                                aBackgroundColor);

                        tPositionX = drawChar(tPositionX, tPositionY, tChar, tFontScaleFactor, aTextColor, aBackgroundColor);
                    } else {
                        //clear actual word in line and start on next line
                        fillRect(tPositionX - (tWordLength * FONT_WIDTH * tFontScaleFactor), tPositionY,
                        LOCAL_DISPLAY_WIDTH - 1, (tPositionY + tEffectiveFontHeight), aBackgroundColor);

                        tPositionX = aPositionX;
                        tPositionY += tEffectiveFontHeight + 1;
                        fillRect(tPositionX, tPositionY, LOCAL_DISPLAY_WIDTH - 1, tPositionY + tEffectiveFontHeight - 1,
                                aBackgroundColor);
                        tPointerToCurrentChar = tWordStartPointer;
                    }
                }
            } else {
                // Append character on current line
                tPositionX = drawChar(tPositionX, tPositionY, tChar, tFontScaleFactor, aTextColor, aBackgroundColor);
            }
        }
    }
    return tPositionY;
}

uint16_t LocalDisplayInterface::drawMLText(uint16_t aStartX, uint16_t aStartY, const __FlashStringHelper *aPGMMultiLineText,
        uint8_t aFontSize, uint16_t aTextColor, uint16_t aBackgroundColor) {
#if defined (AVR)
    return drawMLText(aStartX, aStartY, reinterpret_cast<const char*>(aPGMMultiLineText), aFontSize, aTextColor, aBackgroundColor, true); // Is PGM text
#else
    return drawMLText(aStartX, aStartY, reinterpret_cast<const char*>(aPGMMultiLineText), aFontSize, aTextColor, aBackgroundColor);
#endif // defined (AVR)
}

#endif // _LOCAL_DISPLAY_INTERFACE_HPP
