/*
 * BDButton.h
 *
 * The BDButton object is just a 8 bit (16 bit) unsigned integer holding the remote index of the button.
 * So every pointer to a memory location holding the index of the button is a valid pointer to BDButton :-).
 *
 * The constants here must correspond to the values used in the BlueDisplay App
 *
 *  Copyright (C) 2015-2025  Armin Joachimsmeyer
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
 * Button position is upper left corner of button
 * If button color is COLOR16_NO_BACKGROUND only a text button without background is rendered
 */

#ifndef _BDBUTTON_H
#define _BDBUTTON_H

#include <stdint.h>

#if defined(ARDUINO)
#  if ! defined(ESP32)
// For not AVR platforms this contains mapping defines (at least for STM32)
#include <avr/pgmspace.h>
#  endif
#include "WString.h"    // for __FlashStringHelper
#endif

#if !defined(OMIT_BD_DEPRECATED_FUNCTIONS)
#define BUTTON_AUTO_RED_GREEN_FALSE_COLOR       COLOR16_RED
#define BUTTON_AUTO_RED_GREEN_VALUE_FOR_RED     false
#define BUTTON_AUTO_RED_GREEN_TRUE_COLOR        COLOR16_GREEN
#define BUTTON_AUTO_RED_GREEN_VALUE_FOR_GREEN   true
#endif
// New 5.0 macros
#define BUTTON_AUTO_TOGGLE_FALSE_COLOR       COLOR16_RED
#define BUTTON_AUTO_TOGGLE_VALUE_FOR_RED     false
#define BUTTON_AUTO_TOGGLE_TRUE_COLOR        COLOR16_GREEN
#define BUTTON_AUTO_TOGGLE_VALUE_FOR_GREEN   true


// Flags for BUTTON_GLOBAL_SETTINGS
static const int FLAG_BUTTON_GLOBAL_USE_DOWN_EVENTS_FOR_BUTTONS = 0x00; // Default
static const int FLAG_BUTTON_GLOBAL_USE_UP_EVENTS_FOR_BUTTONS = 0x01;   // If swipe can start on a button, you require this.
static const int FLAG_BUTTON_GLOBAL_SET_BEEP_TONE = 0x02; // Beep on button touch, TONE_CDMA_KEYPAD_VOLUME_KEY_LITE (89) is default tone

// Flags for init - must correspond to the values used at the BlueDisplay App
#define FLAG_BUTTON_NO_BEEP_ON_TOUCH        0x00
#define FLAG_BUTTON_DO_BEEP_ON_TOUCH        0x01 // Beep on this button touch
#if !defined(OMIT_BD_DEPRECATED_FUNCTIONS)
#define FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN   0x02 // Value true -> green, false -> red. Value toggling is generated by button.
#endif
#define FLAG_BUTTON_TYPE_TOGGLE             0x02 // Value true -> green, false -> red. Value toggling is generated by button.
#define FLAG_BUTTON_TYPE_AUTOREPEAT         0x04
#define FLAG_BUTTON_TYPE_MANUAL_REFRESH     0x08 // Button must be manually drawn after event. Makes only sense for Red/Green toggle button which should be invisible.
#if !defined(OMIT_BD_DEPRECATED_FUNCTIONS)
#define FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN_MANUAL_REFRESH (FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN | FLAG_BUTTON_TYPE_MANUAL_REFRESH) // Red/Green toggle button must be manually drawn after event to show new text/color
#endif
#define FLAG_BUTTON_TYPE_TOGGLE_MANUAL_REFRESH (FLAG_BUTTON_TYPE_TOGGLE | FLAG_BUTTON_TYPE_MANUAL_REFRESH) // Toggle button must be manually drawn after event to show new text/color
// Flags for local buttons
#define LOCAL_BUTTON_FLAG_IS_ACTIVE                     0x10 // Local button is to be checked by checkAllButtons() etc.
// LOCAL_BUTTON_FLAG_USE_BDBUTTON_FOR_CALLBACK is set, when we have a local and a remote button, i.e. SUPPORT_REMOTE_AND_LOCAL_DISPLAY is defined.
// Then only the remote button pointer is used as callback parameter to enable easy comparison of this parameter with a fixed button.
#define LOCAL_BUTTON_FLAG_USE_BDBUTTON_FOR_CALLBACK     0x20
#define LOCAL_BUTTON_FLAG_BUTTON_TEXT_IS_IN_PGMSPACE    0x40
#define LOCAL_BUTTON_FLAG_MASK                          0x70

#if defined(SUPPORT_LOCAL_DISPLAY)
#include "LocalGUI/LocalTouchButton.h"
#endif

/*
 *  The (remote) index of the button in the order of calling init()
 *  The BDButton object is just a 8 bit (16 bit) unsigned integer holding the remote index of the button.
 */
#if defined(__AVR__)
typedef uint8_t BDButtonIndex_t;
#else
typedef uint16_t BDButtonIndex_t;
#endif

extern BDButtonIndex_t sLocalButtonIndex; // local button index counter used by BDButton.init() and LocalTouchButton.createAllLocalButtonsAtRemote()

#include "Colors.h"

#ifdef __cplusplus
class BDButton {
public:

    struct BDButtonParameterStruct {
        uint16_t aPositionX;
        uint16_t aPositionY;
        uint16_t aWidthX;
        uint16_t aHeightY;
        color16_t aButtonColor;
        const char *aText;
        uint16_t aTextSize;
        uint8_t aFlags;
        int16_t aValue;
        void (*aOnTouchHandler)(BDButton*, int16_t);
    };

#if defined(__AVR__)
    // Same as before, but with aPGMText instead of aText
    struct BDButtonPGMTextParameterStruct {
        uint16_t aPositionX;
        uint16_t aPositionY;
        uint16_t aWidthX;
        uint16_t aHeightY;
        color16_t aButtonColor;
        const __FlashStringHelper *aPGMText;
        uint16_t aTextSize;
        uint8_t aFlags;
        int16_t aValue;
        void (*aOnTouchHandler)(BDButton*, int16_t);
    };
#endif

    // Constructors
    BDButton();
    BDButton(BDButtonIndex_t aButtonHandle);
    BDButton(const BDButton &aButton);

    // Operators
    bool operator==(const BDButton &aButton);
    bool operator!=(const BDButton &aButton);
#if defined(SUPPORT_LOCAL_DISPLAY)
    bool operator==(const LocalTouchButton &aButton);
    bool operator!=(const LocalTouchButton &aButton);
#endif

    void init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor,
            const char *aText, uint16_t aTextSize, uint8_t aFlags, int16_t aValue, void (*aOnTouchHandler)(BDButton*, int16_t));
    void init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor,
            const __FlashStringHelper *aPGMText, uint16_t aTextSize, uint8_t aFlags, int16_t aValue,
            void (*aOnTouchHandler)(BDButton*, int16_t));

    void init(BDButtonParameterStruct *aBDButtonParameter);
    static void setInitParameters(BDButtonParameterStruct *aBDButtonParameterStructToFill, uint16_t aPositionX, uint16_t aPositionY,
            uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor, const char *aText, uint16_t aTextSize, uint8_t aFlags,
            int16_t aValue, void (*aOnTouchHandler)(BDButton*, int16_t));
#if defined(__AVR__)
    void init(BDButtonPGMTextParameterStruct *aBDButtonParameter);
    static void setInitParameters(BDButtonPGMTextParameterStruct *aBDButtonParameterStructToFill, uint16_t aPositionX,
            uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor, const __FlashStringHelper *aPGMText,
            uint16_t aTextSize, uint8_t aFlags, int16_t aValue, void (*aOnTouchHandler)(BDButton*, int16_t));
#endif

    void deinit(); // is defined as dummy if SUPPORT_LOCAL_DISPLAY is not active
    void removeButton(color16_t aBackgroundColor); // Deactivates the button and redraws its screen space with aBackgroundColor
    void activate();
    void deactivate();

    void setCallback(void (*aOnTouchHandler)(BDButton*, int16_t));
    void setFlags(int16_t aFlags);
    /*
     * Autorepeat functions
     */
    void setButtonAutorepeatTiming(uint16_t aMillisFirstDelay, uint16_t aMillisFirstRate, uint16_t aFirstCount,
            uint16_t aMillisSecondRate);
    static void disableAutorepeatUntilEndOfTouch();

    // Global behavior
    static void setGlobalFlags(uint16_t aFlags); // FLAG_BUTTON_GLOBAL_USE_DOWN_EVENTS_FOR_BUTTONS, FLAG_BUTTON_GLOBAL_USE_UP_EVENTS_FOR_BUTTONS

    // TODO !!! Possible memory leak !!!
    static void resetAll();

    /*
     * Functions using the list of all buttons
     */
    static void activateAll();
    static void deactivateAll();

    // Position
    void setPosition(int16_t aPositionX, int16_t aPositionY);

    // Draw
    void drawButton();

    // Color
    void setButtonColor(color16_t aButtonColor);
    void setButtonColorAndDraw(color16_t aButtonColor);

    // Text
    void setButtonTextColor(color16_t aButtonTextColor);
    void setButtonTextColorAndDraw(color16_t aButtonTextColor);
    void setText(const char *aText, bool doDrawButton = false);
    void setText(const __FlashStringHelper *aPGMText, bool doDrawButton = false);
    void setTextForValueTrue(const char *aText);
    void setTextForValueTrue(const __FlashStringHelper *aText);
    void setTextFromStringArray(const char *const*aTextStringArrayPtr, uint8_t aStringIndex, bool doDrawButton = false);
    void setPGMTextFromPGMArray(const char *const*aPGMTextPGMArrayPtr, uint8_t aStringIndex, bool doDrawButton = false);

//#define OMIT_BD_DEPRECATED_FUNCTIONS // For testing :-)
#if !defined(OMIT_BD_DEPRECATED_FUNCTIONS)
    // deprecated
    void setCaption(const char *aText, bool doDrawButton = false) __attribute__ ((deprecated ("Renamed to setText")));
    void setCaption(const __FlashStringHelper *aPGMText, bool doDrawButton = false)
            __attribute__ ((deprecated ("Renamed to setText")));
    void setCaptionForValueTrue(const char *aText) __attribute__ ((deprecated ("Renamed to setTextForValueTrue")));
    void setCaptionForValueTrue(const __FlashStringHelper *aText) __attribute__ ((deprecated ("Renamed to setTextForValueTrue")));
    void setCaptionFromStringArray(const char *const*aTextStringArrayPtr, uint8_t aStringIndex, bool doDrawButton = false)
            __attribute__ ((deprecated ("Renamed to setTextFromStringArray")));
    void setCaptionFromStringArray(const __FlashStringHelper *const*aTextStringArrayPtr, uint8_t aStringIndex, bool doDrawButton =
            false) __attribute__ ((deprecated ("Renamed to setTextFromStringArray")));

#if defined(__AVR__)
    // deprecated
    void setCaptionPGM(const char *aPGMText, bool doDrawButton = false)
            __attribute__ ((deprecated ("Use setTextForValueTrue(const __FlashStringHelper *aText,...) with cast")));
    void setCaptionPGMForValueTrue(const char *aText)
            __attribute__ ((deprecated ("Use setText(const __FlashStringHelper *aText) with cast")));
    void setCaptionFromStringArrayPGM(const char *const aPGMTextStringArrayPtr[], uint8_t aStringIndex,
            bool doDrawButton = false)
                    __attribute__ ((deprecated ("Use setTextFromStringArray(const __FlashStringHelper *const *aTextStringArrayPtr,...) with cast")));
#endif // defined(__AVR__)

    static void activateAllButtons() __attribute__ ((deprecated ("Renamed to activateAll")));
    static void deactivateAllButtons() __attribute__ ((deprecated ("Renamed to deactivateAll")));
#endif

    // Value
    void setValue(int16_t aValue, bool doDrawButton = false);
    void setValueAndDraw(int16_t aValue);

    // Feedback tone
    static void setButtonsTouchTone(uint8_t aToneIndex, uint16_t aToneDuration);
    static void setButtonsTouchTone(uint8_t aToneIndex, uint16_t aToneDuration, uint8_t aToneVolume);
    static void playFeedbackTone();
    static void playFeedbackTone(bool aPlayErrorTone);

    BDButtonIndex_t mButtonIndex; // Index of button for BlueDisplay button functions 0 to n. Taken in init() from sLocalButtonIndex.

#if defined(SUPPORT_LOCAL_DISPLAY)
    LocalTouchButton *mLocalButtonPtr; // Pointer to the corresponding local button, which is allocated at init()
#endif

private:
};

///**
// * @brief Button Init Structure definition
// * This uses around 200 bytes and saves 8 to 24 bytes per button
// */
//struct ButtonInit {
//    uint16_t PositionX;
//    uint16_t PositionY;
//    uint16_t WidthX;
//    uint16_t HeightY;
//    color16_t ButtonColor;
//    uint16_t TextSize;
//    uint16_t Flags;
//    int16_t Value;
//    void (*aOnTouchHandler)(BDButton*, int16_t);
////    const __FlashStringHelper *PGMText;
//};
#endif // #ifdef __cplusplus

#endif //_BDBUTTON_H
