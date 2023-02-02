/*
 * PageDraw.cpp
 *
 *  Copyright (C) 2013-2023  Armin Joachimsmeyer
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

#ifndef _PAGE_DRAW_HPP
#define _PAGE_DRAW_HPP

#if defined(SUPPORT_LOCAL_DISPLAY) && defined(DISABLE_REMOTE_DISPLAY)
#define Button              TouchButton
#define AutorepeatButton    TouchButtonAutorepeat
#define Slider              TouchSlider
#define Display             LocalDisplay
#else
#define Button              BDButton
#define AutorepeatButton    BDButton
#define Slider              BDSlider
#define Display             BlueDisplay1
#endif

static struct XYPosition sLastPos;
static uint16_t sDrawColor = COLOR16_BLACK;
static bool sButtonTouched;

Button TouchButtonClear;
#define NUMBER_OF_DRAW_COLORS 5
Button TouchButtonsDrawColor[NUMBER_OF_DRAW_COLORS];
static const uint16_t DrawColors[NUMBER_OF_DRAW_COLORS] =
        { COLOR16_BLACK, COLOR16_RED, COLOR16_GREEN, COLOR16_BLUE, COLOR16_YELLOW };

void initDrawPage(void) {
}

void drawDrawPage(void) {
    Display.clearDisplay(BACKGROUND_COLOR);
    for (uint8_t i = 0; i < 5; ++i) {
        TouchButtonsDrawColor[i].drawButton();
    }
    TouchButtonClear.drawButton();
#if defined(AVR)
    TouchButtonBack.drawButton();
#else
    TouchButtonMainHome.drawButton();
#endif
}

void doDrawClear(Button *aTheTouchedButton, int16_t aValue) {
    drawDrawPage();
}

static void doDrawColor(Button *aTheTouchedButton, int16_t aValue) {
    sDrawColor = DrawColors[aValue];
}

/*
 * Position changed -> draw line
 */
void drawPageTouchMoveCallbackHandler(struct TouchEvent *const aCurrentPositionPtr) {
    if (!sButtonTouched) {
        Display.drawLine(sLastPos.PosX, sLastPos.PosY, aCurrentPositionPtr->TouchPosition.PosX,
                aCurrentPositionPtr->TouchPosition.PosY, sDrawColor);
        sLastPos.PosX = aCurrentPositionPtr->TouchPosition.PosX;
        sLastPos.PosY = aCurrentPositionPtr->TouchPosition.PosY;
    }
}

/*
 * Touch is going down on canvas -> draw starting point
 */
void drawPageTouchDownCallbackHandler(struct TouchEvent *const aCurrentPositionPtr) {
    // first check buttons
    sButtonTouched = TouchButton::checkAllButtons(aCurrentPositionPtr->TouchPosition.PosX, aCurrentPositionPtr->TouchPosition.PosY);
    if (!sButtonTouched) {
        int x = aCurrentPositionPtr->TouchPosition.PosX;
        int y = aCurrentPositionPtr->TouchPosition.PosY;
        Display.drawPixel(x, y, sDrawColor);
        sLastPos.PosX = x;
        sLastPos.PosY = y;
    }
}

void startDrawPage(void) {
    // Color buttons
    uint16_t tPosY = 0;
    for (uint8_t i = 0; i < 5; ++i) {
        TouchButtonsDrawColor[i].init(0, tPosY, 30, 30, DrawColors[i], (const char*)NULL, TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, i,
                &doDrawColor);
        tPosY += 30;
    }

    TouchButtonClear.init(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_RED, "Clear",
            TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doDrawClear);

#if !defined(AVR)
    // No need to store old values since I know, that I return to main page
    registerTouchDownCallback(&drawPageTouchDownCallbackHandler);
    registerTouchMoveCallback(&drawPageTouchMoveCallbackHandler);
    registerRedrawCallback(&drawDrawPage);
#endif

    drawDrawPage();
    // to avoid first line because of moves after touch of the button starting this page
    sButtonTouched = true;
}

void loopDrawPage(void) {
    checkAndHandleEvents();
}

void stopDrawPage(void) {
// free buttons
    for (unsigned int i = 0; i < NUMBER_OF_DRAW_COLORS; ++i) {
        TouchButtonsDrawColor[i].deinit();
    }
    TouchButtonClear.deinit();
}

#undef Button
#undef AutorepeatButton
#undef Slider
#undef Display

#endif // _PAGE_DRAW_HPP
