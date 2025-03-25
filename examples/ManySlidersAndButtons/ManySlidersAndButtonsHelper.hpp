/*
 * ManySlidersAndButtonsHelper.hpp
 *
 * Helper code for ManySlidersAndButtons example.
 *
 *  Copyright (C) 2025  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay https://github.com/ArminJo/Arduino-BlueDisplay.
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

#ifndef _MANY_SLIDER_AND_BUTTONS_HELPER_HPP
#define _MANY_SLIDER_AND_BUTTONS_HELPER_HPP

/********************
 *      SLIDERS
 ********************/
struct SliderStaticInfoStruct sTemporarySliderStaticInfo; // The PROGMEM values to be processed are copied to this instance at runtime
BDSlider sLeftSliderArray[NUMBER_OF_LEFT_SLIDERS];
BDSlider sRightSliderArray[NUMBER_OF_RIGHT_SLIDERS];

#if NUMBER_OF_LEFT_SLIDERS > 0
/**
 * Current values of the sliders and EEpron storage for them
 */
int16_t sLeftSliderValues[NUMBER_OF_LEFT_SLIDERS];
#  if defined(EEMEM)
int16_t EEPROMLeftSliderValues[NUMBER_OF_LEFT_SLIDERS] EEMEM;
#  endif
#endif

#if NUMBER_OF_RIGHT_SLIDERS > 0
// Current values of the sliders and EEprom storage for them
int16_t sRightSliderValues[NUMBER_OF_RIGHT_SLIDERS];
#  if defined(EEMEM)
int16_t EEPROMRightSliderValues[NUMBER_OF_RIGHT_SLIDERS] EEMEM;
#  endif
#endif

/********************
 *     BUTTONS
 ********************/
BDButton sButtonArray[NUMBER_OF_BUTTONS];

/********************
 *     FUNCTIONS
 ********************/
void InitSliderFromSliderStaticInfoStruct(BDSlider *aSlider, const SliderStaticInfoStruct *aSliderStaticPGMInfoPtr, int aXPosition,
        int aYPosition) {
    const SliderStaticInfoStruct *tTemporarySliderStaticInfoPtr;

#if defined(ARDUINO) && defined(__AVR__)
// copy PGM data to RAM
    memcpy_P(&sTemporarySliderStaticInfo, aSliderStaticPGMInfoPtr, sizeof(sTemporarySliderStaticInfo));
    tTemporarySliderStaticInfoPtr = &sTemporarySliderStaticInfo;
#else
    tTemporarySliderStaticInfoPtr = aSliderStaticPGMInfoPtr;
#endif

// Initialize slider
    aSlider->init(aXPosition, aYPosition, SLIDER_BAR_WIDTH, SLIDER_BAR_LENGTH, tTemporarySliderStaticInfoPtr->Threshold,
            SLIDER_BAR_LENGTH / 2, SLIDER_BAR_BG_COLOR, SLIDER_BAR_COLOR, FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_SHOW_VALUE,
            &doSlider);

    /*
     * Set additional values, like min, max, and value and caption size and position
     */
    aSlider->setMinMaxValue(tTemporarySliderStaticInfoPtr->MinValue, tTemporarySliderStaticInfoPtr->MaxValue);

    aSlider->setCaptionProperties(SLIDER_CAPTION_SIZE, FLAG_SLIDER_CAPTION_ALIGN_LEFT_BELOW, SLIDER_CAPTION_MARGIN,
            SLIDER_CAPTION_COLOR, SLIDER_CAPTION_BG_COLOR);
    aSlider->setPrintValueProperties(SLIDER_CAPTION_SIZE, FLAG_SLIDER_VALUE_CAPTION_ALIGN_RIGHT, SLIDER_CAPTION_MARGIN,
            SLIDER_VALUE_COLOR, SLIDER_CAPTION_BG_COLOR);

    aSlider->setCaption(reinterpret_cast<const __FlashStringHelper*>(tTemporarySliderStaticInfoPtr->SliderName));
}

/*
 * Loads slider values from EEPROM
 * Function used as callback handler for connect too
 */
void initSlidersAndButtons(const SliderStaticInfoStruct *aLeftSliderStaticPGMInfoPtr,
        const SliderStaticInfoStruct *aRightSliderStaticPGMInfoPtr, const ButtonStaticInfoStruct *aButtonStaticPGMInfoPtr) {
    BDSlider::setDefaultBarThresholdColor (COLOR16_RED);

    /*
     * First, set button common parameters
     */
#if defined(__AVR__)
    BDButton::BDButtonPGMTextParameterStruct tBDButtonParameterStruct; // Saves 480 Bytes for all 5 buttons
#else
    BDButton::BDButtonParameterStruct tBDButtonParameterStruct;
#endif
    BDButton::setInitParameters(&tBDButtonParameterStruct, BUTTONS_START_X, 0, BUTTON_WIDTH, BUTTON_HEIGHT, COLOR16_GREEN, nullptr,
            BUTTON_TEXT_SIZE, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doButton);

// Upper start position of sliders and buttons

    /*
     * Initialize sliders and position the button in the middle between the sliders
     */
    int tYPosition = SLIDER_AND_BUTTON_START_Y;
#if NUMBER_OF_LEFT_SLIDERS > 0
    /*
     * LEFT sliders
     */
    for (uint8_t i = 0; i < NUMBER_OF_LEFT_SLIDERS; i++) {
        // Left sliders with index from 0 to NUMBER_OF_LEFT_SLIDERS - 1
        InitSliderFromSliderStaticInfoStruct(&sLeftSliderArray[i], &aLeftSliderStaticPGMInfoPtr[i], BUTTON_DEFAULT_SPACING_HALF,
                tYPosition);
        // Prepare for next line
        tYPosition += SLIDER_AND_BUTTON_DELTA_Y;
    }
#endif

#if NUMBER_OF_RIGHT_SLIDERS > 0
    /*
     * RIGHT sliders
     */
    tYPosition = SLIDER_AND_BUTTON_START_Y;
    for (uint8_t i = 0; i < NUMBER_OF_RIGHT_SLIDERS; i++) {
        // Right sliders with index from NUMBER_OF_LEFT_SLIDERS to NUMBER_OF_LEFT_SLIDERS + NUMBER_OF_RIGHT_SLIDERS - 1
        InitSliderFromSliderStaticInfoStruct(&sRightSliderArray[i], &aRightSliderStaticPGMInfoPtr[i],
        DISPLAY_WIDTH - (BUTTON_DEFAULT_SPACING_HALF + SLIDER_BAR_LENGTH), tYPosition);
        // Prepare for next line
        tYPosition += SLIDER_AND_BUTTON_DELTA_Y;
    }
#endif

    tYPosition = SLIDER_AND_BUTTON_START_Y;
    /*
     * MIDDLE buttons
     */
    for (uint8_t i = 0; i < NUMBER_OF_BUTTONS; i++) {
        // Middle button
        tBDButtonParameterStruct.aPositionY = tYPosition;
#if defined(__AVR__)
        ButtonStaticInfoStruct tButtonStaticPGMInfo;
        memcpy_P(&tButtonStaticPGMInfo, &aButtonStaticPGMInfoPtr[i], sizeof(ButtonStaticInfoStruct));
        tBDButtonParameterStruct.aValue =tButtonStaticPGMInfo.Value;
        tBDButtonParameterStruct.aPGMText = reinterpret_cast<const __FlashStringHelper*>(tButtonStaticPGMInfo.ButtonText);
        if (tButtonStaticPGMInfo.ButtonTextForValueTrue != nullptr) {
            tBDButtonParameterStruct.aFlags |= FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN;
        }

        sButtonArray[i].init(&tBDButtonParameterStruct);
        if (tButtonStaticPGMInfo.ButtonTextForValueTrue != nullptr) {
            sButtonArray[i].setTextForValueTrue(reinterpret_cast<const __FlashStringHelper*>(tButtonStaticPGMInfo.ButtonTextForValueTrue));
            tBDButtonParameterStruct.aFlags &= ~FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN;
        }
#else
        tBDButtonParameterStruct.aValue = aButtonStaticPGMInfoPtr[i].Value;
        tBDButtonParameterStruct.aText = aButtonStaticPGMInfoPtr[i].ButtonText;
        if (aButtonStaticPGMInfoPtr[i].ButtonTextForValueTrue != nullptr) {
            tBDButtonParameterStruct.aFlags |= FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN;
        }

        sButtonArray[i].init(&tBDButtonParameterStruct);
        if (aButtonStaticPGMInfoPtr[i].ButtonTextForValueTrue != nullptr) {
            sButtonArray[i].setTextForValueTrue(aButtonStaticPGMInfoPtr[i].ButtonTextForValueTrue);
            tBDButtonParameterStruct.aFlags &= ~FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN;
        }
#endif
        // Prepare for next line
        tYPosition += SLIDER_AND_BUTTON_DELTA_Y;
    }

    /*
     * Set all slider values to initial position
     */
    loadSliderValuesFromEEPROM();
}

void drawSlidersAndButtons(void) {
#if NUMBER_OF_LEFT_SLIDERS > 0
    for (uint8_t i = 0; i < NUMBER_OF_LEFT_SLIDERS; i++) {
        sLeftSliderArray[i].drawSlider();
    }
#endif
    for (uint8_t i = 0; i < NUMBER_OF_BUTTONS; i++) {
        sButtonArray[i].drawButton();
    }
#if NUMBER_OF_RIGHT_SLIDERS > 0
    for (uint8_t i = 0; i < NUMBER_OF_RIGHT_SLIDERS; i++) {
        sRightSliderArray[i].drawSlider();
    }
#endif
}

uint8_t copyPGMStringStoredInPGMVariable(char *aStringBuffer, void *aPGMStringPtrStoredInPGMVariable) {
    const char *tSliderNamePtr = (const char*) pgm_read_ptr(aPGMStringPtrStoredInPGMVariable);
    PGM_P tPGMString = reinterpret_cast<PGM_P>(tSliderNamePtr);
    /*
     * compute string length
     */
    uint8_t tLength = strlen_P(tPGMString);
    /*
     * copy string up to length
     */
    strncpy_P(aStringBuffer, tPGMString, tLength + 1);
    return tLength;
}

/*
 * Functions used by Reset and Store buttons
 * Dummy for CPU without EEprom
 */
void storeSliderValuesToEEPROM() {
#if defined(EEMEM)
# if NUMBER_OF_LEFT_SLIDERS > 0
    eeprom_update_block(sLeftSliderValues, EEPROMLeftSliderValues, sizeof(sLeftSliderValues));
#  endif
# if NUMBER_OF_RIGHT_SLIDERS > 0
    eeprom_update_block(sRightSliderValues, EEPROMRightSliderValues, sizeof(sRightSliderValues));
#  endif
#endif
}

/*
 * No range checking here!
 */
void loadSliderValuesFromEEPROM() {
#if defined(EEMEM)
# if NUMBER_OF_LEFT_SLIDERS > 0
    eeprom_read_block(sLeftSliderValues, EEPROMLeftSliderValues, sizeof(sLeftSliderValues));
#  endif
# if NUMBER_OF_RIGHT_SLIDERS > 0
    eeprom_read_block(sRightSliderValues, EEPROMRightSliderValues, sizeof(sRightSliderValues));
#  endif

#  if NUMBER_OF_LEFT_SLIDERS > 0
    for (uint8_t i = 0; i < NUMBER_OF_LEFT_SLIDERS; i++) {
        sLeftSliderArray[i].setValueAndDrawBar(sLeftSliderValues[i]);
    }
#  endif
#  if NUMBER_OF_RIGHT_SLIDERS > 0
    for (uint8_t i = 0; i < NUMBER_OF_RIGHT_SLIDERS; i++) {
        sRightSliderArray[i].setValueAndDrawBar(sRightSliderValues[i]);
    }
#  endif
#endif
}

#endif // _MANY_SLIDER_AND_BUTTONS_HELPER_HPP
