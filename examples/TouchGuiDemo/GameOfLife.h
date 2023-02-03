/**
 * GameOfLife.h
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

#ifndef _GAME_OF_LIFE_H
#define _GAME_OF_LIFE_H

#define EMPTY_CELL_COLOR  (0)
#define ALIVE_COLOR_INDEX (1)
#define JUST_DIED_COLOR   (2)
#define LONGER_DEAD_COLOR (3)
#define DEAD_COLOR_INDEX  (4)

#define GAME_OF_LIFE_MAX_GEN  (600) //max generations
#if defined(SUPPORT_LOCAL_DISPLAY) && (defined(RAMEND) || defined(RAMSIZE)) && (RAMEND <= 0x8FF || RAMSIZE < 0x8FF)
// One cell requires one byte
#define GAME_OF_LIFE_X_SIZE   (20)
#define GAME_OF_LIFE_Y_SIZE   (15)
#else
#define GAME_OF_LIFE_X_SIZE   (40)
#define GAME_OF_LIFE_Y_SIZE   (30)
#endif

/*
 * Cell bit meanings
 */
#define CELL_IS_EMPTY   (0x00)
#define CELL_JUST_DIED  (0x04)
#define CELL_IS_ALIVE   (0x08)
#define CELL_IS_NEW     (0x10)
#define CELL_DIE_HISTORY_COUNTER_MASK (0x03) // Lower 2 bits are used as a die history counter from 2 to 0

extern uint8_t (*tGameOfLifeByteArray)[GAME_OF_LIFE_Y_SIZE];
extern uint16_t sCurrentGameOfLifeGeneration;

void initGameOfLife(void);
void playGameOfLife(void);
void drawGameOfLife(void);
void drawGenerationText(void);
void ClearScreenAndDrawGameOfLifeGrid(void);

extern bool GameOfLifeShowDying;

#endif // _GAME_OF_LIFE_H
