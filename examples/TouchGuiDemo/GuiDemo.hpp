/**
 * GuiDemo.cpp
 *
 * Demo of the GUI: TouchButton, TouchSlider and Chart
 * and the programs Game of life and show ADS7846 A/D channels
 *
 *  Copyright (C) 2012-2023  Armin Joachimsmeyer
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

#ifndef _GUI_DEMO_HPP
#define _GUI_DEMO_HPP

#include "GuiDemo.h"
#include "GameOfLife.hpp"

// Must defined after including GameOfLife.hhp, since this #undefine Button etc.
#if !defined(Button)
#define BUTTON_IS_DEFINED_LOCALLY
#  if defined(SUPPORT_ONLY_LOCAL_DISPLAY)
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

/** @addtogroup Gui_Demo
 * @{
 */

/*
 * LCD and touch panel stuff
 */

#define COLOR_DEMO_BACKGROUND COLOR16_WHITE

void createDemoButtonsAndSliders(void);

/*
 * Global button handler for Chart, GameOfLife, Settings, Calibration and Back button
 */
void doGuiDemoButtons(Button *aTheTouchedButton, int16_t aValue);

void LongTouchDownHandlerGUIDemo(struct TouchEvent *const aTouchPosition);

/**
 * GUIDemo menu stuff
 */
void showGuiDemoMenu(void);

Button TouchButtonChartDemo;
void showCharts(void);

Button TouchButtonGameOfLife;

Button TouchButtonDemoSettings;
void showSettings(void);

#if defined(SUPPORT_LOCAL_DISPLAY)
Button TouchButtonFont;
void showFont(void);

Button TouchButtonCalibration;

//ADS7846 channels stuff
Button TouchButtonADS7846Channels;
void doADS7846Channels(Button *aTheTouchedButton, int16_t aValue);
void ADS7846DisplayChannels(void);
#endif

/*
 * Game of life GUI stuff
 */
void createGameOfLifeGUI();
void showGameOfLifeSettings(void);

static Slider TouchSliderGameOfLifeSpeed; // is also used in Settings sub page
void doGameOfLifeSpeed(Slider *aTheTouchedSlider, uint16_t aSliderValue);
// to slow down game of life
unsigned long GameOfLifeDelay = 0;

Button TouchButtonGameOfLifeDying;
void doGameOfLifeDying(Button *aTheTouchedButton, int16_t aValue);

Button TouchButtonNewGame;
Button TouchButtonStartStopGame;

void doNewGameOfLife(Button *aTheTouchedButton, int16_t aValue);
void doStartStopGameOfLife(Button *aTheTouchedButton, int16_t aValue);
void initNewGameOfLife(void);

bool GameOfLifeShowDying = true;
bool GameOfLifeRunning = false; // to stop or run the game
bool showingGameOfLife = false; // true if the game grid is shown
bool GameOfLifeInitialized = false;

/*
 * Settings GUI stuff
 */
Slider TouchSliderDemo1;
Slider TouchSliderAction;
Slider TouchSliderActionWithoutBorder;
unsigned int ActionSliderValue = 0;
#define ACTION_SLIDER_MAX 100
bool ActionSliderUp = true;

/*
 * For PageDraw
 */
Button TouchButtonDrawDemo;
void drawLine(const bool aNewStart, unsigned int color);

#define APPLICATION_MENU            0
#define APPLICATION_SETTINGS        1
#define APPLICATION_DRAW            2
#define APPLICATION_GAME_OF_LIFE    3
#define APPLICATION_CHART           4
#define APPLICATION_ADS7846_CHANNELS 5

int mCurrentApplication = APPLICATION_MENU;
unsigned int sMillisSinceLastDemoOutput;

void initGuiDemo(void) {
}

Button *TouchButtonsGuiDemo[] = { &TouchButtonChartDemo, &TouchButtonGameOfLife, &TouchButtonDrawDemo, &TouchButtonDemoSettings,
        &TouchButtonBack, &TouchButtonGameOfLifeDying, &TouchButtonNewGame, &TouchButtonStartStopGame
#  if defined(SUPPORT_LOCAL_DISPLAY)
        ,&TouchButtonFont, &TouchButtonCalibration, &TouchButtonADS7846Channels
#  endif
        };

void startGuiDemo(void) {
#if defined(SUPPORT_LOCAL_DISPLAY)
    createBacklightGUI();
#endif
#if !defined(DISABLE_REMOTE_DISPLAY)
    registerRedrawCallback(&showGuiDemoMenu);
#endif

    createDemoButtonsAndSliders();
    showGuiDemoMenu();
    tGameOfLifeByteArray = new uint8_t[GAME_OF_LIFE_X_SIZE][GAME_OF_LIFE_Y_SIZE]; // One cell requires one byte
    sMillisOfLastLoop = millis();
    registerLongTouchDownCallback(&LongTouchDownHandlerGUIDemo, TOUCH_STANDARD_LONG_TOUCH_TIMEOUT_MILLIS);
}

void loopGuiDemo(void) {
    // count milliseconds for loop control
    uint32_t tMillis = millis();
    sMillisSinceLastDemoOutput += tMillis - sMillisOfLastLoop;
    sMillisOfLastLoop = tMillis;

    /*
     * switch for different "apps"
     */
    switch (mCurrentApplication) {
    case APPLICATION_SETTINGS:
        /*
         * Animate self moving slider bar :-)
         */
        if (sMillisSinceLastDemoOutput >= 20) {
            sMillisSinceLastDemoOutput = 0;
            if (ActionSliderUp) {
                ActionSliderValue++;
                if (ActionSliderValue == ACTION_SLIDER_MAX) {
                    ActionSliderUp = false;
                }
            } else {
                ActionSliderValue--;
                if (ActionSliderValue == 0) {
                    ActionSliderUp = true;
                }
            }
            TouchSliderAction.setValueAndDrawBar(ActionSliderValue);
            TouchSliderActionWithoutBorder.setValueAndDrawBar(ACTION_SLIDER_MAX - ActionSliderValue);
        }
        break;

    case APPLICATION_GAME_OF_LIFE:
        if (GameOfLifeRunning && sMillisSinceLastDemoOutput >= GameOfLifeDelay) {
            //game of live "app"
            playGameOfLife();
            /*
             * Check if no change for 5 seconds or to much generations,
             * then start a new game
             */
            if (millis() - sLastFrameChangeMillis > 5000 || ++sCurrentGameOfLifeGeneration > GAME_OF_LIFE_MAX_GEN) {
                initGameOfLife();
            }
            drawGenerationText();
            drawGameOfLife();
            sMillisSinceLastDemoOutput = 0;
        }
        break;

#if defined(SUPPORT_LOCAL_DISPLAY)
    case APPLICATION_ADS7846_CHANNELS:
        ADS7846DisplayChannels();
        break;
#endif

    case APPLICATION_DRAW:
        loopDrawPage();
        break;

    case APPLICATION_MENU:
        break;
    }

#if defined(SUPPORT_LOCAL_DISPLAY) && !defined(LOCAL_DISPLAY_GENERATES_BD_EVENTS)
    checkAndHandleTouchPanelEvents();
#else
    checkAndHandleEvents();
#endif
}

void stopGuiDemo(void) {
    registerLongTouchDownCallback(NULL, 0);

    delete[] tGameOfLifeByteArray;
    // free buttons
    for (unsigned int i = 0; i < sizeof(TouchButtonsGuiDemo) / sizeof(TouchButtonsGuiDemo[0]); ++i) {
        TouchButtonsGuiDemo[i]->deinit();
    }
    TouchSliderGameOfLifeSpeed.deinit();
    TouchSliderActionWithoutBorder.deinit();
    TouchSliderAction.deinit();

#if defined(SUPPORT_LOCAL_DISPLAY)
    deinitBacklightElements();
#endif
}

/*
 * buttons are positioned relative to sliders and vice versa
 */
void createDemoButtonsAndSliders(void) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
    /*
     * Create and position all menu buttons initially
     */
    //1. row
    TouchButtonChartDemo.init(0, 0, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR16_RED, F("Chart"), TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);

    // 2. row
    TouchButtonGameOfLife.init(0, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR16_RED, F("Game\nof Life"),
            TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);

    TouchButtonDrawDemo.init(BUTTON_WIDTH_2_POS_2, BUTTON_HEIGHT_4_LINE_2, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR16_RED, F("Draw"),
            TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);

    // 3. row
    TouchButtonDemoSettings.init(0, BUTTON_HEIGHT_4_LINE_3, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR16_RED, F("Settings\nDemo"),
            TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);

#if defined(SUPPORT_LOCAL_DISPLAY)
    TouchButtonFont.init(BUTTON_WIDTH_2_POS_2, BUTTON_HEIGHT_4_LINE_3, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR16_RED, F("Font"),
            TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);

    // 4. row
    TouchButtonADS7846Channels.init(0, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR16_YELLOW, F("ADS7846"),
    TEXT_SIZE_22, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doADS7846Channels);

    TouchButtonCalibration.init(BUTTON_WIDTH_2_POS_2, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR16_YELLOW,
            F("TP-Calibration"),            TEXT_SIZE_11, FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);
#endif

    /*
     * Common back text button for sub pages
     */
    TouchButtonBack.init(BUTTON_WIDTH_3_POS_3, 0, BUTTON_WIDTH_3, BUTTON_HEIGHT_4, COLOR16_RED, F("Back"), TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doGuiDemoButtons);

    createGameOfLifeGUI();

    /*
     * Slider
     */
// self moving sliders COLOR16_WHITE is bar background color for slider without border
    TouchSliderActionWithoutBorder.init(180, BUTTON_HEIGHT_4_LINE_2 - 10, 20, ACTION_SLIDER_MAX, ACTION_SLIDER_MAX, 0,
            COLOR16_WHITE, COLOR16_YELLOW, FLAG_SLIDER_SHOW_VALUE | FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    TouchSliderActionWithoutBorder.setPrintValueProperties(TEXT_SIZE_11, FLAG_SLIDER_VALUE_CAPTION_ALIGN_MIDDLE,
            SLIDER_DEFAULT_VALUE_MARGIN, COLOR16_BLUE, COLOR_DEMO_BACKGROUND);

    TouchSliderAction.init(180 + 2 * 20 + BUTTON_DEFAULT_SPACING, BUTTON_HEIGHT_4_LINE_2 - 10, 20, ACTION_SLIDER_MAX,
    ACTION_SLIDER_MAX, 0, COLOR16_BLUE, COLOR16_YELLOW,
            FLAG_SLIDER_SHOW_BORDER | FLAG_SLIDER_SHOW_VALUE | FLAG_SLIDER_IS_ONLY_OUTPUT, NULL);
    TouchSliderAction.setPrintValueProperties(TEXT_SIZE_11, FLAG_SLIDER_VALUE_CAPTION_ALIGN_MIDDLE, SLIDER_DEFAULT_VALUE_MARGIN,
            COLOR16_BLUE, COLOR_DEMO_BACKGROUND);

#pragma GCC diagnostic pop
}

void LongTouchDownHandlerGUIDemo(struct TouchEvent *const aTouchPosition) {
#if defined(SUPPORT_ONLY_LOCAL_DISPLAY)
    Display.drawText(0, 0, F("Long touch down detected"), TEXT_SIZE_11, COLOR16_RED, COLOR_DEMO_BACKGROUND);
#else
    Display.drawText(0, TEXT_SIZE_11_ASCEND, F("Long touch down detected"), TEXT_SIZE_11, COLOR16_RED, COLOR_DEMO_BACKGROUND);
#endif
    Display.drawCircle(aTouchPosition->TouchPosition.PositionX, aTouchPosition->TouchPosition.PositionY, 4, COLOR16_RED);
}

void doGuiDemoButtons(Button *aTheTouchedButton, int16_t aValue) {
    Button::deactivateAll();
    Slider::deactivateAll();

    if (*aTheTouchedButton == TouchButtonBack) {
        // Back button pressed
        if (showingGameOfLife) {
            showGameOfLifeSettings();
        } else {
            if (mCurrentApplication == APPLICATION_DRAW) {
                stopDrawPage();
            }
            showGuiDemoMenu();
            // Enable long touch down callback again for main menu page
            registerLongTouchDownCallback(&LongTouchDownHandlerGUIDemo, TOUCH_STANDARD_LONG_TOUCH_TIMEOUT_MILLIS);
        }
    } else {
        registerLongTouchDownCallback(NULL, 0); // disable long touch down callback

#if defined(SUPPORT_LOCAL_DISPLAY)
    if (*aTheTouchedButton == TouchButtonCalibration) {
        //Calibration Button pressed -> calibrate touch panel
#  if defined(AVR)
        TouchPanel.doCalibration(TP_EEPROMADDR, false);
#  else
        TouchPanel.doCalibration(false);
#  endif
    } else if (*aTheTouchedButton == TouchButtonFont) {
        showFont();
    } else
#endif

        // BD button has an operator == defined
        if (*aTheTouchedButton == TouchButtonChartDemo) {
            // Chart button pressed
            showCharts();
        } else if (*aTheTouchedButton == TouchButtonGameOfLife) {
            // Game of Life button pressed
            showGameOfLifeSettings();
            mCurrentApplication = APPLICATION_GAME_OF_LIFE;

        } else if (*aTheTouchedButton == TouchButtonDemoSettings) {
            // Settings button pressed
            showSettings();

        } else if (*aTheTouchedButton == TouchButtonDrawDemo) {
            startDrawPage();
            mCurrentApplication = APPLICATION_DRAW;
        }
    }
}

/*
 * Game of life GUI functions
 */
void createGameOfLifeGUI() {
    TouchButtonNewGame.init(0, BUTTON_HEIGHT_4_LINE_3, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR16_RED, F("New\nGame"), TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH, 0, &doNewGameOfLife);

    TouchButtonStartStopGame.init(BUTTON_WIDTH_2_POS_2, BUTTON_HEIGHT_4_LINE_3, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, COLOR16_RED,
            F("Start\nStop"), TEXT_SIZE_22, FLAG_BUTTON_NO_BEEP_ON_TOUCH, GameOfLifeRunning, &doStartStopGameOfLife);

    TouchSliderGameOfLifeSpeed.init(70, BUTTON_HEIGHT_4 + BUTTON_DEFAULT_SPACING_HALF, 10, 75, 75, 75, COLOR16_BLUE, COLOR16_GREEN,
            FLAG_SLIDER_SHOW_BORDER | FLAG_SLIDER_IS_HORIZONTAL | FLAG_SLIDER_VALUE_CAPTION_BELOW | FLAG_SLIDER_VALUE_BY_CALLBACK,
            &doGameOfLifeSpeed);
    TouchSliderGameOfLifeSpeed.setCaptionProperties(TEXT_SIZE_11, FLAG_SLIDER_VALUE_CAPTION_ALIGN_MIDDLE, 2, COLOR16_RED,
            COLOR_DEMO_BACKGROUND);
    TouchSliderGameOfLifeSpeed.setCaption("Gol-Speed");
    TouchSliderGameOfLifeSpeed.setPrintValueProperties(TEXT_SIZE_11, FLAG_SLIDER_VALUE_CAPTION_ALIGN_MIDDLE, 4 + TEXT_SIZE_11,
            COLOR16_BLUE, COLOR_DEMO_BACKGROUND);

    TouchButtonGameOfLifeDying.init(0, BUTTON_HEIGHT_4_LINE_4, BUTTON_WIDTH_2, BUTTON_HEIGHT_4, 0, F("Show\ndying"), TEXT_SIZE_22,
            FLAG_BUTTON_DO_BEEP_ON_TOUCH | FLAG_BUTTON_TYPE_TOGGLE_RED_GREEN, GameOfLifeShowDying, &doGameOfLifeDying);
}

void showGameOfLifeSettings(void) {
    /*
     * Game of Life button pressed
     */
    GameOfLifeRunning = false;
    showingGameOfLife = false;
    Display.clearDisplay(COLOR_DEMO_BACKGROUND);
    TouchButtonBack.drawButton();
    TouchSliderGameOfLifeSpeed.drawSlider();
    TouchButtonNewGame.drawButton();
    TouchButtonStartStopGame.drawButton();
    TouchButtonGameOfLifeDying.drawButton();
}

void initNewGameOfLife(void) {
    GameOfLifeInitialized = true;
    initGameOfLife();
}

void doGameOfLifeSpeed(Slider *aTheTouchedSlider, uint16_t aSliderValue) {
    aSliderValue = aSliderValue / 25;
    const char *tValueString = "";
    switch (aSliderValue) {
    case 0:
        GameOfLifeDelay = 8000;
        tValueString = "slowest";
        break;
    case 1:
        GameOfLifeDelay = 2000;
        tValueString = "slow   ";
        break;
    case 2:
        GameOfLifeDelay = 500;
        tValueString = "normal ";
        break;
    case 3:
        GameOfLifeDelay = 0;
        tValueString = "fast   ";
        break;
    }
    aTheTouchedSlider->printValue(tValueString);
    aTheTouchedSlider->setValueAndDrawBar(aSliderValue * 25);
}

void doGameOfLifeDying(Button *aTheTouchedButton, int16_t aValue) {
    GameOfLifeShowDying = aValue;
}

void doNewGameOfLife(Button *aTheTouchedButton, int16_t aValue) {
    /*
     * Let slider Start/Stop and New game buttons active as invisible GUI
     */
    TouchButtonGameOfLifeDying.deactivate();

    initNewGameOfLife();
    drawGameOfLife();

    sMillisSinceLastDemoOutput = 0;
    GameOfLifeRunning = true;
    showingGameOfLife = true;
    TouchButtonStartStopGame.setValue(GameOfLifeRunning);
}

void doStartStopGameOfLife(Button *aTheTouchedButton, int16_t aValue) {
    if (showingGameOfLife) {
        GameOfLifeRunning = !aValue;
        TouchButtonStartStopGame.setValue(GameOfLifeRunning);
        Button::playFeedbackTone();
    }
}

/*
 * Settings page GUI functions
 */
void showSettings(void) {
    /*
     * Settings button pressed
     */
    Display.clearDisplay(COLOR_DEMO_BACKGROUND);
    TouchButtonBack.drawButton();
#if defined(SUPPORT_LOCAL_DISPLAY)
    drawBacklightElements();
#endif
    TouchSliderGameOfLifeSpeed.drawSlider();
    TouchSliderAction.drawSlider();
    TouchSliderActionWithoutBorder.drawSlider();
    mCurrentApplication = APPLICATION_SETTINGS;
}

void showGuiDemoMenu(void) {
    TouchButtonBack.deactivate();

    Display.clearDisplay(COLOR_DEMO_BACKGROUND);
#if defined(MAIN_HOME_AVAILABLE)
    TouchButtonMainHome.drawButton();
#endif
    TouchButtonChartDemo.drawButton();
    TouchButtonGameOfLife.drawButton();
    GameOfLifeInitialized = false;
    TouchButtonDrawDemo.drawButton();
    TouchButtonDemoSettings.drawButton();

#if defined(SUPPORT_LOCAL_DISPLAY)
    TouchButtonFont.drawButton();
    TouchButtonADS7846Channels.drawButton();
    TouchButtonCalibration.drawButton();
#endif

    mCurrentApplication = APPLICATION_MENU;
}

/*
 * Charts page GUI functions
 */
void showCharts(void) {
    Display.clearDisplay(COLOR_DEMO_BACKGROUND);
    TouchButtonBack.drawButton();
    showChartDemo();
    mCurrentApplication = APPLICATION_CHART;
}

#if defined(SUPPORT_LOCAL_DISPLAY)
void showFont(void) {
    Display.clearDisplay(COLOR_DEMO_BACKGROUND);
    TouchButtonBack.drawButton();

    uint16_t tXPos;
    uint16_t tYPos = 10;
    unsigned char tChar = 0x20;
    for (uint8_t i = 14; i != 0; i--) {
        tXPos = 10;
        for (uint8_t j = 16; j != 0; j--) {
#if defined(SUPPORT_LOCAL_DISPLAY) && defined(DISABLE_REMOTE_DISPLAY)
            tXPos = Display.drawChar(tXPos, tYPos, tChar, 1, COLOR16_BLACK, COLOR16_YELLOW) + 4;
#else
            tXPos = Display.drawChar(tXPos, tYPos, tChar, TEXT_SIZE_11, COLOR16_BLACK, COLOR16_YELLOW) + 4;
#endif
            tChar++;
        }
        tYPos += TEXT_SIZE_11_HEIGHT + 4;
    }
}

/*
 * ADS7846 page GUI functions
 */
void doADS7846Channels(Button *aTheTouchedButton, int16_t aValue) {
    Button::deactivateAll();
    mCurrentApplication = APPLICATION_ADS7846_CHANNELS;
    Display.clearDisplay(COLOR_DEMO_BACKGROUND);
    uint16_t tPosY = 30;
    // draw text
    for (uint8_t i = 0; i < 8; ++i) {
        Display.drawText(90, tPosY, (const __FlashStringHelper *) ADS7846ChannelStrings[i], TEXT_SIZE_22, COLOR16_RED, COLOR_DEMO_BACKGROUND);
        tPosY += TEXT_SIZE_22_HEIGHT;
    }
    TouchButtonBack.drawButton();
}

#define BUTTON_CHECK_INTERVAL 20
void ADS7846DisplayChannels(void) {
    uint16_t tPosY = 30;
    int16_t tTemp;
    bool tUseDiffMode = true;
    bool tUse12BitMode = false;

    /*
     * Display values for the 8 ADS7846 channels
     */
    for (uint8_t i = 0; i < 8; ++i) {
        if (i == 4) {
            tUseDiffMode = false;
        }
        if (i == 2) {
            tUse12BitMode = true;
        }
        tTemp = TouchPanel.readChannel(ADS7846ChannelMapping[i], tUse12BitMode, tUseDiffMode, 2);
        snprintf(sStringBuffer, sizeof sStringBuffer, "%04u", tTemp);
        Display.drawText(15, tPosY, sStringBuffer, TEXT_SIZE_22, COLOR16_RED, COLOR_DEMO_BACKGROUND);
        tPosY += TEXT_SIZE_22_HEIGHT;
    }
}

#endif //

/**
 * @}
 */
#if defined(BUTTON_IS_DEFINED_LOCALLY)
#undef BUTTON_IS_DEFINED_LOCALLY
#undef Button
#undef AutorepeatButton
#undef Slider
#undef Display
#endif

#endif // _GUI_DEMO_HPP
