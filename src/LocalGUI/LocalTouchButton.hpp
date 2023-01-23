/*
 * LocalTouchButton.hpp
 *
 * Renders touch buttons for locally attached LCD screens with SSD1289 (an HY32D board) or HX8347 (on MI0283QT2 board) controller
 * A button can be a simple clickable text
 * or a box with or without text or even an invisible touch area
 *
 *  Local display interface used:
 *      LocalDisplay.fillRectRel()
 *      LocalDisplay.drawText()
 *      LOCAL_DISPLAY_WIDTH
 *      LOCAL_DISPLAY_HEIGHT
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
 */

/*  Ram usage: 7 byte + 24 bytes per button
 *  Code size: 1,5 kByte
 */

#ifndef _LOCAL_TOUCH_BUTTON_HPP
#define _LOCAL_TOUCH_BUTTON_HPP

#include "LocalGUI/LocalTouchButton.h"
#include "BDButton.h"
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
TouchButton *TouchButton::sButtonListStart = NULL; // Start of list of touch buttons, required for the *AllButtons functions
color16_t TouchButton::sDefaultCaptionColor = TOUCHBUTTON_DEFAULT_CAPTION_COLOR;

/**
 * Constructor - insert in list
 */
TouchButton::TouchButton() { // @suppress("Class members should be properly initialized")
    mCaptionForTrue = NULL; // moving this into init() costs 100 bytes
    mNextObject = NULL;
    if (sButtonListStart == NULL) {
        // first button
        sButtonListStart = this;
    } else {
        // put object in button list
        TouchButton *tButtonPointer = sButtonListStart;
        // search last list element
        while (tButtonPointer->mNextObject != NULL) {
            tButtonPointer = tButtonPointer->mNextObject;
        }
        //insert actual button as last element
        tButtonPointer->mNextObject = this;
    }
}

#if !defined(DISABLE_REMOTE_DISPLAY)
/*
 * Required for creating a local button for an existing aBDButton at BDButton::init
 */
TouchButton::TouchButton(BDButton *aBDButtonPtr) { // @suppress("Class members should be properly initialized")
    mCaptionForTrue = NULL;
    mBDButtonPtr = aBDButtonPtr;
    mNextObject = NULL;
    if (sButtonListStart == NULL) {
        // first button
        sButtonListStart = this;
    } else {
        // put object in button list
        TouchButton *tButtonPointer = sButtonListStart;
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
TouchButton::~TouchButton() {
    TouchButton *tButtonPointer = sButtonListStart;
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
TouchButton* TouchButton::getLocalButtonFromBDButtonHandle(BDButtonHandle_t aButtonHandleToSearchFor) {
    TouchButton *tButtonPointer = sButtonListStart;
// walk through list
    while (tButtonPointer != NULL) {
        if (tButtonPointer->mBDButtonPtr->mButtonHandle == aButtonHandleToSearchFor) {
            break;
        }
        tButtonPointer = tButtonPointer->mNextObject;
    }
    if (tButtonPointer->mFlags & FLAG_BUTTON_TYPE_AUTOREPEAT) {
        return (TouchButtonAutorepeat*) tButtonPointer;
    } else {
        return tButtonPointer;
    }
}

/**
 * Is called once after reconnect, to build up a remote copy of all local buttons
 * Handles also mCaptionForTrue and autorepeat buttons
 */
void TouchButton::createAllLocalButtonsAtRemote() {
    if (USART_isBluetoothPaired()) {
        TouchButton *tButtonPointer = sButtonListStart;
        sLocalButtonIndex = 0;
// walk through list
        while (tButtonPointer != NULL) {
            // cannot use BDButton.init since this allocates a new TouchButton
            sendUSARTArgsAndByteBuffer(FUNCTION_BUTTON_CREATE, 11, tButtonPointer->mBDButtonPtr->mButtonHandle,
                    tButtonPointer->mPositionX, tButtonPointer->mPositionY, tButtonPointer->mWidthX, tButtonPointer->mHeightY,
                    tButtonPointer->mButtonColor, tButtonPointer->mCaptionSize, tButtonPointer->mFlags & ~(LOCAL_BUTTON_FLAG_MASK),
                    tButtonPointer->mValue, tButtonPointer->mOnTouchHandler,
                    (reinterpret_cast<uint32_t>(tButtonPointer->mOnTouchHandler) >> 16), strlen(tButtonPointer->mCaption),
                    tButtonPointer->mCaption);
            if(tButtonPointer->mCaptionForTrue != NULL) {
                tButtonPointer->setCaptionForValueTrue(tButtonPointer->mCaptionForTrue);
            }
            if (tButtonPointer->mFlags & FLAG_BUTTON_TYPE_AUTOREPEAT) {
                TouchButtonAutorepeat *tAutorepeatButtonPointer = (TouchButtonAutorepeat*) tButtonPointer;
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
 * @param aCaptionSize  if == 0 don't render anything, just check touch area -> transparent button ;-)
 * @param aFlags        Like FLAG_BUTTON_DO_BEEP_ON_TOUCH | LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE
 * @param aValue        true -> green, false -> red
 */
int8_t TouchButton::init(uint16_t aPositionX, uint16_t aPositionY, uint16_t aWidthX, uint16_t aHeightY, color16_t aButtonColor,
        const char *aCaption, uint8_t aCaptionSize, uint8_t aFlags, int16_t aValue,
        void (*aOnTouchHandler)(TouchButton*, int16_t)) {

    mWidthX = aWidthX;
    mHeightY = aHeightY;
    mCaptionColor = sDefaultCaptionColor;
    mCaption = aCaption;
    mCaptionSize = aCaptionSize;
    mOnTouchHandler = aOnTouchHandler;
    mFlags = aFlags;
    mButtonColor = aButtonColor;
    mValue = aValue;

    return setPosition(aPositionX, aPositionY);
}

int8_t TouchButton::setPosition(uint16_t aPositionX, uint16_t aPositionY) {
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

void TouchButton::setDefaultCaptionColor(color16_t aDefaultCaptionColor) {
    sDefaultCaptionColor = aDefaultCaptionColor;
}

/**
 * Renders the button on lcd
 */
void TouchButton::drawButton() {
    setColorForRedGreenButton(mValue);
    // Draw rect
    LocalDisplay.fillRectRel(mPositionX, mPositionY, mWidthX, mHeightY, mButtonColor);
    drawCaption();
}

/**
 * deactivates the button and redraws its screen space with @a aBackgroundColor
 */
void TouchButton::removeButton(color16_t aBackgroundColor) {
    mFlags &= ~LOCAL_BUTTON_FLAG_IS_ACTIVE;
    // Draw rect
    LocalDisplay.fillRectRel(mPositionX, mPositionY, mWidthX, mHeightY, aBackgroundColor);

}

/**
 * draws the caption of a button
 * @return 0 or error number #TOUCHBUTTON_ERROR_CAPTION_TOO_LONG etc.
 */
void TouchButton::drawCaption() {
    mFlags |= LOCAL_BUTTON_FLAG_IS_ACTIVE;

    auto tCaption = mCaption;
    if (mFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
        // Handle caption for red green button
        if (mValue && mCaptionForTrue != NULL) {
            tCaption = mCaptionForTrue;
        }
    }
    if (mCaptionSize > 0) { // dont render anything if caption size == 0
        if (tCaption != NULL) {
            uint16_t tXCaptionPosition;
            uint16_t tYCaptionPosition;
            /*
             * Simple 2 line handling
             * 1. position first string in the middle of the box
             * 2. draw second string just below
             */
            auto tCaptionHeight = getTextHeight(mCaptionSize);
            const char *tPosOfNewline;
            size_t tStringlengthOfCaption;
#if defined (AVR)
            if (mFlags & LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE) {
                tPosOfNewline = strchr_P(mCaption, '\n');
                tStringlengthOfCaption = strlen_P(mCaption);
            } else {
                tPosOfNewline = strchr(mCaption, '\n');
                tStringlengthOfCaption = strlen(mCaption);
            }
#else
            tPosOfNewline = strchr(tCaption, '\n');
            tStringlengthOfCaption = strlen(tCaption);
#endif
            size_t tStringlength = tStringlengthOfCaption;
            if (tPosOfNewline != NULL) {
                // assume 2 lines of caption
                tCaptionHeight = 2 * getTextHeight(mCaptionSize);
                tStringlength = (tPosOfNewline - tCaption);
            }
            auto tLength = getTextWidth(mCaptionSize) * tStringlength;
            // try to position the string in the middle of the box
            if (tLength >= mWidthX) {
                // String too long here
                tXCaptionPosition = mPositionX;
            } else {
                tXCaptionPosition = mPositionX + ((mWidthX - tLength) / 2);
            }

            if (tCaptionHeight >= mHeightY) {
                // Font height to big
                tYCaptionPosition = mPositionY;
            } else {
                tYCaptionPosition = mPositionY + ((mHeightY - tCaptionHeight) / 2);
            }
#if defined (AVR)
            if (mFlags & LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE) {
                LocalDisplay.drawTextPGM(tXCaptionPosition, tYCaptionPosition, (char*) tCaption, mCaptionSize, mCaptionColor,
                        mButtonColor, tStringlength);
            } else {
                LocalDisplay.drawText(tXCaptionPosition, tYCaptionPosition, (char*) tCaption, mCaptionSize, mCaptionColor,
                        mButtonColor, tStringlength);
            }
#else
            LocalDisplay.drawText(tXCaptionPosition, tYCaptionPosition, (char*) tCaption, mCaptionSize,
                    mCaptionColor, mButtonColor, tStringlength);
#endif
            if (tPosOfNewline != NULL) {
                // 2. line - position the string in the middle of the box again
                tStringlength = tStringlengthOfCaption - tStringlength - 1;
                tLength = getTextWidth(mCaptionSize) * tStringlength;
                tXCaptionPosition = mPositionX + ((mWidthX - tLength) / 2);
#if defined (AVR)
                if (mFlags & LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE) {
                    LocalDisplay.drawTextPGM(tXCaptionPosition, tYCaptionPosition + getTextHeight(mCaptionSize),
                            (tPosOfNewline + 1), mCaptionSize, mCaptionColor, mButtonColor, tStringlength);
                } else {
                    LocalDisplay.drawText(tXCaptionPosition, tYCaptionPosition + getTextHeight(mCaptionSize), (tPosOfNewline + 1),
                            mCaptionSize, mCaptionColor, mButtonColor, tStringlength);
                }
#else
                LocalDisplay.drawText(tXCaptionPosition, tYCaptionPosition + getTextHeight(mCaptionSize), (tPosOfNewline + 1),
                        mCaptionSize, mCaptionColor, mButtonColor, tStringlength);
#endif
            }
        }
    }
}

bool TouchButton::isAutorepeatButton() {
    return (mFlags & FLAG_BUTTON_TYPE_AUTOREPEAT);
}

/**
 * Check if touch event is in button area
 * if yes - return true
 * if no - return false
 */
bool TouchButton::checkButtonInArea(uint16_t aTouchPositionX, uint16_t aTouchPositionY) {
    if (aTouchPositionX < mPositionX || aTouchPositionX > mPositionX + mWidthX || aTouchPositionY < mPositionY
            || aTouchPositionY > (mPositionY + mHeightY)) {
        return false;
    }
    return true;
}

void playLocalFeedbackTone() {
#if defined(ARDUINO)
#  if defined(LOCAL_GUI_FEEDBACK_TONE_PIN)
    tone(LOCAL_GUI_FEEDBACK_TONE_PIN, 3000, 50);
#  endif
#else
    tone(3000, 50);
#endif
}
/**
 * Check if touch event is in button area
 * if yes - call callback function and return true
 * if no - return false
 */
bool TouchButton::checkButton(uint16_t aTouchPositionX, uint16_t aTouchPositionY, bool aCheckOnlyAutorepeatButtons) {
    if ((mFlags & LOCAL_BUTTON_FLAG_IS_ACTIVE) && mOnTouchHandler != NULL
            && (!aCheckOnlyAutorepeatButtons || (mFlags & FLAG_BUTTON_TYPE_AUTOREPEAT))) {
        if (checkButtonInArea(aTouchPositionX, aTouchPositionY)) {
            /*
             *  Touch position is in button - call callback function
             */

            //Beep if requested
            if (mFlags & FLAG_BUTTON_DO_BEEP_ON_TOUCH) {
                playLocalFeedbackTone();
            }
            //Red/Green button handling
            if (mFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
                // Toggle value and handle color for Red/Green button
                mValue = !mValue;
                if (!(mFlags & FLAG_BUTTON_TYPE_MANUAL_REFRESH)) {
#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
                    this->mBDButtonPtr->setValueAndDraw(mValue); // Update also the remote button
#else
                    drawButton(); // Only local button refresh to show new color (and caption)
#endif
                }
            }

#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
            // Call with mBDButtonPtr as parameter, only the autorepeatTouchHandler must be called with the local button here
            if ((mFlags & LOCAL_BUTTON_FLAG_USE_BDBUTTON_FOR_CALLBACK)
                    && (&TouchButtonAutorepeat::autorepeatTouchHandler
                            != (void (*)(TouchButtonAutorepeat *, int16_t)) mOnTouchHandler)) {
                mOnTouchHandler((TouchButton *) this->mBDButtonPtr, mValue);
            } else {
                mOnTouchHandler(this, mValue); // Beep for autorepeat buttons is handled here
            }
#else
            mOnTouchHandler(this, mValue); // In case of autorepeat button, this calls the autorepeatTouchHandler()
#endif
            return true;
        }
    }
    return false;
}

/**
 * Static convenience method - checks all buttons for matching touch position.
 */
uint8_t TouchButton::checkAllButtons(unsigned int aTouchPositionX, unsigned int aTouchPositionY, bool aCheckOnlyAutorepeatButtons) {
    TouchButton *tButtonPointer = sButtonListStart;
    // walk through list
    while (tButtonPointer != NULL) {
        // Always check autorepeat buttons
        if ((!aCheckOnlyAutorepeatButtons || (tButtonPointer->mFlags & FLAG_BUTTON_TYPE_AUTOREPEAT))) {
            if (tButtonPointer->checkButton(aTouchPositionX, aTouchPositionY, aCheckOnlyAutorepeatButtons)) {
                return BUTTON_TOUCHED;
            }
        }
        tButtonPointer = tButtonPointer->mNextObject;
    }
    return NO_BUTTON_TOUCHED;
}

/**
 * Static convenience method - deactivate all buttons (e.g. before switching screen)
 */
void TouchButton::deactivateAllButtons() {
    TouchButton *tObjectPointer = sButtonListStart;
    // walk through list
    while (tObjectPointer != NULL) {
        tObjectPointer->deactivate();
        tObjectPointer = tObjectPointer->mNextObject;
    }
}

/**
 * Static convenience method - activate all buttons
 */
void TouchButton::activateAllButtons() {
    TouchButton *tObjectPointer = sButtonListStart;
    // walk through list
    while (tObjectPointer != NULL) {
        tObjectPointer->activate();
        tObjectPointer = tObjectPointer->mNextObject;
    }
}

#if defined(AVR)
/*
 * Not used yet
 */
uint8_t TouchButton::getCaptionLength(char *aCaptionPointer) {
    uint8_t tLength = 0;
    uint8_t tFontWidth = 8;
    if (mCaptionSize > 11) {
        tFontWidth = 16;
    }
    if (mFlags & LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE) {
        while (pgm_read_byte(aCaptionPointer++) != 0) {
            tLength += (tFontWidth);
        }
    } else {
        while (*aCaptionPointer++ != 0) {
            tLength += (tFontWidth);
        }
    }
    return tLength;
}

void TouchButton::setCaptionPGM(PGM_P aCaption) {
    mFlags |= LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE;
    mCaption = aCaption;
}

void TouchButton::setCaptionPGMForValueTrue(PGM_P aCaption) {
    mFlags |= LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE;
    mCaptionForTrue = aCaption;
}

#  if defined(DEBUG)
    /*
     * for debug purposes
     * needs char aStringBuffer[23+<CaptionLength>]
     */
    void TouchButton::toString(char * aStringBuffer) const {
        sprintf(aStringBuffer, "X=%03u Y=%03u X1=%03u Y1=%03u B=%02u %s", mPositionX, mPositionY, mPositionX + mWidthX - 1,
                mPositionY + mHeightY - 1, mTouchBorder, mCaption);
    }
#  endif
#endif

const char* TouchButton::getCaption() const {
    return mCaption;
}

uint16_t TouchButton::getValue() const {
    return mValue;
}

void TouchButton::setCaptionForValueTrue(const char *aCaption) {
#if defined(AVR)
    mFlags &= ~LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE;
#endif
    mCaptionForTrue = aCaption;
}

/*
 * Set caption
 */
void TouchButton::setCaption(const char *aCaption, bool doDrawButton) {
#if defined(AVR)
    mFlags &= ~LOCAL_BUTTON_FLAG_BUTTON_CAPTION_IS_IN_PGMSPACE;
#endif
    mCaption = aCaption;
    if (doDrawButton) {
        drawButton();
    }
}

/**
 * Set caption and draw
 */
void TouchButton::setCaptionAndDraw(const char *aCaption) {
    mCaption = aCaption;
    drawButton();
}

/*
 * changes box color and redraws button
 */
void TouchButton::setButtonColor(color16_t aButtonColor) {
    mButtonColor = aButtonColor;
}

void TouchButton::setButtonColorAndDraw(color16_t aButtonColor) {
    mButtonColor = aButtonColor;
    this->drawButton();
}

void TouchButton::setCaptionColor(color16_t aCaptionColor) {
    mCaptionColor = aCaptionColor;
}

/**
 * value 0 -> red, 1 -> green
 */
void TouchButton::setColorForRedGreenButton(bool aValue) {
    if (mFlags & FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN) {
        if (aValue) {
            mButtonColor = COLOR16_GREEN;
        } else {
            mButtonColor = COLOR16_RED;
        }
    }
}

void TouchButton::setValue(int16_t aValue) {
    setColorForRedGreenButton(aValue);
    mValue = aValue;
}

void TouchButton::setValueAndDraw(int16_t aValue) {
    setValue(aValue);
    drawButton();
}

uint16_t TouchButton::getPositionX() const {
    return mPositionX;
}
int8_t TouchButton::setPositionX(uint16_t aPositionX) {
    return setPosition(aPositionX, mPositionY);
}

uint16_t TouchButton::getPositionY() const {
    return mPositionY;
}
int8_t TouchButton::setPositionY(uint16_t aPositionY) {
    return setPosition(mPositionX, aPositionY);
}

uint16_t TouchButton::getPositionXRight() const {
    return mPositionX + mWidthX - 1;
}

uint16_t TouchButton::getPositionYBottom() const {
    return mPositionY + mHeightY - 1;
}

/*
 * activate for touch checking
 */
void TouchButton::activate() {
    mFlags |= LOCAL_BUTTON_FLAG_IS_ACTIVE;
}

/*
 * deactivate for touch checking
 */
void TouchButton::deactivate() {
    mFlags &= ~LOCAL_BUTTON_FLAG_IS_ACTIVE;
}

void TouchButton::setTouchHandler(void (*aOnTouchHandler)(TouchButton*, int16_t)) {
    mOnTouchHandler = aOnTouchHandler;
}

/** @} */
/** @} */
#endif // _LOCAL_TOUCH_BUTTON_HPP
