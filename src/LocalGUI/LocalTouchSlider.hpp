/*
 * LocalTouchSlider.hpp
 *
 * Slider with value as unsigned word value
 *
 *  Local display interface used:
 *      LocalDisplay.fillRectRel()
 *      LocalDisplay.drawText()
 *      LOCAL_DISPLAY_WIDTH
 *      LOCAL_DISPLAY_HEIGHT
 *
 * Size of one slider is 44 bytes on Arduino
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
 *
 */

#ifndef _LOCAL_TOUCH_SLIDER_HPP
#define _LOCAL_TOUCH_SLIDER_HPP

#include "LocalGUI/LocalTouchSlider.h"
#include "BDSlider.h"

#if defined(AVR)
#define failParamMessage(wrongParam,message) void()
#endif

/** @addtogroup Gui_Library
 * @{
 */
/** @addtogroup Slider
 * @{
 */

TouchSlider *TouchSlider::sSliderListStart = NULL; // Start of list of touch sliders, required for the *AllSliders functions
uint16_t TouchSlider::sDefaultSliderColor = SLIDER_DEFAULT_BORDER_COLOR;
uint16_t TouchSlider::sDefaultBarColor = SLIDER_DEFAULT_BAR_COLOR;
uint16_t TouchSlider::sDefaultBarThresholdColor = SLIDER_DEFAULT_BAR_THRESHOLD_COLOR;
uint16_t TouchSlider::sDefaultBarBackgroundColor = SLIDER_DEFAULT_BAR_BACKGROUND_COLOR;
uint16_t TouchSlider::sDefaultCaptionColor = SLIDER_DEFAULT_CAPTION_COLOR;
uint16_t TouchSlider::sDefaultValueColor = SLIDER_DEFAULT_VALUE_COLOR;
uint16_t TouchSlider::sDefaultValueCaptionBackgroundColor = SLIDER_DEFAULT_CAPTION_VALUE_BACK_COLOR;

uint8_t TouchSlider::sDefaultTouchBorder = SLIDER_DEFAULT_TOUCH_BORDER;

/*
 * Constructor - insert in list
 */
TouchSlider::TouchSlider() { // @suppress("Class members should be properly initialized")
    mNextObject = NULL;
    if (sSliderListStart == NULL) {
        // first slider
        sSliderListStart = this;
    } else {
        // put object in slider list
        TouchSlider *tSliderPointer = sSliderListStart;
        // search last list element
        while (tSliderPointer->mNextObject != NULL) {
            tSliderPointer = tSliderPointer->mNextObject;
        }
        //append actual slider as last element
        tSliderPointer->mNextObject = this;
    }

}

#if !defined(ARDUINO)
/*
 * Destructor  - remove from slider list
 */
TouchSlider::~TouchSlider() {
    TouchSlider *tSliderPointer = sSliderListStart;
    if (tSliderPointer == this) {
        // remove first element of list
        sSliderListStart = NULL;
    } else {
        // walk through list to find "this"
        while (tSliderPointer != NULL) {
            if (tSliderPointer->mNextObject == this) {
                tSliderPointer->mNextObject = this->mNextObject;
                break;
            }
            tSliderPointer = tSliderPointer->mNextObject;
        }
    }
}
#endif

#if !defined(DISABLE_REMOTE_DISPLAY)
/*
 * Required for creating a local slider for an existing aBDSlider at aBDSlider::init
 */
TouchSlider::TouchSlider(BDSlider *aBDSliderPtr) { // @suppress("Class members should be properly initialized")
    mBDSliderPtr = aBDSliderPtr;
    mNextObject = NULL;
    if (sSliderListStart == NULL) {
        // first slider
        sSliderListStart = this;
    } else {
        // put object in slider list
        TouchSlider *tSliderPointer = sSliderListStart;
        // search last list element
        while (tSliderPointer->mNextObject != NULL) {
            tSliderPointer = tSliderPointer->mNextObject;
        }
        //append actual slider as last element
        tSliderPointer->mNextObject = this;
    }

}
#endif
/**
 * Static initialization of slider default colors
 */
void TouchSlider::setDefaults(uintForPgmSpaceSaving aDefaultTouchBorder, uint16_t aDefaultSliderColor, uint16_t aDefaultBarColor,
        uint16_t aDefaultBarThresholdColor, uint16_t aDefaultBarBackgroundColor, uint16_t aDefaultCaptionColor,
        uint16_t aDefaultValueColor, uint16_t aDefaultValueCaptionBackgroundColor) {
    sDefaultSliderColor = aDefaultSliderColor;
    sDefaultBarColor = aDefaultBarColor;
    sDefaultBarThresholdColor = aDefaultBarThresholdColor;
    sDefaultBarBackgroundColor = aDefaultBarBackgroundColor;
    sDefaultCaptionColor = aDefaultCaptionColor;
    sDefaultValueColor = aDefaultValueColor;
    sDefaultValueCaptionBackgroundColor = aDefaultValueCaptionBackgroundColor;
    sDefaultTouchBorder = aDefaultTouchBorder;
}

#if defined(SUPPORT_REMOTE_AND_LOCAL_DISPLAY)
/*
 * Used by event handler to create a temporary BDSlider for setting the value and as calling parameter for the local callback function
 */
TouchSlider* TouchSlider::getLocalSliderFromBDSliderHandle(BDSliderHandle_t aSliderHandleToSearchFor) {
    TouchSlider *tSliderPointer = sSliderListStart;
// walk through list
    while (tSliderPointer != NULL) {
        if (tSliderPointer->mBDSliderPtr->mSliderHandle == aSliderHandleToSearchFor) {
            break;
        }
        tSliderPointer = tSliderPointer->mNextObject;
    }
    return tSliderPointer;
}

/*
 * is called once after reconnect, to build up a remote copy of all local sliders
 */
void TouchSlider::createAllLocalSlidersAtRemote() {
    if (USART_isBluetoothPaired()) {
        TouchSlider *tSliderPointer = sSliderListStart;
        sLocalSliderIndex = 0;
// walk through list
        while (tSliderPointer != NULL) {
            // cannot use BDSlider.init since this allocates a new TouchSlider
            sendUSARTArgs(FUNCTION_SLIDER_CREATE, 12, tSliderPointer->mBDSliderPtr->mSliderHandle, tSliderPointer->mPositionX,
                    tSliderPointer->mPositionY, tSliderPointer->mBarWidth, tSliderPointer->mBarLength,
                    tSliderPointer->mThresholdValue, tSliderPointer->mActualValue, tSliderPointer->mSliderColor,
                    tSliderPointer->mBarColor, tSliderPointer->mFlags, tSliderPointer->mOnChangeHandler,
                    (reinterpret_cast<uint32_t>(tSliderPointer->mOnChangeHandler) >> 16));
            sLocalSliderIndex++;
            tSliderPointer = tSliderPointer->mNextObject;
        }
    }
}
#endif

/**
 * @brief initialization with all parameters (except BarBackgroundColor)
 * @param aPositionX - Determines upper left corner
 * @param aPositionY - Determines upper left corner
 * Only next 2 values are physical values in pixel
 * @param aBarWidth - Width of bar (and border) in pixel - no scaling!
 * @param aBarLength - Size of slider bar in pixel = maximum slider value if no scaling applied!
 *                     Negative means slider bar is top down an is equivalent to positive with FLAG_SLIDER_IS_INVERSE.
 * @param aThresholdValue - Scaling applied! If selected or sent value is bigger, then color of bar changes from BarColor to BarBackgroundColor
 * @param aInitalValue - Scaling applied!
 * @param aSliderColor - Color of slider border. If no border specified, then bar background color.
 * @param aBarColor
 * @param aFlags - See #FLAG_SLIDER_SHOW_BORDER etc.
 * @param aOnChangeHandler - If NULL no update of bar is done on touch - equivalent to FLAG_SLIDER_IS_ONLY_OUTPUT
 */

void TouchSlider::init(uint16_t aPositionX, uint16_t aPositionY, uint8_t aBarWidth, uint16_t aBarLength, uint16_t aThresholdValue,
        int16_t aInitalValue, uint16_t aSliderColor, uint16_t aBarColor, uint8_t aFlags,
        void (*aOnChangeHandler)(TouchSlider*, uint16_t)) {

    mIsActive = false;
    mCaption = NULL;
    mXOffsetValue = 0;

    /*
     * Set defaults
     */
    mBarThresholdColor = sDefaultBarThresholdColor;
    mBarBackgroundColor = sDefaultBarBackgroundColor;
    mCaptionColor = sDefaultCaptionColor;
    mValueColor = sDefaultValueColor;
    mValueCaptionBackgroundColor = sDefaultValueCaptionBackgroundColor;
    mTouchBorder = sDefaultTouchBorder;

    /*
     * Copy parameter
     */
    mPositionX = aPositionX;
    mPositionY = aPositionY;
    mFlags = aFlags;
    mSliderColor = aSliderColor;
    mBarColor = aBarColor;
    mBarWidth = aBarWidth;
    mBarLength = aBarLength;
    mActualValue = aInitalValue;
    mThresholdValue = aThresholdValue;
    mOnChangeHandler = aOnChangeHandler;

    checkParameterValues();

    uint8_t tShortBordersAddedWidth = mBarWidth;
    uint8_t tLongBordersAddedWidth = 2 * mBarWidth;
    if (!(mFlags & FLAG_SLIDER_SHOW_BORDER)) {
        tShortBordersAddedWidth = 0;
        tLongBordersAddedWidth = 0;
    }

    /*
     * compute lower right corner and validate
     */
    if (mFlags & FLAG_SLIDER_IS_HORIZONTAL) {
        mPositionXRight = mPositionX + mBarLength + tShortBordersAddedWidth - 1;
        if (mPositionXRight >= LOCAL_DISPLAY_WIDTH) {
            // simple fallback
            mPositionX = 0;
            failParamMessage(mPositionXRight, "XRight wrong");
            mPositionXRight = LOCAL_DISPLAY_WIDTH - 1;
        }
        mPositionYBottom = mPositionY + (tLongBordersAddedWidth + mBarWidth) - 1;
        if (mPositionYBottom >= LOCAL_DISPLAY_HEIGHT) {
            // simple fallback
            mPositionY = 0;
            failParamMessage(mPositionYBottom, "YBottom wrong");
            mPositionYBottom = LOCAL_DISPLAY_HEIGHT - 1;
        }

    } else {
        mPositionXRight = mPositionX + (tLongBordersAddedWidth + mBarWidth) - 1;
        if (mPositionXRight >= LOCAL_DISPLAY_WIDTH) {
            // simple fallback
            mPositionX = 0;
            failParamMessage(mPositionXRight, "XRight wrong");
            mPositionXRight = LOCAL_DISPLAY_WIDTH - 1;
        }
        mPositionYBottom = mPositionY + mBarLength + tShortBordersAddedWidth - 1;
        if (mPositionYBottom >= LOCAL_DISPLAY_HEIGHT) {
            // simple fallback
            mPositionY = 0;
            failParamMessage(mPositionYBottom, "YBottom wrong");
            mPositionYBottom = LOCAL_DISPLAY_HEIGHT - 1;
        }
    }
}

// Dummy to be more compatible with BDButton
void TouchSlider::deinit() {
}

void TouchSlider::setDefaultSliderColor(uint16_t aDefaultSliderColor) {
    sDefaultSliderColor = aDefaultSliderColor;
}

void TouchSlider::setDefaultBarColor(uint16_t aDefaultBarColor) {
    sDefaultBarColor = aDefaultBarColor;
}

void TouchSlider::initSliderColors(uint16_t aSliderColor, uint16_t aBarColor, uint16_t aBarThresholdColor,
        uint16_t aBarBackgroundColor, uint16_t aCaptionColor, uint16_t aValueColor, uint16_t aValueCaptionBackgroundColor) {
    mSliderColor = aSliderColor;
    mBarColor = aBarColor;
    mBarThresholdColor = aBarThresholdColor;
    mBarBackgroundColor = aBarBackgroundColor;
    mCaptionColor = aCaptionColor;
    mValueColor = aValueColor;
    mValueCaptionBackgroundColor = aValueCaptionBackgroundColor;
}

void TouchSlider::setValueAndCaptionBackgroundColor(uint16_t aValueCaptionBackgroundColor) {
    mValueCaptionBackgroundColor = aValueCaptionBackgroundColor;
}

void TouchSlider::setValueColor(uint16_t aValueColor) {
    mValueColor = aValueColor;
}

/**
 * Static convenience method - activate all sliders
 * @see deactivateAllSliders()
 */
void TouchSlider::activateAllSliders() {
    TouchSlider *tObjectPointer = sSliderListStart;
    while (tObjectPointer != NULL) {
        tObjectPointer->activate();
        tObjectPointer = tObjectPointer->mNextObject;
    }
}

/**
 * Static convenience method - deactivate all sliders
 * @see activateAllSliders()
 */
void TouchSlider::deactivateAllSliders() {
    TouchSlider *tObjectPointer = sSliderListStart;
    while (tObjectPointer != NULL) {
        tObjectPointer->deactivate();
        tObjectPointer = tObjectPointer->mNextObject;
    }
}

void TouchSlider::drawSlider() {
    mIsActive = true;

    if ((mFlags & FLAG_SLIDER_SHOW_BORDER)) {
        drawBorder();
    }

    // Fill middle bar with initial value
    drawBar();
    printCaption();
    // Print value as string
    printValue();
}

void TouchSlider::drawBorder() {
    uintForPgmSpaceSaving mShortBorderWidth = mBarWidth / 2;
    if (mFlags & FLAG_SLIDER_IS_HORIZONTAL) {
        // Create value bar upper border
        LocalDisplay.fillRectRel(mPositionX, mPositionY, mBarLength + mBarWidth, mBarWidth, mSliderColor);
        // Create value bar lower border
        LocalDisplay.fillRectRel(mPositionX, mPositionY + (2 * mBarWidth), mBarLength + mBarWidth, mBarWidth, mSliderColor);

        // Create left border
        LocalDisplay.fillRectRel(mPositionX, mPositionY + mBarWidth, mShortBorderWidth, mBarWidth, mSliderColor);
        // Create right border
        LocalDisplay.fillRectRel(mPositionXRight - mShortBorderWidth + 1, mPositionY + mBarWidth, mShortBorderWidth, mBarWidth,
                mSliderColor);
    } else {
        // Create left border
        LocalDisplay.fillRectRel(mPositionX, mPositionY, mBarWidth, mBarLength + mBarWidth, mSliderColor);
        // Create right border
        LocalDisplay.fillRectRel(mPositionX + (2 * mBarWidth), mPositionY, mBarWidth, mBarLength + mBarWidth, mSliderColor);

        // Create value bar upper border
        LocalDisplay.fillRectRel(mPositionX + mBarWidth, mPositionY, mBarWidth, mShortBorderWidth, mSliderColor);
        // Create value bar lower border
        LocalDisplay.fillRectRel(mPositionX + mBarWidth, mPositionYBottom - mShortBorderWidth + 1, mBarWidth, mShortBorderWidth,
                mSliderColor);
    }
}

/*
 * (re)draws the middle bar according to actual value
 */
void TouchSlider::drawBar() {
    uint16_t tActualValue = mActualValue;
    if (tActualValue > mBarLength) {
        tActualValue = mBarLength;
    }

    uintForPgmSpaceSaving mShortBorderWidth = 0;
    uintForPgmSpaceSaving tLongBorderWidth = 0;

    if ((mFlags & FLAG_SLIDER_SHOW_BORDER)) {
        mShortBorderWidth = mBarWidth / 2;
        tLongBorderWidth = mBarWidth;
    }

// draw background bar
    if (tActualValue < mBarLength) {
        if (mFlags & FLAG_SLIDER_IS_HORIZONTAL) {
            LocalDisplay.fillRectRel(mPositionX + mShortBorderWidth + tActualValue, mPositionY + tLongBorderWidth,
                    mBarLength - tActualValue, mBarWidth, mBarBackgroundColor);
        } else {
            LocalDisplay.fillRectRel(mPositionX + tLongBorderWidth, mPositionY + mShortBorderWidth, mBarWidth,
                    mBarLength - tActualValue, mBarBackgroundColor);
        }
    }

// Draw value bar
    if (tActualValue > 0) {
        int tColor = mBarColor;
        if (tActualValue > mThresholdValue) {
            tColor = mBarThresholdColor;
        }
        if (mFlags & FLAG_SLIDER_IS_HORIZONTAL) {
            LocalDisplay.fillRectRel(mPositionX + mShortBorderWidth, mPositionY + tLongBorderWidth, tActualValue, mBarWidth,
                    tColor);
        } else {
            LocalDisplay.fillRectRel(mPositionX + tLongBorderWidth, mPositionYBottom - mShortBorderWidth - tActualValue + 1,
                    mBarWidth, tActualValue, tColor);
        }
    }
}

void TouchSlider::setCaption(const char *aCaption) {
    mCaption = aCaption;
}

void TouchSlider::setCaptionColors(uint16_t aCaptionColor, uint16_t aValueCaptionBackgroundColor) {
    mCaptionColor = aCaptionColor;
    mValueCaptionBackgroundColor = aValueCaptionBackgroundColor;
}

/**
 * Dummy stub to compatible with BDSliders
 */
void TouchSlider::setCaptionProperties(uint8_t aCaptionSize __attribute__((unused)),
        uint8_t aCaptionPosition __attribute__((unused)), uint8_t aCaptionMargin __attribute__((unused)), color16_t aCaptionColor,
        color16_t aCaptionBackgroundColor) {
    setCaptionColors(aCaptionColor, aCaptionBackgroundColor);
}

void TouchSlider::setValueStringColors(uint16_t aValueStringColor, uint16_t aValueStringCaptionBackgroundColor) {
    mValueColor = aValueStringColor;
    mValueCaptionBackgroundColor = aValueStringCaptionBackgroundColor;
}

/**
 * Dummy stub to compatible with BDSliders
 *
 * Default values are ((BlueDisplay1.mReferenceDisplaySize.YHeight / 20), (FLAG_SLIDER_CAPTION_ALIGN_MIDDLE | FLAG_SLIDER_CAPTION_BELOW),
 *                      (BlueDisplay1.mReferenceDisplaySize.YHeight / 40), COLOR16_BLACK, COLOR16_WHITE);
 */
void TouchSlider::setPrintValueProperties(uint8_t aPrintValueTextSize __attribute__((unused)),
        uint8_t aPrintValuePosition __attribute__((unused)), uint8_t aPrintValueMargin __attribute__((unused)),
        color16_t aPrintValueColor, color16_t aPrintValueBackgroundColor) {
    setValueStringColors(aPrintValueColor, aPrintValueBackgroundColor);
}

/**
 * Print caption in the middle below slider
 */
void TouchSlider::printCaption() {
    if (mCaption == NULL) {
        return;
    }
    uint16_t tCaptionLengthPixel = strlen(mCaption) * TEXT_SIZE_11_WIDTH;
    if (tCaptionLengthPixel == 0) {
        mCaption = NULL;
    }

    uintForPgmSpaceSaving tSliderWidthPixel;
    if (mFlags & FLAG_SLIDER_IS_HORIZONTAL) {
        tSliderWidthPixel = mBarLength;
        if ((mFlags & FLAG_SLIDER_SHOW_BORDER)) {
            tSliderWidthPixel += mBarWidth;
        }
    } else {
        tSliderWidthPixel = mBarWidth;
        if ((mFlags & FLAG_SLIDER_SHOW_BORDER)) {
            tSliderWidthPixel = 3 * tSliderWidthPixel;
        }
    }

    /**
     * try to position the string in the middle below slider
     */
    uint16_t tCaptionPositionX = mPositionX + (tSliderWidthPixel / 2) - (tCaptionLengthPixel / 2);
// unsigned arithmetic overflow handling
    if (tCaptionPositionX > mPositionXRight) {
        tCaptionPositionX = 0;
    }

    // space for caption?
    uint16_t tCaptionPositionY = mPositionYBottom + (mBarWidth / 2);
    if (tCaptionPositionY > LOCAL_DISPLAY_HEIGHT - TEXT_SIZE_11) {
        // fallback
        failParamMessage(tCaptionPositionY, "Caption Bottom wrong");
        tCaptionPositionY = LOCAL_DISPLAY_HEIGHT - TEXT_SIZE_11;
    }

    LocalDisplay.drawText(tCaptionPositionX, tCaptionPositionY, (char*) mCaption, 1, mCaptionColor, mValueCaptionBackgroundColor);
}

/**
 * Print value
 */
int TouchSlider::printValue() {
    if (!(mFlags & FLAG_SLIDER_SHOW_VALUE)) {
        return 0;
    }
    unsigned int tValuePositionY = mPositionYBottom + mBarWidth / 2 + getTextAscend(TEXT_SIZE_11);
    if (mCaption != NULL && !((mFlags & FLAG_SLIDER_IS_HORIZONTAL) && (mFlags & FLAG_SLIDER_SHOW_VALUE))) {
        // print below value
        tValuePositionY += TEXT_SIZE_11_HEIGHT;
    }

    if (tValuePositionY > LOCAL_DISPLAY_HEIGHT - TEXT_SIZE_11_DECEND) {
        // fallback
        failParamMessage(tValuePositionY, "Value Bottom wrong");
        tValuePositionY = LOCAL_DISPLAY_HEIGHT - TEXT_SIZE_11_DECEND;
    }

    // Convert to string
    char tValueAsString[4];
    sprintf(tValueAsString, "%03d", mActualValue);
    return LocalDisplay.drawText(mPositionX + mXOffsetValue, tValuePositionY - TEXT_SIZE_11_ASCEND, tValueAsString, 1, mValueColor,
            mValueCaptionBackgroundColor);
}

/**
 * Print value
 */
int TouchSlider::printValue(const char *aValueString) {

    unsigned int tValuePositionY = mPositionYBottom + mBarWidth / 2 + getTextAscend(TEXT_SIZE_11);
    if (mCaption != NULL && !((mFlags & FLAG_SLIDER_IS_HORIZONTAL) && (mFlags & FLAG_SLIDER_SHOW_VALUE))) {
        // print below value
        tValuePositionY += TEXT_SIZE_11_HEIGHT;
    }

    if (tValuePositionY > LOCAL_DISPLAY_HEIGHT - TEXT_SIZE_11_DECEND) {
        // fallback
        tValuePositionY = LOCAL_DISPLAY_HEIGHT - TEXT_SIZE_11_DECEND;
    }
    return LocalDisplay.drawText(mPositionX + mXOffsetValue, tValuePositionY - TEXT_SIZE_11_ASCEND, aValueString, 1, mValueColor,
            mValueCaptionBackgroundColor);
}

/**
 * Check if touch event is in slider
 * - if yes - set bar and value call callback function and return true
 * - if no - return false
 */

bool TouchSlider::checkSlider(uint16_t aTouchPositionX, uint16_t aTouchPositionY) {
    unsigned int tPositionBorderX = mPositionX - mTouchBorder;
    if (mTouchBorder > mPositionX) {
        tPositionBorderX = 0;
    }
    unsigned int tPositionBorderY = mPositionY - mTouchBorder;
    if (mTouchBorder > mPositionY) {
        tPositionBorderY = 0;
    }
    if (!mIsActive || aTouchPositionX < tPositionBorderX || aTouchPositionX > mPositionXRight + mTouchBorder
            || aTouchPositionY < tPositionBorderY || aTouchPositionY > mPositionYBottom + mTouchBorder) {
        return false;
    }

    uintForPgmSpaceSaving tShortBorderWidth = 0;
    if ((mFlags & FLAG_SLIDER_SHOW_BORDER)) {
        tShortBorderWidth = mBarWidth / 2;
    }
    /*
     *  Touch position is in slider (plus additional touch border) here
     */
    unsigned int tActualTouchValue;
    if (mFlags & FLAG_SLIDER_IS_HORIZONTAL) {
        if (aTouchPositionX < (mPositionX + tShortBorderWidth)) {
            tActualTouchValue = 0;
        } else if (aTouchPositionX > (mPositionXRight - tShortBorderWidth)) {
            tActualTouchValue = mBarLength;
        } else {
            tActualTouchValue = aTouchPositionX - mPositionX - tShortBorderWidth + 1;
        }
    } else {
// adjust value according to size of upper and lower border
        if (aTouchPositionY > (mPositionYBottom - tShortBorderWidth)) {
            tActualTouchValue = 0;
        } else if (aTouchPositionY < (mPositionY + tShortBorderWidth)) {
            tActualTouchValue = mBarLength;
        } else {
            tActualTouchValue = mPositionYBottom - tShortBorderWidth - aTouchPositionY + 1;
        }
    }

    if (tActualTouchValue != mActualTouchValue) {
        mActualTouchValue = tActualTouchValue;
        if (mOnChangeHandler != NULL) {
            // call change handler
#if !defined(DISABLE_REMOTE_DISPLAY)
            if (mFlags & LOCAL_SLIDER_FLAG_USE_BDSLIDER_FOR_CALLBACK) {
                mOnChangeHandler((TouchSlider*) this->mBDSliderPtr, tActualTouchValue);
                // Synchronize remote slider
                this->mBDSliderPtr->setValueAndDrawBar(tActualTouchValue);
            } else {
                mOnChangeHandler(this, tActualTouchValue);
            }
#else
            mOnChangeHandler(this, tActualTouchValue);
#endif

            if (!(mFlags & FLAG_SLIDER_VALUE_BY_CALLBACK)) {
                if (tActualTouchValue == mActualValue) {
                    // value returned is equal displayed value - do nothing
                    return true;
                }
                // display value changed - check, store and redraw
                if (tActualTouchValue > mBarLength) {
                    tActualTouchValue = mBarLength;
                }
                mActualValue = tActualTouchValue;
                drawBar();
                printValue();
            }
        }
    }
    return true;
}

/**
 * Static convenience method - checks all sliders in for event position.
 */

bool TouchSlider::checkAllSliders(unsigned int aTouchPositionX, unsigned int aTouchPositionY) {
    TouchSlider *tObjectPointer = sSliderListStart;

// walk through list of active elements
    while (tObjectPointer != NULL) {
        if (tObjectPointer->mIsActive && tObjectPointer->checkSlider(aTouchPositionX, aTouchPositionY)) {
            return true;
        }
        tObjectPointer = tObjectPointer->mNextObject;
    }
    return false;
}

int16_t TouchSlider::getCurrentValue() const {
    return mActualValue;
}

/*
 * also redraws value bar
 */
void TouchSlider::setValue(int16_t aCurrentValue) {
    mActualValue = aCurrentValue;
}

void TouchSlider::setValueAndDraw(int16_t aCurrentValue) {
    mActualValue = aCurrentValue;
    drawBar();
    printValue(); // this checks for flag TOUCHFLAG_SLIDER_SHOW_VALUE itself
}

void TouchSlider::setValueAndDrawBar(int16_t aCurrentValue) {
    mActualValue = aCurrentValue;
    drawBar();
    printValue(); // this checks for flag TOUCHFLAG_SLIDER_SHOW_VALUE itself
}

void TouchSlider::setXOffsetValue(int16_t aXOffsetValue) {
    mXOffsetValue = aXOffsetValue;
}

/**
 * @param aPositionX - Determines upper left corner
 * @param aPositionY - Determines upper left corner
 */
void TouchSlider::setPosition(int16_t aPositionX, int16_t aPositionY) {
    mPositionX = aPositionX;
    mPositionY = aPositionY;
}

uint16_t TouchSlider::getPositionXRight() const {
    return mPositionXRight;
}

uint16_t TouchSlider::getPositionYBottom() const {
    return mPositionYBottom;
}

void TouchSlider::activate() {
    mIsActive = true;
}
void TouchSlider::deactivate() {
    mIsActive = false;
}

int8_t TouchSlider::checkParameterValues() {
    /**
     * Check and copy parameter
     */
    int8_t tRetValue = 0;

    if (mBarWidth == 0) {
        mBarWidth = SLIDER_DEFAULT_BAR_WIDTH;
    } else if (mBarWidth > SLIDER_MAX_BAR_WIDTH) {
        mBarWidth = SLIDER_MAX_BAR_WIDTH;
    }
    if (mBarLength == 0) {
        mBarLength = SLIDER_DEFAULT_MAX_VALUE;
    }
    if (mActualValue > mBarLength) {
        tRetValue = SLIDER_ERROR;
        mActualValue = mBarLength;
    }

    return tRetValue;
}

void TouchSlider::setBarThresholdColor(uint16_t barThresholdColor) {
    mBarThresholdColor = barThresholdColor;
}

void TouchSlider::setSliderColor(uint16_t sliderColor) {
    mSliderColor = sliderColor;
}

void TouchSlider::setBarColor(uint16_t barColor) {
    mBarColor = barColor;
}

void TouchSlider::setBarBackgroundColor(uint16_t aBarBackgroundColor) {
    mBarBackgroundColor = aBarBackgroundColor;
}

/** @} */
/** @} */
#endif // _LOCAL_TOUCH_SLIDER_HPP
