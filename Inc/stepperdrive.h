/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __STEPPERDRIVE_H
#define __STEPPERDRIVE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "menu.h"

#define STEPPER_STEP_Pin GPIO_PIN_1
#define STEPPER_STEP_GPIO_Port GPIOD
#define STEPPER_DIR_Pin GPIO_PIN_2
#define STEPPER_DIR_GPIO_Port GPIOD

#define ENCODER_RESOLUTION 4000
#define STEPS_PER_REVOLUTION 9600    // 3 x 3200  (3 rot./rev. 5mm / leadscrew rev.)
#define LEADSCREW_PITCH 5
#define _ENCODER_MAX_COUNT 0x00ffffff

void Error_Handler(void);
void setFeedVal(float feedv);
float get_pitch_selection(uint32_t value);
void set_lock_nut(void);
void unset_lock_nut(void);
void set_move_left(void);
void set_move_right(void);
uint32_t get_spindlePosition(void);
void set_feed_direction_right(void);
void set_feed_direction_left(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
