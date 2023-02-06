/*
 * LocalDisplayGui.cpp
 *
 * GUI for local display settings like backlight and ADS7846 settings and info
 *
 *
 *  Copyright (C) 2023  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of STMF3-Discovery-Demos https://github.com/ArminJo/STMF3-Discovery-Demos.
 *
 *  STMF3-Discovery-Demos is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/gpl.html>.
 */

#ifndef _LOCAL_DISPLAY_GUI_HPP
#define _LOCAL_DISPLAY_GUI_HPP

#if defined(SUPPORT_LOCAL_DISPLAY)

/*
 * For programs, that must save memory when running on local display only
 */
#if !defined(Button)
#define BUTTON_IS_DEFINED_LOCALLY
#  if defined(SUPPORT_LOCAL_DISPLAY) && defined(DISABLE_REMOTE_DISPLAY)
// Only local display must be supported, so TouchButton, etc is sufficient
#define Button              LocalTouchButton
#define AutorepeatButton    LocalTouchButtonAutorepeat
#define Slider              LocalTouchSlider
#define Display             LocalDisplay
#  else
// Remote display must be served here, so use BD elements, they are aware of the existence of Local* objects and use them if SUPPORT_LOCAL_DISPLAY is enabled
#define Button              BDButton
#define AutorepeatButton    BDButton
#define Slider              BDSlider
#define Display             BlueDisplay1
#  endif
#endif

#if !defined(BACKLIGHT_CONTROL_X)
#define BACKLIGHT_CONTROL_X 30
#define BACKLIGHT_CONTROL_Y 4
#endif

AutorepeatButton TouchButtonAutorepeatBacklight_Plus;
AutorepeatButton TouchButtonAutorepeatBacklight_Minus;

/*******************
 * Backlight stuff
 *******************/
void doChangeBacklight(Button *aTheTouchedButton, int16_t aValue);
void doBacklightSlider(Slider *aTheTouchedSlider, uint16_t aBrightnessPercent);

/**
 * create backlight slider and autorepeat buttons
 */
void createBacklightGUI(void) {
    TouchButtonAutorepeatBacklight_Plus.init(BACKLIGHT_CONTROL_X, BACKLIGHT_CONTROL_Y, BUTTON_WIDTH_10,
    BUTTON_HEIGHT_6, COLOR16_RED, "+", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, 1,
            &doChangeBacklight);
    /*
     * Backlight slider
     */
    TouchSliderBacklight.init(BACKLIGHT_CONTROL_X, BACKLIGHT_CONTROL_Y + BUTTON_HEIGHT_6 + 4,
    SLIDER_DEFAULT_BAR_WIDTH, BACKLIGHT_MAX_BRIGHTNESS_VALUE, BACKLIGHT_MAX_BRIGHTNESS_VALUE, sCurrentBacklightPercent, COLOR16_BLUE,
    COLOR16_GREEN, FLAG_SLIDER_SHOW_BORDER | FLAG_SLIDER_SHOW_VALUE, &doBacklightSlider);
    TouchSliderBacklight.setCaptionProperties(TEXT_SIZE_11, FLAG_SLIDER_VALUE_CAPTION_ALIGN_MIDDLE, SLIDER_DEFAULT_VALUE_MARGIN, COLOR16_RED,
    BACKGROUND_COLOR);
    TouchSliderBacklight.setCaption("Backlight");
    TouchSliderBacklight.setPrintValueProperties(TEXT_SIZE_11, FLAG_SLIDER_VALUE_CAPTION_ALIGN_MIDDLE, SLIDER_DEFAULT_VALUE_MARGIN + TEXT_SIZE_11,
    COLOR16_BLUE, BACKGROUND_COLOR);

    TouchButtonAutorepeatBacklight_Minus.init(BACKLIGHT_CONTROL_X, 180, BUTTON_WIDTH_10, BUTTON_HEIGHT_6, COLOR16_RED, "-", TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT, -1, &doChangeBacklight);

    TouchButtonAutorepeatBacklight_Plus.setButtonAutorepeatTiming(600, 100, 10, 20);
    TouchButtonAutorepeatBacklight_Minus.setButtonAutorepeatTiming(600, 100, 10, 20);
}


void deinitBacklightElements(void) {
    TouchButtonAutorepeatBacklight_Plus.deinit();
    TouchButtonAutorepeatBacklight_Minus.deinit();
    TouchSliderBacklight.deinit();
}

void drawBacklightElements(void) {
    TouchButtonAutorepeatBacklight_Plus.drawButton();
    TouchSliderBacklight.drawSlider();
    TouchButtonAutorepeatBacklight_Minus.drawButton();
}

void doBacklightSlider(Slider *aTheTouchedSlider, uint16_t aBrightnessPercent) {
    LocalDisplay.setBacklightBrightness(aBrightnessPercent);
}

void doChangeBacklight(Button *aTheTouchedButton, int16_t aValue) {
    auto tCurrentBacklightPercent = sCurrentBacklightPercent + aValue; // See 8 bit roll-under here :-)
    if(tCurrentBacklightPercent == 101) { // we know that aValue can only be 1 or -1
        tCurrentBacklightPercent = 100;
        AutorepeatButton::disableAutorepeatUntilEndOfTouch();
    }
    LocalDisplay.setBacklightBrightness(tCurrentBacklightPercent);
    TouchSliderBacklight.setValueAndDrawBar(tCurrentBacklightPercent);
}
#endif

#if defined(BUTTON_IS_DEFINED_LOCALLY)
#undef BUTTON_IS_DEFINED_LOCALLY
#undef Button
#undef AutorepeatButton
#undef Slider
#undef Display
#endif

#endif // _LOCAL_DISPLAY_GUI_HPP
