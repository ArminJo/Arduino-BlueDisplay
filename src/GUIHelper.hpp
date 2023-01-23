/*
 * GUIHelper.hpp
 *
 * Helper functions for text sizes etc.
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
#ifndef _GUI_HELPER_HPP
#define _GUI_HELPER_HPP

#include "GUIHelper.h"

//#define SUPPORT_ONLY_TEXT_SIZE_11_AND_22

/*
 * TextSize * 1,125 (* (1 + 1/8))
 */
uint16_t getTextHeight(uint16_t aTextSize) {
    if (aTextSize == 11) {
        return TEXT_SIZE_11_HEIGHT;
    }
#if defined(SUPPORT_ONLY_TEXT_SIZE_11_AND_22)
    return TEXT_SIZE_22_HEIGHT;
#else
    if (aTextSize == 22) {
        return TEXT_SIZE_22_HEIGHT;
    }
    return aTextSize + aTextSize / 8; // TextSize * 1,125
#endif
}

/*
 * Formula for Monospace Font on Android
 * TextSize * 0.6
 * Integer Formula (rounded): (TextSize *6)+4 / 10
 */
uint16_t getTextWidth(uint16_t aTextSize) {
    if (aTextSize == 11) {
        return TEXT_SIZE_11_WIDTH;
    }
#if defined(SUPPORT_ONLY_TEXT_SIZE_11_AND_22)
    return TEXT_SIZE_22_WIDTH;
#else
    if (aTextSize == 22) {
        return TEXT_SIZE_22_WIDTH;
    }
    return ((aTextSize * 6) + 4) / 10;
#endif
}

/*
 * Formula for Monospace Font on Android
 * float: TextSize * 0.76
 * int: (TextSize * 195 + 128) >> 8
 */
uint16_t getTextAscend(uint16_t aTextSize) {
    if (aTextSize == TEXT_SIZE_11) {
        return TEXT_SIZE_11_ASCEND;
    }
#if defined(SUPPORT_ONLY_TEXT_SIZE_11_AND_22)
    return TEXT_SIZE_22_ASCEND;
#else
    if (aTextSize == TEXT_SIZE_22) {
        return TEXT_SIZE_22_ASCEND;
    }
    uint32_t tRetvalue = aTextSize;
    tRetvalue = ((tRetvalue * 195) + 128) >> 8;
    return tRetvalue;
#endif
}

/*
 * Formula for Monospace Font on Android
 * float: TextSize * 0.24
 * int: (TextSize * 61 + 128) >> 8
 */
uint16_t getTextDecend(uint16_t aTextSize) {
    if (aTextSize == TEXT_SIZE_11) {
        return TEXT_SIZE_11_DECEND;
    }
#if defined(SUPPORT_ONLY_TEXT_SIZE_11_AND_22)
    return TEXT_SIZE_22_DECEND;
#else
    if (aTextSize == TEXT_SIZE_22) {
        return TEXT_SIZE_22_DECEND;
    }
    uint32_t tRetvalue = aTextSize;
    tRetvalue = ((tRetvalue * 61) + 128) >> 8;
    return tRetvalue;
#endif
}
/*
 * Ascend - Decent
 * is used to position text in the middle of a button
 * Formula for positioning:
 * Position = ButtonTop + (ButtonHeight + getTextAscendMinusDescend())/2
 */
uint16_t getTextAscendMinusDescend(uint16_t aTextSize) {
    if (aTextSize == TEXT_SIZE_11) {
        return TEXT_SIZE_11_ASCEND - TEXT_SIZE_11_DECEND;
    }
#if defined(SUPPORT_ONLY_TEXT_SIZE_11_AND_22)
    return TEXT_SIZE_22_ASCEND - TEXT_SIZE_22_DECEND;
#else
    if (aTextSize == TEXT_SIZE_22) {
        return TEXT_SIZE_22_ASCEND - TEXT_SIZE_22_DECEND;
    }
    uint32_t tRetvalue = aTextSize;
    tRetvalue = ((tRetvalue * 133) + 128) >> 8;
    return tRetvalue;
#endif
}

/*
 * (Ascend -Decent)/2
 */
uint16_t getTextMiddle(uint16_t aTextSize) {
    if (aTextSize == TEXT_SIZE_11) {
        return (TEXT_SIZE_11_ASCEND - TEXT_SIZE_11_DECEND) / 2;
    }
#if defined(SUPPORT_ONLY_TEXT_SIZE_11_AND_22)
    return (TEXT_SIZE_22_ASCEND - TEXT_SIZE_22_DECEND) / 2;
#else
    if (aTextSize == TEXT_SIZE_22) {
        return (TEXT_SIZE_22_ASCEND - TEXT_SIZE_22_DECEND) / 2;
    }
    uint32_t tRetvalue = aTextSize;
    tRetvalue = ((tRetvalue * 66) + 128) >> 8;
    return tRetvalue;
#endif
}

/*
 * Fast divide by 11 for MI0283QT2 driver arguments
 */
uint16_t getFontScaleFactorFromTextSize(uint16_t aTextSize) {
    if (aTextSize <= 11) {
        return 1;
    }
#if defined(SUPPORT_ONLY_TEXT_SIZE_11_AND_22)
    return 2;
#else
    if (aTextSize == 22) {
        return 2;
    }
    return aTextSize / 11;
#endif
}
#endif // _GUI_HELPER_HPP
