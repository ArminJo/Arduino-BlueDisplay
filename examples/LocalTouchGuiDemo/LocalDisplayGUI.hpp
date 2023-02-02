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

#if defined(DISABLE_REMOTE_DISPLAY)
#define Button              TouchButton
#define AutorepeatButton    TouchButtonAutorepeat
#define Slider              TouchSlider
#else
#define Button              BDButton
#define AutorepeatButton    BDButton
#define Slider              BDSlider
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
    TouchSliderBacklight.setCaptionProperties(TEXT_SIZE_11, FLAG_SLIDER_CAPTION_ALIGN_MIDDLE, 4, COLOR16_RED,
    BACKGROUND_COLOR);
    TouchSliderBacklight.setCaption("Backlight");
    TouchSliderBacklight.setPrintValueProperties(TEXT_SIZE_11, FLAG_SLIDER_CAPTION_ALIGN_MIDDLE, 4 + TEXT_SIZE_11,
    COLOR16_BLUE, BACKGROUND_COLOR);

    TouchButtonAutorepeatBacklight_Minus.init(BACKLIGHT_CONTROL_X, TouchSliderBacklight.getPositionYBottom() + 30,
    BUTTON_WIDTH_10, BUTTON_HEIGHT_6, COLOR16_RED, "-", TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_AUTOREPEAT,
            -1, &doChangeBacklight);

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
    LocalDisplay.setBacklightBrightness(sCurrentBacklightPercent + aValue);
    TouchSliderBacklight.setValueAndDrawBar(sCurrentBacklightPercent);
}
#endif

#undef Button
#undef AutorepeatButton
#undef Slider

#endif // _LOCAL_DISPLAY_GUI_HPP
