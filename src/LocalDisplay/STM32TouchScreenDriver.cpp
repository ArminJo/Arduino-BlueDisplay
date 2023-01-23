/*
 * STM32TouchScreenDriver.cpp
 *
 * Firmware for the TI ADS7846 resistive touch controller
 *
 *
 *  Copyright (C) 2015-2023  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of BlueDisplay https://github.com/ArminJo/android-blue-display.
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
 */
#if defined(STM32F10X) || defined(STM32F30X)
#include "STM32TouchScreenDriver.h"

#ifdef STM32F10X
#include <stm32f1xx.h>
#endif
#ifdef STM32F30X
#include <stm32f3xx.h>
#endif

TIM_HandleTypeDef TIM4Handle;

/**
 * Init the HY32D CS, Control/Data, WR, RD, and port D data pins
 */
void SSD1289_IO_initalize(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable the GPIO Clocks */
    __GPIOB_CLK_ENABLE();
    __GPIOD_CLK_ENABLE();

// CS pin, Register/Data select pin, WR pin, RD pin
    GPIO_InitStructure.Pin = HY32D_CS_PIN | HY32D_DATA_CONTROL_PIN | HY32D_WR_PIN | HY32D_RD_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(HY32D_CS_GPIO_PORT, &GPIO_InitStructure);
    // set HIGH
    HAL_GPIO_WritePin(HY32D_CS_GPIO_PORT, HY32D_CS_PIN | HY32D_WR_PIN | HY32D_RD_PIN, GPIO_PIN_SET);

    // 16 data pins
    GPIO_InitStructure.Pin = GPIO_PIN_All;
    HAL_GPIO_Init(HY32D_DATA_GPIO_PORT, &GPIO_InitStructure);
}

/**
 * Pins for touch panel CS, PENINT
 */
// Interrupt in pin
#define ADS7846_EXTI_IRQn                        EXTI1_IRQn

void ADS7846_IO_initalize(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable the GPIO Clock */
    __GPIOB_CLK_ENABLE();

    // CS pin
    GPIO_InitStructure.Pin = ADS7846_CS_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(ADS7846_CS_GPIO_PORT, &GPIO_InitStructure);
    // set HIGH
    HAL_GPIO_WritePin(ADS7846_CS_GPIO_PORT, ADS7846_CS_PIN, GPIO_PIN_SET);

    /* Configure pin as external interrupt  */
    GPIO_InitStructure.Pin = ADS7846_EXTI_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(ADS7846_EXTI_GPIO_PORT, &GPIO_InitStructure);

    /* Enable and set Button EXTI Interrupt to low priority */
    NVIC_SetPriority((IRQn_Type) (ADS7846_EXTI_IRQn), 12);
    HAL_NVIC_EnableIRQ((IRQn_Type) (ADS7846_EXTI_IRQn));
}

/**
 * first clear pending interrupts
 */
void ADS7846_clearAndEnableInterrupt(void) {
//  // Enable  interrupt
//  uint32_t tmp = (uint32_t) EXTI_BASE + EXTI_Mode_Interrupt
//          + (((STM32F3D_ADS7846_EXTI_LINE) >> 5) * 0x20);
//  /* Enable the selected external lines */
//  *(__IO uint32_t *) tmp |= (uint32_t) (1 << (STM32F3D_ADS7846_EXTI_LINE & 0x1F));

    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);
    NVIC_ClearPendingIRQ(ADS7846_EXTI_IRQn);
    NVIC_EnableIRQ(ADS7846_EXTI_IRQn);
}

void ADS7846_disableInterrupt(void) {
//  // Disable  interrupt
//  uint32_t tmp = (uint32_t) EXTI_BASE + EXTI_Mode_Interrupt
//          + (((STM32F3D_ADS7846_EXTI_LINE) >> 5) * 0x20);
//  /* Disable the selected external lines */
//  *(__IO uint32_t *) tmp &= ~(uint32_t) (1 << (STM32F3D_ADS7846_EXTI_LINE & 0x1F));
//

    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);
    // Disable NVIC Int
    NVIC_DisableIRQ(ADS7846_EXTI_IRQn);
    NVIC_ClearPendingIRQ(ADS7846_EXTI_IRQn);
}

/**
 * PWM for display backlight with timer 4
 */
#define STM32F3D_PWM_BL_TIMER                       TIM4
#define STM32F3D_PWM_BL_TIMER_CLOCK                 RCC_APB1Periph_TIM4

// for pin F6
#define STM32F3D_PWM_BL_GPIO_PIN                    GPIO_PIN_6
#define STM32F3D_PWM_BL_GPIO_PORT                   GPIOF
#define STM32F3D_PWM_BL_GPIO_CLK                    RCC_AHBPeriph_GPIOF
#define STM32F3D_PWM_BL_SOURCE                      GPIO_PinSource6
#define STM32F3D_PWM_BL_AF                          GPIO_AF_2
#define STM32F3D_PWM_BL_CHANNEL                     TIM_Channel_4
#define STM32F3D_PWM_BL_CHANNEL_INIT_COMMAND        TIM_OC4Init
#define STM32F3D_PWM_BL_CHANNEL_PRELOAD_COMMAND     TIM_OC4PreloadConfig

#define PWM_RESOLUTION_BACKLIGHT 0x100 // 0-255, 256 = constant-high
void PWM_BL_initalize(void) {
    TIM4Handle.Instance = TIM4;

    /* TIM4 clock enable */
    __TIM4_CLK_ENABLE();

    /* Compute the prescaler value for 1 kHz period with 256 resolution*/
    uint16_t PrescalerValue = (uint16_t) ((SystemCoreClock / 2) / (1000 * PWM_RESOLUTION_BACKLIGHT)) - 1;

    /* Time base configuration */
    TIM4Handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    TIM4Handle.Init.Period = PWM_RESOLUTION_BACKLIGHT - 1;
    TIM4Handle.Init.Prescaler = PrescalerValue;
    TIM4Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    HAL_TIM_Base_Init(&TIM4Handle);

    // Channel 4 for Pin F6
    TIM_OC_InitTypeDef TIM_OCInitStructure;
    /* PWM1 Mode configuration */
    TIM_OCInitStructure.OCMode = TIM_OCMODE_PWM1;
    TIM_OCInitStructure.OCFastMode = TIM_OCFAST_DISABLE;
    TIM_OCInitStructure.Pulse = PWM_RESOLUTION_BACKLIGHT / 2; //TIM_OPMODE_REPETITIVE
    TIM_OCInitStructure.OCPolarity = TIM_OCPOLARITY_HIGH;
    TIM_OCInitStructure.OCIdleState = TIM_OCIDLESTATE_RESET;
    TIM_OCInitStructure.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    TIM_OCInitStructure.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    HAL_TIM_PWM_ConfigChannel(&TIM4Handle, &TIM_OCInitStructure, TIM_CHANNEL_4);

    GPIO_InitTypeDef GPIO_InitStructure;
    __GPIOF_CLK_ENABLE();

// Configure pin as alternate function for timer
    GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStructure.Pin = STM32F3D_PWM_BL_GPIO_PIN;
    GPIO_InitStructure.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(STM32F3D_PWM_BL_GPIO_PORT, &GPIO_InitStructure);

//    STM32F3D_PWM_BL_CHANNEL_PRELOAD_COMMAND(STM32F3D_PWM_BL_TIMER, TIM_OCPreload_Enable);
//    TIM_ARRPreloadConfig(STM32F3D_PWM_BL_TIMER, ENABLE);
    /* TIM4 enable counter */
//    TIM_Cmd(STM32F3D_PWM_BL_TIMER, ENABLE);
    __HAL_TIM_ENABLE(&TIM4Handle);

// Start channel 4
    HAL_TIM_PWM_Start(&TIM4Handle, TIM_CHANNEL_4);
}

void PWM_BL_setOnRatio(uint32_t aOnTimePercent) {
    if (aOnTimePercent >= 100) {
        aOnTimePercent = 100;
    }
    TIM4Handle.Instance->CCR4 = (uint32_t) aOnTimePercent * 256 / 100;
//    TIM_SetCompare4(STM32F3D_PWM_BL_TIMER, (uint32_t) aOnTimePercent * 256 / 100);
}
#endif
