/*
 * LocalTouchButton.h
 *
 * Renders touch buttons for lcd
 * A button can be a simple clickable text
 * or a box with or without text
 *
 * 40 Bytes per button on 32Bit ARM
 * 22 Bytes per button on Arduino
 *
 *  Copyright (C) 2012-2024  Armin Joachimsmeyer
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

#ifndef _LOCAL_TOUCH_BUTTON_H
#define _LOCAL_TOUCH_BUTTON_H

#if !defined(DISABLE_REMOTE_DISPLAY)
class BDButton;

#  if defined(__AVR__)
typedef uint8_t BDButtonHandle_t;
#  else
typedef uint16_t BDButtonHandle_t;
#  endif
#endif

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Button
 * @{
 */

#define TOUCHBUTTON_DEFAULT_CAPTION_COLOR       COLOR16_BLACK
#define TOUCHBUTTON_DEFAULT_TEXT_COLOR          COLOR16_BLACK
// Error codes
#define TOUCHBUTTON_ERROR_X_RIGHT               -1
#define TOUCHBUTTON_ERROR_Y_BOTTOM              -2
// for documentation
#define ERROR_STRING_X_RIGHT                    "X right > LOCAL_DISPLAY_WIDTH"
#define ERROR_STRING_Y_BOTTOM                   "Y bottom > LOCAL_DISPLAY_HEIGHT"

// A copy from BDButton.h for documentation
//#define FLAG_BUTTON_NO_BEEP_ON_TOUCH        0x00
//#define FLAG_BUTTON_DO_BEEP_ON_TOUCH        0x01 // Beep on this button touch
//#define FLAG_BUTTON_TYPE_TOGGLE             0x02 // Value true -> green, false -> red. Value toggling is generated by button.
//#define FLAG_BUTTON_TYPE_AUTOREPEAT         0x04
//#define FLAG_BUTTON_TYPE_MANUAL_REFRESH     0x08 // Button must be manually drawn after event. Makes only sense for Red/Green toggle button which should be invisible.
//#define FLAG_BUTTON_TYPE_TOGGLE_MANUAL_REFRESH (FLAG_BUTTON_TYPE_TOGGLE | FLAG_BUTTON_TYPE_MANUAL_REFRESH) // Red/Green toggle button must be manually drawn after event to show new text/color
//// Flags for local buttons
//#define LOCAL_BUTTON_FLAG_IS_ACTIVE                     0x10 // Local button is to be checked by checkAllButtons() etc.
// LOCAL_BUTTON_FLAG_USE_BDBUTTON_FOR_CALLBACK is set, when we have a local and a remote button, i.e. SUPPORT_REMOTE_AND_LOCAL_DISPLAY is defined.
// Then only the remote button pointer is used as callback parameter to enable easy comparison of this parameter with a fixed BDButton.
//#define LOCAL_BUTTON_FLAG_USE_BDBUTTON_FOR_CALLBACK     0x20
//#define LOCAL_BUTTON_FLAG_BUTTON_TEXT_IS_IN_PGMSPACE    0x40
//#define LOCAL_BUTTON_FLAG_MASK                          0x70

class LocalTouchObject {
public:
    uint16_t mPositionX; // of upper left corner
    uint16_t mPositionY; // of upper left corner

    bool isTouched(uint16_t aTouchPositionX, uint16_t aTouchPositionY);
};

class LocalTouchButton: LocalTouchObject {
public:

    LocalTouchButton();
#if !defined(ARDUINO)
    ~LocalTouchButton();
#endif

    // Operators
    bool operator==(const LocalTouchButton &aButton);
    bool operator!=(const LocalTouchButton &aButton);

    int8_t init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor,
            const char *aText, uint8_t aTextSize, uint8_t aFlags, int16_t aValue,
            void (*aOnTouchHandler)(LocalTouchButton*, int16_t));
    int8_t init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor,
            const __FlashStringHelper *aPGMText, uint8_t aTextSize, uint8_t aFlags, int16_t aValue,
            void (*aOnTouchHandler)(LocalTouchButton*, int16_t));
    void deinit(); // Dummy to be more compatible with BDButton
    void activate();
    void deactivate();

#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
    static LocalTouchButton * getLocalTouchButtonFromBDButtonHandle(BDButtonHandle_t aButtonHandleToSearchFor);
    static void createAllLocalButtonsAtRemote();
#endif

    // Defaults
    static void setDefaultTouchBorder(uint8_t aDefaultTouchBorder);
    static void setDefaultCaptionColor(uint16_t aDefaultTextColor);
    static void setDefaultTextColor(uint16_t aDefaultTextColor);

    /*
     * Basic touch functions
     */
    bool isTouched(uint16_t aTouchPositionX, uint16_t aTouchPositionY);
    void performTouchAction();

    /*
     * Functions using the list of all buttons
     */
    static void activateAll();
    static void deactivateAll();
    static LocalTouchButton* findAndAction(unsigned int aTouchPositionX, unsigned int aTouchPositionY,
            bool aCheckOnlyAutorepeatButtons = false);
    static LocalTouchButton* find(unsigned int aTouchPositionX, unsigned int aTouchPositionY, bool aSearchOnlyAutorepeatButtons =
            false);
    static bool checkAllButtons(unsigned int aTouchPositionX, unsigned int aTouchPositionY,
            bool aCheckOnlyAutorepeatButtons = false);

    // Position
    int8_t setPosition(uint16_t aPositionX, uint16_t aPositionY);
    int8_t setPositionX(uint16_t aPositionX);
    int8_t setPositionY(uint16_t aPositionY);

    // Draw
    void drawButton();
    void removeButton(color16_t aBackgroundColor); // Deactivates the button and redraws its screen space with aBackgroundColor

    // Color
    void setButtonColor(color16_t aButtonColor);
    void setButtonColorAndDraw(color16_t aButtonColor);
    void setTextColor(color16_t aTextColor);

    // Text
#if !defined(OMIT_BD_DEPRECATED_FUNCTIONS)
    void drawCaption() __attribute__ ((deprecated ("Renamed to drawText")));
    void setCaption(const char *aText, bool doDrawButton = false) __attribute__ ((deprecated ("Renamed to setText")));
    void setCaption(const __FlashStringHelper *aPGMText, bool doDrawButton = false) __attribute__ ((deprecated ("Renamed to setText")));
    void setCaptionForValueTrue(const char *aText) __attribute__ ((deprecated ("Renamed to setTextForValueTrue")));
    void setCaptionForValueTrue(const __FlashStringHelper *aText) __attribute__ ((deprecated ("Renamed to setTextForValueTrue")));
    void setCaptionAndDraw(const char *aText) __attribute__ ((deprecated ("Renamed to setTextAndDraw")));
#endif

    void drawText();
    void setText(const char *aText, bool doDrawButton = false);
    void setText(const __FlashStringHelper *aPGMText, bool doDrawButton = false);
    void setTextForValueTrue(const char *aText);
    void setTextForValueTrue(const __FlashStringHelper *aText);
    void setTextAndDraw(const char *aText);

    // Value
    void setValue(int16_t aValue);
#if !defined(OMIT_BD_DEPRECATED_FUNCTIONS)
    void setColorForRedGreenButton(bool aValue) __attribute__ ((deprecated ("Renamed to setColorForToggleButton")));
#endif
    void setColorForToggleButton(bool aValue);
    void setValueAndDraw(int16_t aValue);

    // Feedback tone
    static void playFeedbackTone();
    static void playFeedbackTone(bool aPlayErrorTone);

    void setTouchHandler(void (*aOnTouchHandler)(LocalTouchButton*, int16_t));

    /**
     * Getter
     */
    uint16_t getPositionXRight() const;
    uint16_t getPositionYBottom() const;
    bool isAutorepeatButton();

#ifdef AVR
    uint8_t getCaptionLength(char *aTextPointer);
    uint8_t getTextLength(char *aTextPointer);
#  if defined(DEBUG)
    void toString(char *aStringBuffer) const;
#  endif
#endif

    static void activateAllButtons() __attribute__ ((deprecated ("Renamed to activateAll")));
    static void deactivateAllButtons() __attribute__ ((deprecated ("Renamed to deactivateAll")));

#if !defined(DISABLE_REMOTE_DISPLAY)
    LocalTouchButton(BDButton *aBDButtonPtr); // Constructor required for creating a (temporary) local button for an existing aBDButton at BDButton::init
    BDButton *mBDButtonPtr;
#endif

    color16_t mButtonColor;
    color16_t mTextColor;
//    uint16_t mPositionX; // of upper left corner
//    uint16_t mPositionY; // of upper left corner
    uint16_t mWidthX;
    uint16_t mHeightY;
    uint8_t mTextSize;
    uint8_t mFlags; //Flag for: Autorepeat type, allocated, Red/Green toggle button. For definition, see BDButton.h

    const char *mText; // Pointer to Text
    const char *mTextForTrue; // Pointer to text for true for Red/Green toggle button
    int16_t mValue;
    void (*mOnTouchHandler)(LocalTouchButton*, int16_t);

    static color16_t sDefaultTextColor;

    /*
     * Button list, required for the *AllButtons functions
     */
    static LocalTouchButton *sButtonListStart; // Start of list of touch buttons, required for the *AllButtons functions
    LocalTouchButton *mNextObject;

};

/** @} */
/** @} */

#endif // _LOCAL_TOUCH_BUTTON_H
