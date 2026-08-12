/*
 * ManySlidersAndButtonsHelper.hpp
 *
 * Helper code for ManySlidersAndButtons example.
 *
 *  Copyright (C) 2025-2026  Armin Joachimsmeyer
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

//#define NUMBER_OF_SETUPS 4
//#define EEMEM // enable for Eclipse for pretty print

/********************
 *      SLIDERS
 ********************/
struct SliderStaticInfoStruct sTemporarySliderStaticInfo; // The PROGMEM values to be processed are copied to this instance at runtime
BDSlider sLeftSliderArray[NUMBER_OF_LEFT_SLIDERS];
BDSlider sRightSliderArray[NUMBER_OF_RIGHT_SLIDERS];

/**
 * Current values of the sliders and EEprom storage for them
 */
uint8_t sSetupIndex = 0; // 0 for button "1 load"
#if defined(EEMEM)
uint8_t sEEPROMSetupIndex; // always a copy of EEPROMSetupIndex. If "0xFF" assume setup values as not initialized
uint8_t EEPROMSetupIndex EEMEM;
#endif

/*
 * For 1 setup we have 2 big buttons "Load value" and "Store value"
 * otherwise we have 2 small buttons "1 Load" and "Store" for each setup
 */
#if NUMBER_OF_SETUPS > 1
BDButton FirstLoadButton; // The load button with text "1 Load"
#elif NUMBER_OF_SETUPS == 1
BDButton LoadButton;
BDButton StoreButton;
#endif

#if NUMBER_OF_LEFT_SLIDERS > 0
int16_t sLeftSliderValues[NUMBER_OF_LEFT_SLIDERS];
#  if defined(EEMEM)
#    if NUMBER_OF_SETUPS > 0
int16_t EEPROMLeftSliderValues[NUMBER_OF_SETUPS][NUMBER_OF_LEFT_SLIDERS] EEMEM;
#    endif
#  endif
#endif

#if NUMBER_OF_RIGHT_SLIDERS > 0
// Current values of the sliders and EEprom storage for them
int16_t sRightSliderValues[NUMBER_OF_RIGHT_SLIDERS];
#  if defined(EEMEM)
#    if NUMBER_OF_SETUPS > 0
int16_t EEPROMRightSliderValues[NUMBER_OF_RIGHT_SLIDERS][NUMBER_OF_LEFT_SLIDERS] EEMEM;
#    endif
#  endif
#endif

/********************
 *     BUTTONS
 ********************/
#if defined(EEMEM)
BDButton sLoadStoreButtonArray[NUMBER_OF_SETUPS * 2];
void doLoadButton(BDButton *aTheTouchedButton, int16_t aValue);
void doStoreButton(BDButton *aTheTouchedButton, int16_t aValue);
#endif
BDButton sButtonArray[NUMBER_OF_BUTTONS];

/********************
 *     FUNCTIONS
 ********************/
void InitSliderFromSliderStaticInfoStruct(BDSlider *aSlider, const SliderStaticInfoStruct *aSliderStaticPGMInfoPtr,
        int16_t *SliderValuesPtr, int aXPosition, int aYPosition) {
    const SliderStaticInfoStruct *tTemporarySliderStaticInfoPtr;

#if defined(ARDUINO) && defined(__AVR__)
// copy PGM data to RAM
    memcpy_P(&sTemporarySliderStaticInfo, aSliderStaticPGMInfoPtr, sizeof(sTemporarySliderStaticInfo));
    tTemporarySliderStaticInfoPtr = &sTemporarySliderStaticInfo;
#else
    tTemporarySliderStaticInfoPtr = aSliderStaticPGMInfoPtr;
#endif

// Initialize slider with default value SLIDER_BAR_LENGTH / 2
    aSlider->init(aXPosition, aYPosition, SLIDER_BAR_WIDTH, SLIDER_BAR_LENGTH, tTemporarySliderStaticInfoPtr->Threshold,
            SLIDER_BAR_LENGTH / 2, SLIDER_BAR_BG_COLOR, SLIDER_BAR_COLOR, FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_SHOW_VALUE,
            &doSlider);

    /*
     * Set additional values, like min, max, and value and caption size and position
     */
    aSlider->setMinMaxValue(tTemporarySliderStaticInfoPtr->MinValue, tTemporarySliderStaticInfoPtr->MaxValue);
    *SliderValuesPtr = (tTemporarySliderStaticInfoPtr->MinValue + tTemporarySliderStaticInfoPtr->MaxValue) / 2;

    aSlider->setCaptionProperties(SLIDER_CAPTION_SIZE, FLAG_SLIDER_CAPTION_ALIGN_LEFT_BELOW, SLIDER_CAPTION_MARGIN,
            SLIDER_CAPTION_COLOR, SLIDER_CAPTION_BG_COLOR);
    aSlider->setPrintValueProperties(SLIDER_CAPTION_SIZE, FLAG_SLIDER_VALUE_CAPTION_ALIGN_RIGHT, SLIDER_CAPTION_MARGIN,
            SLIDER_VALUE_COLOR, SLIDER_CAPTION_BG_COLOR);

    aSlider->setCaption(reinterpret_cast<const __FlashStringHelper*>(tTemporarySliderStaticInfoPtr->SliderName));
}

void drawSlidersAndButtons(void) {
#if NUMBER_OF_LEFT_SLIDERS > 0
    for (uint8_t i = 0; i < NUMBER_OF_LEFT_SLIDERS; i++) {
        sLeftSliderArray[i].drawSlider();
    }
#endif

#if NUMBER_OF_BUTTONS > 0
    for (uint8_t i = 0; i < NUMBER_OF_BUTTONS; i++) {
        sButtonArray[i].drawButton();
    }
#endif

#if defined(EEMEM)
#  if NUMBER_OF_SETUPS > 1
    BDButton tButton(FirstLoadButton);
    for (uint8_t i = 0; i < (NUMBER_OF_SETUPS * 2); ++i) {
        tButton.drawButton();
        tButton.mButtonIndex++; // Simply increment id to get the next button
    }
    FirstLoadButton.mButtonIndex -= (NUMBER_OF_SETUPS * 2); // restore mButtonIndex

    displayCurrentEEPROMSetupNumber();

#  elif NUMBER_OF_SETUPS == 1
    LoadButton.drawButton();
    StoreButton.drawButton();
#  endif
#endif

#if NUMBER_OF_RIGHT_SLIDERS > 0
    for (uint8_t i = 0; i < NUMBER_OF_RIGHT_SLIDERS; i++) {
        sRightSliderArray[i].drawSlider();
    }
#endif
}

/*
 * Loads slider values from EEPROM
 * Function used as callback handler for connect too
 * Position the buttons in the middle between the sliders
 * If right sliders are missing increase slider and button size
 */
void initSlidersAndButtons(const SliderStaticInfoStruct *aLeftSliderStaticPGMInfoPtr,
        const SliderStaticInfoStruct *aRightSliderStaticPGMInfoPtr, const ButtonStaticInfoStruct *aButtonStaticPGMInfoPtr) {
    BDSlider::setDefaultBarThresholdColor (COLOR16_RED);
    int tYPosition;

    /*
     * Initialize sliders with default value SLIDER_BAR_LENGTH / 2
     */
    tYPosition = SLIDER_START_Y;
#if NUMBER_OF_LEFT_SLIDERS > 0
    /*
     * LEFT sliders
     * Initialize the SliderValues array, which is required if no valid EEPROM setup is available (100 bytes)
     */
    for (uint8_t i = 0; i < NUMBER_OF_LEFT_SLIDERS; i++) {
        // Left sliders with index from 0 to NUMBER_OF_LEFT_SLIDERS - 1
        InitSliderFromSliderStaticInfoStruct(&sLeftSliderArray[i], &aLeftSliderStaticPGMInfoPtr[i], &sLeftSliderValues[i], BUTTON_DEFAULT_SPACING_HALF,
                tYPosition);
        // Prepare for next line
        tYPosition += SLIDER_AND_BUTTON_DELTA_Y;
    }
#endif

#if NUMBER_OF_RIGHT_SLIDERS > 0
    /*
     * RIGHT sliders
     * Initialize the SliderValues array, which is required if no valid EEPROM setup is available
     */
    tYPosition = SLIDER_START_Y;
    for (uint8_t i = 0; i < NUMBER_OF_RIGHT_SLIDERS; i++) {
        // Right sliders with index from NUMBER_OF_LEFT_SLIDERS to NUMBER_OF_LEFT_SLIDERS + NUMBER_OF_RIGHT_SLIDERS - 1
        InitSliderFromSliderStaticInfoStruct(&sRightSliderArray[i], &aRightSliderStaticPGMInfoPtr[i], &sRightSliderValues[i],
        DISPLAY_WIDTH - (BUTTON_DEFAULT_SPACING_HALF + SLIDER_BAR_LENGTH), tYPosition);
        // Prepare for next line
        tYPosition += SLIDER_AND_BUTTON_DELTA_Y;
    }
#endif

#if NUMBER_OF_BUTTONS > 0
    /*
     * First, set button common parameters
     */
#  if defined(__AVR__)
    BDButton::BDButtonPGMTextParameterStruct tBDButtonParameterStruct; // Saves 480 Bytes for 5 buttons
#  else
    BDButton::BDButtonParameterStruct tBDButtonParameterStruct;
#  endif
    BDButton::setInitParameters(&tBDButtonParameterStruct, BUTTONS_START_X, 0, BUTTON_WIDTH, BUTTON_HEIGHT, COLOR16_GREEN, nullptr,
            BUTTON_TEXT_SIZE, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doButton);

    tYPosition = BUTTON_START_Y;

    /*
     * MIDDLE or RIGHT buttons are first to enable mButtonIndex starting with 0
     * Position the buttons in the middle between the sliders
     */
    for (uint8_t i = 0; i < NUMBER_OF_BUTTONS; i++) {
        // Middle button
        tBDButtonParameterStruct.aPositionY = tYPosition;
#  if defined(__AVR__)
        ButtonStaticInfoStruct tButtonStaticPGMInfo;
        memcpy_P(&tButtonStaticPGMInfo, &aButtonStaticPGMInfoPtr[i], sizeof(ButtonStaticInfoStruct));
        tBDButtonParameterStruct.aValue = tButtonStaticPGMInfo.Value;
        tBDButtonParameterStruct.aPGMText = reinterpret_cast<const __FlashStringHelper*>(tButtonStaticPGMInfo.ButtonText);
        if (tButtonStaticPGMInfo.ButtonTextForValueTrue != nullptr) {
            tBDButtonParameterStruct.aFlags |= FLAG_BUTTON_TYPE_TOGGLE;
        }

        sButtonArray[i].init(&tBDButtonParameterStruct);
        if (tButtonStaticPGMInfo.ButtonTextForValueTrue != nullptr) {
            sButtonArray[i].setTextForValueTrue(
                    reinterpret_cast<const __FlashStringHelper*>(tButtonStaticPGMInfo.ButtonTextForValueTrue));
            tBDButtonParameterStruct.aFlags &= ~FLAG_BUTTON_TYPE_TOGGLE;
        }
#  else
        tBDButtonParameterStruct.aValue = aButtonStaticPGMInfoPtr[i].Value;
        tBDButtonParameterStruct.aText = aButtonStaticPGMInfoPtr[i].ButtonText;
        if (aButtonStaticPGMInfoPtr[i].ButtonTextForValueTrue != nullptr) {
            tBDButtonParameterStruct.aFlags |= FLAG_BUTTON_TYPE_TOGGLE;
        }

        sButtonArray[i].init(&tBDButtonParameterStruct);
        if (aButtonStaticPGMInfoPtr[i].ButtonTextForValueTrue != nullptr) {
            sButtonArray[i].setTextForValueTrue(aButtonStaticPGMInfoPtr[i].ButtonTextForValueTrue);
            tBDButtonParameterStruct.aFlags &= ~FLAG_BUTTON_TYPE_TOGGLE;
        }
#  endif
        // Prepare for next line
        tYPosition += SLIDER_AND_BUTTON_DELTA_Y;
    }
#endif // NUMBER_OF_BUTTONS > 0

#if defined(EEMEM) && NUMBER_OF_SETUPS > 0
    initLoadAndStoreButtons(); // This reads the value of sSetupIndex from EEPROM
    loadSliderValuesFromEEPROMAndDisplay(); // initialize slider values
#endif
}

#if defined(EEMEM)

#if NUMBER_OF_SETUPS > 0
void getEEPROMSetupIndex() {
    sEEPROMSetupIndex = eeprom_read_byte(&EEPROMSetupIndex);
    sSetupIndex = sEEPROMSetupIndex;
    if (sSetupIndex >= NUMBER_OF_SETUPS) {
        sSetupIndex = 0;
    }
}
#endif

#if NUMBER_OF_SETUPS > 1
void displayCurrentEEPROMSetupNumber() {
    if (sEEPROMSetupIndex < NUMBER_OF_SETUPS) {
        BlueDisplay1.drawChar(BUTTONS_START_X + LOAD_STORE_BUTTON_WIDTH, LOAD_STORE_BUTTON_START_Y, '1' + sEEPROMSetupIndex,
                BUTTON_TEXT_SIZE, COLOR16_BLACK, COLOR16_WHITE);
    }
}

char sStringLoad[] = "1 Load";
char sStringStore[] = "Store";
#endif

/*
 * Button "1 Load" has (index) value 0
 */
void initLoadAndStoreButtons() {
    getEEPROMSetupIndex();
#if NUMBER_OF_SETUPS > 1
    // 2 small buttons "1 Load" and "Store" for each setup
    uint16_t tYPosition = LOAD_STORE_BUTTON_START_Y;
    sStringLoad[0] = '1';
    for (uint8_t i = 0; i < NUMBER_OF_SETUPS; ++i) {
        FirstLoadButton.init(BUTTONS_START_X, tYPosition, LOAD_STORE_BUTTON_WIDTH, BUTTON_HEIGHT, LOAD_STORE_BUTTON_COLOR,
                sStringLoad, BUTTON_TEXT_SIZE, FLAG_BUTTON_DO_BEEP_ON_TOUCH, i, &doLoadButton);
        FirstLoadButton.init(BUTTONS_START_X + LOAD_STORE_BUTTON_WIDTH + BUTTON_DEFAULT_SPACING_QUARTER, tYPosition,
                LOAD_STORE_BUTTON_WIDTH, BUTTON_HEIGHT, LOAD_STORE_BUTTON_COLOR, sStringStore, BUTTON_TEXT_SIZE,
                FLAG_BUTTON_DO_BEEP_ON_TOUCH, i, &doStoreButton);
        tYPosition += SLIDER_AND_BUTTON_DELTA_Y;
        sStringLoad[0]++; // increment number in load button text
    }
    FirstLoadButton.mButtonIndex -=
            (NUMBER_OF_SETUPS * 2) - 1; // restore mButtonIndex
#elif NUMBER_OF_SETUPS == 1
        // 2 big buttons "Load value" and "Store value"
        LoadButton.init(BUTTONS_START_X, LOAD_STORE_BUTTON_START_Y, BUTTON_WIDTH, BUTTON_HEIGHT, LOAD_STORE_BUTTON_COLOR, reinterpret_cast<const __FlashStringHelper*>(sString_LoadValues), BUTTON_TEXT_SIZE,
                FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doLoadButton);
        StoreButton.init(BUTTONS_START_X, LOAD_STORE_BUTTON_START_Y + SLIDER_AND_BUTTON_DELTA_Y, BUTTON_WIDTH, BUTTON_HEIGHT, LOAD_STORE_BUTTON_COLOR,
                reinterpret_cast<const __FlashStringHelper*>(sString_StoreValues), BUTTON_TEXT_SIZE, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doStoreButton);
#endif
}

/*
 * Sets sSetupIndex and calls loadSliderValuesFromEEPROMAndDisplay()
 */
void doLoadButton(BDButton *aTheTouchedButton, int16_t aValue) {
    (void) aTheTouchedButton;
#if NUMBER_OF_SETUPS > 0
    sSetupIndex = aValue;
#else
    (void) aValue;
#endif
    loadSliderValuesFromEEPROMAndDisplay();
    SliderValueChangeCallback(); // Call user callback
}

void doStoreButton(BDButton *aTheTouchedButton, int16_t aValue) {
    (void) aTheTouchedButton;
#if NUMBER_OF_SETUPS > 0
    sSetupIndex = aValue;
#else
    (void) aValue;
#endif
    storeSliderValuesToEEPROM();
}

/*
 * Functions used by Reset and Store buttons
 */
void storeSliderValuesToEEPROM() {
#if NUMBER_OF_SETUPS > 0
    eeprom_update_byte(&EEPROMSetupIndex, sSetupIndex);
    sEEPROMSetupIndex = sSetupIndex;
#  if NUMBER_OF_SETUPS > 1
    displayCurrentEEPROMSetupNumber();
#  endif
#endif
#if NUMBER_OF_SETUPS > 0
# if NUMBER_OF_LEFT_SLIDERS > 0
    eeprom_update_block(sLeftSliderValues, EEPROMLeftSliderValues[sSetupIndex], sizeof(sLeftSliderValues));
#  endif
# if NUMBER_OF_RIGHT_SLIDERS > 0
    eeprom_update_block(sRightSliderValues, EEPROMRightSliderValues[sSetupIndex], sizeof(sRightSliderValues));
#  endif
#endif
}

/*
 * No range checking for slider values here!
 */
void loadSliderValuesFromEEPROMAndDisplay() {
    // Check for valid EEPROM index value
    if (sEEPROMSetupIndex < NUMBER_OF_SETUPS) {
#  if NUMBER_OF_LEFT_SLIDERS > 0
        eeprom_read_block(sLeftSliderValues, EEPROMLeftSliderValues[sSetupIndex], sizeof(sLeftSliderValues));
#  endif
#  if NUMBER_OF_RIGHT_SLIDERS > 0
        eeprom_read_block(sRightSliderValues, EEPROMRightSliderValues[sSetupIndex], sizeof(sRightSliderValues));
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
        SliderValueChangeCallback();
    }
}
#endif // EEMEM

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

#endif // _MANY_SLIDER_AND_BUTTONS_HELPER_HPP
