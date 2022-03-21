/*
 * FrequencyGeneratorPage.h
 *
 *  Copyright (C) 2015  Armin Joachimsmeyer
 *  Email: armin.joachimsmeyer@gmail.com
 *  License: GPL v3 (http://www.gnu.org/licenses/gpl.html)
 */

#ifndef _FREQUENCY_GENERATOR_PAGE_H
#define _FREQUENCY_GENERATOR_PAGE_H

#include "BDButton.h"

#include <inttypes.h>
#include <avr/pgmspace.h>

/**
 * From FrequencyGenerator page
 */
void initFrequencyGenerator(void);
void initFrequencyGeneratorPage(void);
void drawFrequencyGeneratorPage(void);
void startFrequencyGeneratorPage(void);
void loopFrequencyGeneratorPage(void);
void stopFrequencyGeneratorPage(void);

//extern BDButton TouchButtonFrequencyPage;

extern const char StringStop[] PROGMEM; // "Stop"

#endif // _FREQUENCY_GENERATOR_PAGE_H
#pragma once
