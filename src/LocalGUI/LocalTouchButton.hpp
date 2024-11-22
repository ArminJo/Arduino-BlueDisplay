/*
 * LocalTouchButton.hpp
 *
 * Renders touch buttons for locally attached LCD screens with SSD1289 (on HY32D board) or HX8347 (on MI0283QT2 board) controller
 * A button can be a simple clickable text
 * or a box with or without text or even an invisible touch area
 *
 *  Local display interface used:
 *      LocalDisplay.fillRectRel()
 *      LocalDisplay.drawText()
 *      LOCAL_DISPLAY_WIDTH
 *      LOCAL_DISPLAY_HEIGHT
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
 */

/*  Ram usage: 7 byte + 24 bytes per button
 *  Code size: 1,5 kByte
 */

#ifndef _LOCAL_TOUCH_BUTTON_HPP
#define _LOCAL_TOUCH_BUTTON_HPP

#include "LocalGUI/LocalTouchButton.h"
#include "BDButton.h"
#include "BlueDisplay.h" // for FEEDBACK_TONE_OK
#if !defined(DISABLE_REMOTE_DISPLAY)
#include "LocalGUI/LocalTouchButtonAutorepeat.h"
#endif
/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Button
 * @{
 */

/*
 * List for TouchButton(Autorepeat) objects
 */
LocalTouchButton *LocalTouchButton::sButtonListStart = NULL; // Start of list of touch buttons, required for the *AllButtons functions
color16_t LocalTouchButton::sDefaultTextColor = TOUCHBUTTON_DEFAULT_TEXT_COLOR;

/**
 * Constructor - insert in list
 */
LocalTouchButton::LocalTouchButton() { // @suppress("Class members should be properly initialized")
    mTextForTrue = NULL; // moving this into init() costs 100 bytes
    mNextObject = NULL;
    if (sButtonListStart == NULL) {
        // first button
        sButtonListStart = this;
    } else {
        // put object in button list
        LocalTouchButton *tButtonPointer = sButtonListStart;
        // search last list element
        while (tButtonPointer->mNextObject != NULL) {
            tButtonPointer = tButtonPointer->mNextObject;
        }
        //insert actual button as last element
        tButtonPointer->mNextObject = this;
    }
}

bool LocalTouchButton::operator==(const LocalTouchButton &aButton) {
    return (this == &aButton);
}

bool LocalTouchButton::operator!=(const LocalTouchButton &aButton) {
    return (this != &aButton);
}

#if !defined(DISABLE_REMOTE_DISPLAY)
/*
 * Required for creating a local button for an existing aBDButton at BDButton::init
 */
LocalTouchButton::LocalTouchButton(BDButton *aBDButtonPtr) { // @suppress("Class members should be properly initialized")
    mTextForTrue = NULL;
    mBDButtonPtr = aBDButtonPtr;
    mNextObject = NULL;
    if (sButtonListStart == NULL) {
        // first button
        sButtonListStart = this;
    } else {
        // put object in button list
        LocalTouchButton *tButtonPointer = sButtonListStart;
        // search last list element
        while (tButtonPointer->mNextObject != NULL) {
            tButtonPointer = tButtonPointer->mNextObject;
        }
        //insert actual button as last element
        tButtonPointer->mNextObject = this;
    }
}
#endif

#if !defined(ARDUINO)
/**
 * Destructor  - remove from button list
 */
LocalTouchButton::~LocalTouchButton() {
    LocalTouchButton *tButtonPointer = sButtonListStart;
    if (tButtonPointer == this) {
        // remove first element of list
        sButtonListStart = NULL;
    } else {
        // walk through list to find previous element
        while (tButtonPointer != NULL) {
            if (tButtonPointer->mNextObject == this) {
                tButtonPointer->mNextObject = this->mNextObject;
                break;
            }
            tButtonPointer = tButtonPointer->mNextObject;
        }
    }
}
#endif

#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
/*
 * Used by event handler to create a temporary BDButton for setting the value and as calling parameter for the local callback function
 */
LocalTouchButton* LocalTouchButton::getLocalTouchButtonFromBDButtonHandle(BDButtonHandle_t aButtonHandleToSearchFor) {
    LocalTouchButton *tButtonPointer = sButtonListStart;
// walk through list
    while (tButtonPointer != NULL) {
        if (tButtonPointer->mBDButtonPtr->mButtonHandle == aButtonHandleToSearchFor) {
            return tButtonPointer;
        }
        tButtonPointer = tButtonPointer->mNextObject;
    }
    return NULL;
}

/**
 * Is called once after reconnect, to build up a remote copy of all local buttons
 * Handles also mTextForTrue and autorepeat buttons
 */
void LocalTouchButton::createAllLocalButtonsAtRemote() {
    if (USART_isBluetoothPaired()) {
        LocalTouchButton *tButtonPointer = sButtonListStart;
        sLocalButtonIndex = 0;
// walk through list
        while (tButtonPointer != NULL) {
            // cannot use BDButton.init since this allocates a new TouchButton
            sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 11, tButtonPointer->mBDButtonPtr->mButtonHandle,
                    tButtonPointer->mPositionX, tButtonPointer->mPositionY, tButtonPointer->mWidthX, tButtonPointer->mHeightY,
                    tButtonPointer->mButtonColor, tButtonPointer->mTextSize, tButtonPointer->mFlags & ~(LOCAL_BUTTON_FLAG_MASK),
                    tButtonPointer->mValue, tButtonPointer->mOnTouchHandler,
                    (reinterpret_cast<uint32_t>(tButtonPointer->mOnTouchHandler) >> 16), strlen(tButtonPointer->mText),
                    tButtonPointer->mText);
            if (tButtonPointer->mTextForTrue != NULL) {
                tButtonPointer->setTextForValueTrue(tButtonPointer->mTextForTrue);
            }
            if (tButtonPointer->mFlags & FLAG_BUTTON_TYPE_AUTOREPEAT) {
                LocalTouchButtonAutorepeat *tAutorepeatButtonPointer = (LocalTouchButtonAutorepeat*) tButtonPointer;
                sendUSARTArgs(FUNCTION_BUTTON_SETTINGS, 7, tAutorepeatButtonPointer->mBDButtonPtr->mButtonHandle,
                SUBFUNCTION_BUTTON_SET_AUTOREPEAT_TIMING, tAutorepeatButtonPointer->mMillisFirstDelay,
                        tAutorepeatButtonPointer->mMillisFirstRate, tAutorepeatButtonPointer->mFirstCount,
                        tAutorepeatButtonPointer->mMillisSecondRate);
            }
            sLocalButtonIndex++;
            tButtonPointer = tButtonPointer->mNextObject;
        }
    }
}
#endif

/**
 * Set parameters for touch button
 * @param aWidthX       if == 0 render only text no background box
 * @param aTextSize  if == 0 don't render anything, just check touch area -> transparent button ;-)
 * @param aFlags        Like FLAG_BUTTON_DO_BEEP_ON_TOUCH | LOCAL_BUTTON_FLAG_BUTTON_TEXT_IS_IN_PGMSPACE
 * @param aValue        true -> green, false -> red
 */
int8_t LocalTouchButton::init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor,
        const char *aText, uint8_t aTextSize, uint8_t aFlags, int16_t aValue, void (*aOnTouchHandler)(LocalTouchButton*, int16_t)) {

    mWidthX = aWidthX;
    mHeightY = aHeightY;
    mTextColor = sDefaultTextColor;
    mText = aText;
    mTextSize = aTextSize;
    mOnTouchHandler = aOnTouchHandler;
    mFlags = aFlags;
    mButtonColor = aButtonColor;
    mValue = aValue;

    return setPosition(aPositionX, aPositionY);
}

int8_t LocalTouchButton::init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor,
        const __FlashStringHelper *aPGmText, uint8_t aTextSize, uint8_t aFlags, int16_t aValue,
        void (*aOnTouchHandler)(LocalTouchButton*, int16_t)) {
#if defined(__AVR__)
    mWidthX = aWidthX;
    mHeightY = aHeightY;
    mTextColor = sDefaultTextColor;
    mText = reinterpret_cast<const char*>(aPGmText);
    mTextSize = aTextSize;
    mOnTouchHandler = aOnTouchHandler;
    mFlags = aFlags | LOCAL_BUTTON_FLAG_BUTTON_TEXT_IS_IN_PGMSPACE;
    mButtonColor = aButtonColor;
    mValue = aValue;

    return setPosition(aPositionX, aPositionY);
#else
    return init(aPositionX, aPositionY, aWidthX, aHeightY, aButtonColor, reinterpret_cast<const char*>(aPGmText), aTextSize, aFlags,
            aValue, aOnTouchHandler);
#endif
}
// Dummy to be more compatible with BDButton
void LocalTouchButton::deinit() {
}

int8_t LocalTouchButton::setPosition(uint16_t aPositionX, uint16_t aPositionY) {
    int8_t tRetValue = 0;
    mPositionX = aPositionX;
    mPositionY = aPositionY;

    // check values
    if (aPositionX + mWidthX > LOCAL_DISPLAY_WIDTH) {
        mWidthX = LOCAL_DISPLAY_WIDTH - aPositionX;
#if !defined(ARDUINO)
        failParamMessage(aPositionX + mWidthX, "XRight wrong");
#endif
        tRetValue = TOUCHBUTTON_ERROR_X_RIGHT;
    }
    if (aPositionY + mHeightY > LOCAL_DISPLAY_HEIGHT) {
        mHeightY = LOCAL_DISPLAY_HEIGHT - aPositionY;
#if !defined(ARDUINO)
        failParamMessage(aPositionY + mHeightY, "YBottom wrong");
#endif
        tRetValue = TOUCHBUTTON_ERROR_Y_BOTTOM;
    }
    return tRetValue;
}

void LocalTouchButton::setDefaultCaptionColor(color16_t aDefaultTextColor) {
    sDefaultTextColor = aDefaultTextColor;
}
void LocalTouchButton::setDefaultTextColor(color16_t aDefaultTextColor) {
    sDefaultTextColor = aDefaultTextColor;
}

/**
 * Renders the button on lcd
 */
void LocalTouchButton::drawButton() {
    setColorForRedGreenButton(mValue);
    // Draw rect
    LocalDisplay.fillRectRel(mPositionX, mPositionY, mWidthX, mHeightY, mButtonColor);
    drawText();
}

/**
 * Deactivates the button and redraws its screen space with @a aBackgroundColor
 */
void LocalTouchButton::removeButton(color16_t aBackgroundColor) {
    mFlags &= ~LOCAL_BUTTON_FLAG_IS_ACTIVE;
    // Draw rect
    LocalDisplay.fillRectRel(mPositionX, mPositionY, mWidthX, mHeightY, aBackgroundColor);

}

/**
 * draws the text of a button
 * @return 0 or error number #TOUCHBUTTON_ERROR_TEXT_TOO_LONG etc.
 */
void LocalTouchButton::drawCaption() {
    drawText();
}
void LocalTouchButton::drawText() {
    mFlags |= LOCAL_BUTTON_FLAG_IS_ACTIVE;

    auto tText = mText;
    if (mFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
        // Handle text for red green button
        if (mValue && mTextForTrue != NULL) {
            tText = mTextForTrue;
        }
    }
    if (mTextSize > 0) { // dont render anything if text size == 0
        if (tText != NULL) {
            uint16_t tXTextPosition;
            uint16_t tYTextPosition;
            /*
             * Simple 2 line handling
             * 1. position first string in the middle of the box
             * 2. draw second string just below
             */
            auto tTextHeight = getTextHeight(mTextSize);
            const char *tPosOfNewline;
            size_t tStringlengthOfText;
#if defined (AVR)
            if (mFlags & LOCAL_BUTTON_FLAG_BUTTON_TEXT_IS_IN_PGMSPACE) {
                tPosOfNewline = strchr_P(mText, '\n');
                tStringlengthOfText = strlen_P(mText);
            } else {
                tPosOfNewline = strchr(mText, '\n');
                tStringlengthOfText = strlen(mText);
            }
#else
            tPosOfNewline = strchr(tText, '\n');
            tStringlengthOfText = strlen(tText);
#endif
            size_t tStringlength = tStringlengthOfText;
            if (tPosOfNewline != NULL) {
                // assume 2 lines of Text
                tTextHeight = 2 * getTextHeight(mTextSize);
                tStringlength = (tPosOfNewline - tText);
            }
            auto tLength = getTextWidth(mTextSize) * tStringlength;
            // try to position the string in the middle of the box
            if (tLength >= mWidthX) {
                // String too long here
                tXTextPosition = mPositionX;
            } else {
                tXTextPosition = mPositionX + ((mWidthX - tLength) / 2);
            }

            if (tTextHeight >= mHeightY) {
                // Font height to big
                tYTextPosition = mPositionY;
            } else {
                tYTextPosition = mPositionY + ((mHeightY - tTextHeight) / 2);
            }
#if defined (AVR)
            if (mFlags & LOCAL_BUTTON_FLAG_BUTTON_TEXT_IS_IN_PGMSPACE) {
                LocalDisplay.drawText(tXTextPosition, tYTextPosition, reinterpret_cast<const __FlashStringHelper*>(tText),
                        mTextSize, mTextColor, mButtonColor, tStringlength);
            } else {
                LocalDisplay.drawText(tXTextPosition, tYTextPosition, tText, mTextSize, mTextColor, mButtonColor,
                        tStringlength);
            }
#else
            LocalDisplay.drawText(tXTextPosition, tYTextPosition, tText, mTextSize, mTextColor, mButtonColor, tStringlength);
#endif
            if (tPosOfNewline != NULL) {
                // 2. line - position the string in the middle of the box again
                tStringlength = tStringlengthOfText - tStringlength - 1;
                tLength = getTextWidth(mTextSize) * tStringlength;
                tXTextPosition = mPositionX + ((mWidthX - tLength) / 2);
#if defined (AVR)
                if (mFlags & LOCAL_BUTTON_FLAG_BUTTON_TEXT_IS_IN_PGMSPACE) {
                    LocalDisplay.drawText(tXTextPosition, tYTextPosition + getTextHeight(mTextSize),
                            (const __FlashStringHelper*) (tPosOfNewline + 1), mTextSize, mTextColor, mButtonColor,
                            tStringlength);
                } else {
                    LocalDisplay.drawText(tXTextPosition, tYTextPosition + getTextHeight(mTextSize), (tPosOfNewline + 1),
                            mTextSize, mTextColor, mButtonColor, tStringlength);
                }
#else
                LocalDisplay.drawText(tXTextPosition, tYTextPosition + getTextHeight(mTextSize), (tPosOfNewline + 1), mTextSize,
                        mTextColor, mButtonColor, tStringlength);
#endif
            }
        }
    }
}

bool LocalTouchButton::isAutorepeatButton() {
    return (mFlags & FLAG_BUTTON_TYPE_AUTOREPEAT);
}

void LocalTouchButton::playFeedbackTone() {
#if defined(ARDUINO)
#  if defined(LOCAL_GUI_FEEDBACK_TONE_PIN)
    tone(LOCAL_GUI_FEEDBACK_TONE_PIN, 3000, 50);
#  endif
#else
    tone(3000, 50);
#endif
}

void LocalTouchButton::playFeedbackTone(bool aPlayErrorTone) {
#if defined(ARDUINO)
#  if defined(LOCAL_GUI_FEEDBACK_TONE_PIN)
    if (aPlayErrorTone) {
        // two short beeps
        tone(LOCAL_GUI_FEEDBACK_TONE_PIN, 3000, 50);
        delay(100);
        tone(LOCAL_GUI_FEEDBACK_TONE_PIN, 3000, 50);
    } else {
        tone(LOCAL_GUI_FEEDBACK_TONE_PIN, 3000, 50);
    }
#  endif
#else
    if (aPlayErrorTone) {
// two short beeps
        tone(3000, 50);
        delay(60);
        tone(3000, 50);
    } else {
        tone(3000, 50);
    }
#endif
}

/**
 * Performs the defined actions for a button:
 * - Play tone
 * - Toggle red green and optionally redraw
 * - Call callback handler
 */
void LocalTouchButton::performTouchAction() {
    /*
     * Touch position is in button - call callback function
     * Beep if requested, but not for autorepeat buttons, since they may be checked very often to create the repeat timing.
     */
    if ((mFlags & FLAG_BUTTON_DO_BEEP_ON_TOUCH) && !(mFlags & FLAG_BUTTON_TYPE_AUTOREPEAT)) {
        playFeedbackTone();
    }
    /*
     * Red green button handling
     */
    if (mFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
        // Toggle value and handle color for Red/Green button
        mValue = !mValue;
        if (!(mFlags & FLAG_BUTTON_TYPE_MANUAL_REFRESH)) {
#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
            this->mBDButtonPtr->setValueAndDraw(mValue); // Update also the remote button
#else
            drawButton(); // Only local button refresh to show new color (and text)
#endif
        }
    }

    /*
     * Call callback handler
     */
    if (mOnTouchHandler != NULL) {
#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
        // Call with mBDButtonPtr as parameter, only the autorepeatTouchHandler must be called with the local button here
        if (&LocalTouchButtonAutorepeat::autorepeatTouchHandler
                == (void (*)(LocalTouchButtonAutorepeat*, int16_t)) mOnTouchHandler) {
            mOnTouchHandler(this, 0); // Beep for autorepeat buttons is handled here, value is not required here
        } else {
            mOnTouchHandler((LocalTouchButton*) this->mBDButtonPtr, mValue);
        }
#else
        mOnTouchHandler(this, mValue); // In case of autorepeat button, this calls the autorepeatTouchHandler()
#endif
    }
}

/**
 * Check if touch event is in button area
 * @return  - true if position match, false else
 */
bool LocalTouchButton::isTouched(uint16_t aTouchPositionX, uint16_t aTouchPositionY) {
    return (mPositionX <= aTouchPositionX && aTouchPositionX <= (mPositionX + mWidthX) && mPositionY <= aTouchPositionY
            && aTouchPositionY <= (mPositionY + mHeightY));
}

/**
 * Searched for buttons, which are active
 * @param aSearchOnlyAutorepeatButtons if true search only autorepeat buttons (required for for autorepeat timing by cyclic checking)
 * @return NULL if no button found at the position
 */
LocalTouchButton* LocalTouchButton::find(unsigned int aTouchPositionX, unsigned int aTouchPositionY,
bool aSearchOnlyAutorepeatButtons) {
    LocalTouchButton *tButtonPointer = sButtonListStart;
// walk through list
    while (tButtonPointer != NULL) {
        // Always check autorepeat buttons
        if ((tButtonPointer->mFlags & LOCAL_BUTTON_FLAG_IS_ACTIVE)
                && (!aSearchOnlyAutorepeatButtons || (tButtonPointer->mFlags & FLAG_BUTTON_TYPE_AUTOREPEAT))) {
            if (tButtonPointer->isTouched(aTouchPositionX, aTouchPositionY)) {
                return tButtonPointer;
            }
        }
        tButtonPointer = tButtonPointer->mNextObject;
    }
    return NULL;
}

/**
 * @return NULL if no button found at this position
 */
LocalTouchButton* LocalTouchButton::findAndAction(unsigned int aTouchPositionX, unsigned int aTouchPositionY,
bool aCheckOnlyAutorepeatButtons) {
    LocalTouchButton *tButtonPointer = find(aTouchPositionX, aTouchPositionY, aCheckOnlyAutorepeatButtons);
    if (tButtonPointer != NULL) {
        tButtonPointer->performTouchAction();
    }
    return tButtonPointer;
}

/**
 * Static convenience method - checks all buttons for matching touch position and perform touch action if position match.
 */
bool LocalTouchButton::checkAllButtons(unsigned int aTouchPositionX, unsigned int aTouchPositionY,
bool aCheckOnlyAutorepeatButtons) {
    return (findAndAction(aTouchPositionX, aTouchPositionY, aCheckOnlyAutorepeatButtons) != NULL);
}

/**
 * Static convenience method - deactivate all buttons (e.g. before switching screen)
 */
void LocalTouchButton::deactivateAllButtons() {
    deactivateAll();
}
void LocalTouchButton::deactivateAll() {
    LocalTouchButton *tObjectPointer = sButtonListStart;
// walk through list
    while (tObjectPointer != NULL) {
        tObjectPointer->deactivate();
        tObjectPointer = tObjectPointer->mNextObject;
    }
}

/**
 * Static convenience method - activate all buttons
 */
void LocalTouchButton::activateAllButtons() {
    activateAll();
}
void LocalTouchButton::activateAll() {
    LocalTouchButton *tObjectPointer = sButtonListStart;
// walk through list
    while (tObjectPointer != NULL) {
        tObjectPointer->activate();
        tObjectPointer = tObjectPointer->mNextObject;
    }
}

#if defined(__AVR__)
/*
 * Not used yet
 */
uint8_t LocalTouchButton::getTextLength(char *aTextPointer) {
    uint8_t tLength = 0;
    uint8_t tFontWidth = 8;
    if (mTextSize > 11) {
        tFontWidth = 16;
    }
    if (mFlags & LOCAL_BUTTON_FLAG_BUTTON_TEXT_IS_IN_PGMSPACE) {
        while (pgm_read_byte(aTextPointer++) != 0) {
            tLength += (tFontWidth);
        }
    } else {
        while (*aTextPointer++ != 0) {
            tLength += (tFontWidth);
        }
    }
    return tLength;
}

#  if defined(DEBUG)
    /*
     * for debug purposes
     * needs char aStringBuffer[23+<TextLength>]
     */
    void LocalTouchButton::toString(char * aStringBuffer) const {
        sprintf(aStringBuffer, "X=%03u Y=%03u X1=%03u Y1=%03u B=%02u %s", mPositionX, mPositionY, mPositionX + mWidthX - 1,
                mPositionY + mHeightY - 1, mTouchBorder, mText);
    }
#  endif
#endif

void LocalTouchButton::setCaptionForValueTrue(const char *aText) {
    setTextForValueTrue(aText);
}
void LocalTouchButton::setTextForValueTrue(const char *aText) {
#if defined(__AVR__)
    mFlags &= ~LOCAL_BUTTON_FLAG_BUTTON_TEXT_IS_IN_PGMSPACE;
#endif
    mTextForTrue = aText;
}

void LocalTouchButton::setCaptionForValueTrue(const __FlashStringHelper *aPGmText) {
    setTextForValueTrue(aPGmText);
}
void LocalTouchButton::setTextForValueTrue(const __FlashStringHelper *aPGmText) {
#if defined (AVR)
    mFlags |= LOCAL_BUTTON_FLAG_BUTTON_TEXT_IS_IN_PGMSPACE;
    mTextForTrue = reinterpret_cast<const char*>(aPGmText);
#else
    setTextForValueTrue(reinterpret_cast<const char*>(aPGmText));
#endif
}

/*
 * Set text
 */
void LocalTouchButton::setCaption(const char *aText, bool doDrawButton) {
    setText(aText, doDrawButton);
}
void LocalTouchButton::setText(const char *aText, bool doDrawButton) {
#if defined(__AVR__)
    mFlags &= ~LOCAL_BUTTON_FLAG_BUTTON_TEXT_IS_IN_PGMSPACE;
#endif
    mText = aText;
    if (doDrawButton) {
        drawButton();
    }
}

void LocalTouchButton::setCaption(const __FlashStringHelper *aPGmText, bool doDrawButton) {
    setText(aPGmText, doDrawButton);
}
void LocalTouchButton::setText(const __FlashStringHelper *aPGmText, bool doDrawButton) {
#if defined (AVR)
    mFlags |= LOCAL_BUTTON_FLAG_BUTTON_TEXT_IS_IN_PGMSPACE;
    mText = reinterpret_cast<const char*>(aPGmText);
    if (doDrawButton) {
        drawButton();
    }
#else
    setText(reinterpret_cast<const char*>(aPGmText), doDrawButton);
#endif
}

/**
 * Set text and draw
 */
void LocalTouchButton::setCaptionAndDraw(const char *aText) {
    setTextAndDraw(aText);
}
void LocalTouchButton::setTextAndDraw(const char *aText) {
    mText = aText;
    drawButton();
}

/*
 * changes box color and redraws button
 */
void LocalTouchButton::setButtonColor(color16_t aButtonColor) {
    mButtonColor = aButtonColor;
}

void LocalTouchButton::setButtonColorAndDraw(color16_t aButtonColor) {
    mButtonColor = aButtonColor;
    this->drawButton();
}

void LocalTouchButton::setTextColor(color16_t aTextColor) {
    mTextColor = aTextColor;
}

/**
 * value 0 -> red, 1 -> green
 */
void LocalTouchButton::setColorForRedGreenButton(bool aValue) {
    if (mFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
        if (aValue) {
            mButtonColor = COLOR16_GREEN;
        } else {
            mButtonColor = COLOR16_RED;
        }
    }
}

void LocalTouchButton::setValue(int16_t aValue) {
    setColorForRedGreenButton(aValue);
    mValue = aValue;
}

void LocalTouchButton::setValueAndDraw(int16_t aValue) {
    setValue(aValue);
    drawButton();
}

int8_t LocalTouchButton::setPositionX(uint16_t aPositionX) {
    return setPosition(aPositionX, mPositionY);
}

int8_t LocalTouchButton::setPositionY(uint16_t aPositionY) {
    return setPosition(mPositionX, aPositionY);
}

uint16_t LocalTouchButton::getPositionXRight() const {
    return mPositionX + mWidthX - 1;
}

uint16_t LocalTouchButton::getPositionYBottom() const {
    return mPositionY + mHeightY - 1;
}

/*
 * activate for touch checking
 */
void LocalTouchButton::activate() {
    mFlags |= LOCAL_BUTTON_FLAG_IS_ACTIVE;
}

/*
 * deactivate for touch checking
 */
void LocalTouchButton::deactivate() {
    mFlags &= ~LOCAL_BUTTON_FLAG_IS_ACTIVE;
}

void LocalTouchButton::setTouchHandler(void (*aOnTouchHandler)(LocalTouchButton*, int16_t)) {
    mOnTouchHandler = aOnTouchHandler;
}

/** @} */
/** @} */
#endif // _LOCAL_TOUCH_BUTTON_HPP
