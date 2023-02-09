/**
 * GameOfLife.hpp
 *
 * Implements the Game Of Life on a (Display) on a byte array
 * Cells outside the borders are taken as empty
 *
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

#ifndef _GAME_OF_LIFE_HPP
#define _GAME_OF_LIFE_HPP

#include "GameOfLife.h"

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

unsigned long sLastFrameChangeMillis = 0; // Millis of last no tGameOfLifeByteArray change
uint16_t sCurrentGameOfLifeGeneration = 0;
uint16_t drawcolor[5]; // color scheme for EMPTY_CELL_COLOR, ALIVE_COLOR_INDEX etc.

uint8_t (*tGameOfLifeByteArray)[GAME_OF_LIFE_Y_SIZE];

/*
 * Checks if the cell is alive
 * Cells outside the borders are taken as empty
 */
uint8_t isAlive(uint8_t x, uint8_t y) {
    if ((x < GAME_OF_LIFE_X_SIZE) && (y < GAME_OF_LIFE_Y_SIZE)) {
        if (tGameOfLifeByteArray[x][y] & CELL_IS_ALIVE) {
            return 1;
        }
    }
    return 0;
}

uint8_t countNeighbors(uint8_t x, uint8_t y) {
    uint8_t count = 0;

    //3 above
    if (isAlive(x - 1, y - 1)) {
        count++;
    }
    if (isAlive(x, y - 1)) {
        count++;
    }
    if (isAlive(x + 1, y - 1)) {
        count++;
    }

    //2 on each side
    if (isAlive(x - 1, y)) {
        count++;
    }
    if (isAlive(x + 1, y)) {
        count++;
    }

    //3 below
    if (isAlive(x - 1, y + 1)) {
        count++;
    }
    if (isAlive(x, y + 1)) {
        count++;
    }
    if (isAlive(x + 1, y + 1)) {
        count++;
    }

    return count;
}

/**
 * This implements the rule of Game of Life and creates and dies cells by setting the appropriate flags
 */
void playGameOfLife(void) {
    bool tFrameHasChanged = false;

    //update cells
    for (uint_fast8_t x = 0; x < GAME_OF_LIFE_X_SIZE; x++) {
        for (uint_fast8_t y = 0; y < GAME_OF_LIFE_Y_SIZE; y++) {
            /*
             * Get the values for the rule
             */
            uint8_t tNeightborsCount = countNeighbors(x, y);
            uint8_t tNewCellValue = 0;
            /*
             * Apply the rule
             */
            if (isAlive(x, y)) {
                if (((tNeightborsCount < 2) || (tNeightborsCount > 3))) {
                    /*
                     * Let cell die in next iteration.
                     * Do not clear the alive flag, it is required for the alive check of the next processed cells.
                     * The alive flag is removed later by the drawing function.
                     */
                    tFrameHasChanged = true;
                    tNewCellValue = CELL_IS_ALIVE | CELL_JUST_DIED;
                }
            } else {
                if ((tNeightborsCount == 3)) {
                    // create new cell
                    tFrameHasChanged = true;
                    tNewCellValue = CELL_IS_NEW;
                }
            }
            if (tNewCellValue != 0) {
                tGameOfLifeByteArray[x][y] = tNewCellValue;
            }
        }
    }
    if (tFrameHasChanged) {
        sLastFrameChangeMillis = millis();
    }
}

/**
 * Convert new cells to alive cells and handle dying history.
 * then draw all cells, which are alive or changed.
 * Fill cell region with color corresponding to current cell state
 */
void drawGameOfLife(void) {
    uint_fast16_t tPosX = 0;
    for (uint_fast8_t x = 0; x < GAME_OF_LIFE_X_SIZE; x++) {
#if DISPLAY_HEIGHT > 0xFF
        uint_fast16_t tPosY = 0;
#else
        uint_fast8_t tPosY = 0;
#endif
        for (uint_fast8_t y = 0; y < GAME_OF_LIFE_Y_SIZE; y++) {
            uint8_t tCellValue = tGameOfLifeByteArray[x][y];
            if (tCellValue != CELL_IS_EMPTY) {
                /*
                 * Process all not empty cells
                 */
                uint8_t tColorIndex = ALIVE_COLOR_INDEX; // Red, green or blue
                if (tCellValue & CELL_JUST_DIED) { // die 1. time -> clear isAlive Bits and set die count to 2
                    tGameOfLifeByteArray[x][y] = 2;
                    tColorIndex = JUST_DIED_COLOR;
                } else if (tCellValue == 2) { // die 2.time (to show as history)
                    tGameOfLifeByteArray[x][y] = 1;
                    tColorIndex = LONGER_DEAD_COLOR;
                } else if (tCellValue == 1) { //die 3. time, delete now
                    tGameOfLifeByteArray[x][y] = CELL_IS_EMPTY;
                    tColorIndex = DEAD_COLOR_INDEX; // Clear with wWhite
                } else if (tCellValue & CELL_IS_NEW) { // new
                    tGameOfLifeByteArray[x][y] = CELL_IS_ALIVE;
//                    tColorIndex = ALIVE_COLOR_INDEX; // Red, green or blue
                }
                Display.fillRect(tPosX + 1, tPosY + 1, tPosX + (Display.getDisplayWidth() / GAME_OF_LIFE_X_SIZE) - 2,
                        tPosY + (Display.getDisplayHeight() / GAME_OF_LIFE_Y_SIZE) - 2, drawcolor[tColorIndex]);
            }
            tPosY += (Display.getDisplayHeight() / GAME_OF_LIFE_Y_SIZE);
        }
        tPosX += (Display.getDisplayWidth() / GAME_OF_LIFE_X_SIZE);
    }
}

/**
 * Sets display to grid color and clears each cell region,
 * resulting in a grid :-)
 */
void ClearScreenAndDrawGameOfLifeGrid(void) {
    Display.clearDisplay(drawcolor[EMPTY_CELL_COLOR]);
    uint_fast16_t tPosX = 0;
    for (uint_fast8_t x = 0; x < GAME_OF_LIFE_X_SIZE; x++) {
#if DISPLAY_HEIGHT > 0xFF
        uint_fast16_t tPosY = 0;
#else
        uint_fast8_t tPosY = 0;
#endif
        for (uint_fast8_t y = 0; y < GAME_OF_LIFE_Y_SIZE; y++) {
            //clear cells
            Display.fillRect(tPosX + 1, tPosY + 1, tPosX - 2 + (Display.getDisplayWidth() / GAME_OF_LIFE_X_SIZE),
                    tPosY + (Display.getDisplayHeight() / GAME_OF_LIFE_Y_SIZE) - 2, drawcolor[DEAD_COLOR_INDEX]);
            tPosY += (Display.getDisplayHeight() / GAME_OF_LIFE_Y_SIZE);
        }
        tPosX += (Display.getDisplayWidth() / GAME_OF_LIFE_X_SIZE);
    }
}

/**
 * Switch color scheme from blue -> green -> red -> blue
 */
void initGameOfLife(void) {
    sCurrentGameOfLifeGeneration = 0;

    // change color scheme
    drawcolor[DEAD_COLOR_INDEX] = COLOR16_WHITE;
    drawcolor[LONGER_DEAD_COLOR] = COLOR16_BLACK;

    switch (drawcolor[ALIVE_COLOR_INDEX]) {
    case COLOR16_GREEN:
        drawcolor[EMPTY_CELL_COLOR] = COLOR16_CYAN;
        drawcolor[ALIVE_COLOR_INDEX] = COLOR16_RED;
        drawcolor[JUST_DIED_COLOR] = COLOR16(32, 0, 0);
        break;

    case COLOR16_BLUE:
        drawcolor[EMPTY_CELL_COLOR] = COLOR16_PURPLE;
        drawcolor[ALIVE_COLOR_INDEX] = COLOR16_GREEN;
        drawcolor[JUST_DIED_COLOR] = COLOR16(0, 32, 0);
        break;

    default: // COLOR16_RED
        drawcolor[EMPTY_CELL_COLOR] = COLOR16_YELLOW;
        drawcolor[ALIVE_COLOR_INDEX] = COLOR16_BLUE;
        drawcolor[JUST_DIED_COLOR] = COLOR16(0, 0, 32);
        break;
    }
    if (!GameOfLifeShowDying) {
        // change colors
        drawcolor[LONGER_DEAD_COLOR] = COLOR16_WHITE;
        drawcolor[JUST_DIED_COLOR] = COLOR16_WHITE;
    }

    //generate random start data
#if defined(ARDUINO)
    for (unsigned int x = 0; x < GAME_OF_LIFE_X_SIZE; x++) {
        /*
         * get random data interpreted as 32 bit bit vector, which is more bits than GAME_OF_LIFE_Y_SIZE :-)
         * use random() & random() to set every 2. bit to zero
         */
        uint32_t tRandom32BitValue = random() & random();
        for (unsigned int y = 0; y < GAME_OF_LIFE_Y_SIZE; y++) {
            if (tRandom32BitValue & 1) {
                tGameOfLifeByteArray[x][y] = CELL_IS_ALIVE;
            } else {
                tGameOfLifeByteArray[x][y] = CELL_IS_EMPTY;
            }
            tRandom32BitValue >>= 1; // shift bit vector
        }
    }

    #else
    for (unsigned int x = 0; x < GAME_OF_LIFE_X_SIZE; x++) {
        uint32_t tRandom32BitValue = rand() & rand();
        for (unsigned int y = 0; y < GAME_OF_LIFE_Y_SIZE; y++) {
            if (tRandom32BitValue & 1) {
                tGameOfLifeByteArray[x][y] = CELL_IS_ALIVE;
            } else {
                tGameOfLifeByteArray[x][y] = CELL_IS_EMPTY;
            }
            tRandom32BitValue >>= 1; // shift bit vector
        }
    }
#endif

    ClearScreenAndDrawGameOfLifeGrid();

}

void drawGenerationText(void) {
    //draw current sCurrentGameOfLifeGeneration
    snprintf(sStringBuffer, sizeof sStringBuffer, "Gen.%3d", sCurrentGameOfLifeGeneration);
    Display.drawText(0, TEXT_SIZE_11_ASCEND, sStringBuffer, TEXT_SIZE_11, COLOR16(50, 50, 50), drawcolor[DEAD_COLOR_INDEX]);
}

void test(void) {
    tGameOfLifeByteArray[2][2] = CELL_IS_ALIVE;
    tGameOfLifeByteArray[3][2] = CELL_IS_ALIVE;
    tGameOfLifeByteArray[4][2] = CELL_IS_ALIVE;

    tGameOfLifeByteArray[6][2] = CELL_IS_ALIVE;
    tGameOfLifeByteArray[7][2] = CELL_IS_ALIVE;
    tGameOfLifeByteArray[6][3] = CELL_IS_ALIVE;
    tGameOfLifeByteArray[7][3] = CELL_IS_ALIVE;
}

#if defined(BUTTON_IS_DEFINED_LOCALLY)
#undef BUTTON_IS_DEFINED_LOCALLY
#undef Button
#undef AutorepeatButton
#undef Slider
#undef Display
#endif
#endif // _GAME_OF_LIFE_HPP
