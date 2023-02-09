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

static struct XYPosition sLastPos;
static uint16_t sDrawColor = COLOR16_BLACK;

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
#if defined(MAIN_HOME_AVAILABLE)
    TouchButtonMainHome.drawButton();
#else
    TouchButtonBack.drawButton();
#endif
}

void doDrawClear(Button *aTheTouchedButton, int16_t aValue) {
    drawDrawPage();
}

static void doDrawColor(Button *aTheTouchedButton, int16_t aValue) {
    sDrawColor = DrawColors[aValue];
}

#if defined(SUPPORT_LOCAL_DISPLAY) && !defined(LOCAL_DISPLAY_GENERATES_BD_EVENTS)
void drawLine(const bool aNewStart, unsigned int color) {
    static unsigned int last_x = 0, last_y = 0;
    if (aNewStart) {
        LocalDisplay.drawPixel(TouchPanel.getCurrentX(), TouchPanel.getCurrentY(), color);
    } else {
        LocalDisplay.drawLine(last_x, last_y, TouchPanel.getCurrentX(), TouchPanel.getCurrentY(), color);
    }
    last_x = TouchPanel.getCurrentX();
    last_y = TouchPanel.getCurrentY();
}
#endif

/*
 * Position changed -> draw line
 */
void drawPageTouchMoveCallbackHandler(struct TouchEvent *const aCurrentPositionPtr) {
    Display.drawLine(sLastPos.PositionX, sLastPos.PositionY, aCurrentPositionPtr->TouchPosition.PositionX,
            aCurrentPositionPtr->TouchPosition.PositionY, sDrawColor);
    sLastPos.PositionX = aCurrentPositionPtr->TouchPosition.PositionX;
    sLastPos.PositionY = aCurrentPositionPtr->TouchPosition.PositionY;
}

/*
 * Touch is going down on canvas -> draw starting point
 */
void drawPageTouchDownCallbackHandler(struct TouchEvent *const aCurrentPositionPtr) {
    int x = aCurrentPositionPtr->TouchPosition.PositionX;
    int y = aCurrentPositionPtr->TouchPosition.PositionY;
    Display.drawPixel(x, y, sDrawColor);
    sLastPos.PositionX = x;
    sLastPos.PositionY = y;
}

void startDrawPage(void) {
    // Color buttons
    uint16_t tPosY = 0;
    for (uint8_t i = 0; i < 5; ++i) {
        TouchButtonsDrawColor[i].init(0, tPosY, 30, 30, DrawColors[i], "", TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, i,
                &doDrawColor);
        tPosY += 30;
    }

    TouchButtonClear.init(BUTTON_WIDTH_3_POS_3, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_RED, F("Clear"),
            TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doDrawClear);

#if !defined(DISABLE_REMOTE_DISPLAY)
    registerTouchDownCallback(&drawPageTouchDownCallbackHandler);
    registerTouchMoveCallback(&drawPageTouchMoveCallbackHandler);
    registerRedrawCallback(&drawDrawPage);
#endif

    drawDrawPage();
}

void loopDrawPage(void) {
#if defined(SUPPORT_LOCAL_DISPLAY) && !defined(LOCAL_DISPLAY_GENERATES_BD_EVENTS)
        if (TouchPanel.ADS7846TouchActive && sTouchObjectTouched == PANEL_TOUCHED) {
            drawLine(TouchPanel.wasJustTouched(), sDrawColor);
        }
        printLocalTouchPanelData();
//    checkAndHandleTouchPanelEvents(); // we know it is called at the loop which called us
#else
    checkAndHandleEvents();
#endif
}

void stopDrawPage(void) {
// free buttons
    for (unsigned int i = 0; i < NUMBER_OF_DRAW_COLORS; ++i) {
        TouchButtonsDrawColor[i].deinit();
    }
    TouchButtonClear.deinit();
}

#if defined(BUTTON_IS_DEFINED_LOCALLY)
#undef BUTTON_IS_DEFINED_LOCALLY
#undef Button
#undef AutorepeatButton
#undef Slider
#undef Display
#endif

#endif // _PAGE_DRAW_HPP
