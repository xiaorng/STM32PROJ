/*
 * bsp_pwm.c
 *
 *  Created on: 2025年12月30日
 *      Author: SYRLIST
 */
#include "bsp_pwm.h"
#include "stm32f4xx_hal.h"

extern TIM_HandleTypeDef htim2;

#ifndef PWM_CH
#define PWM_CH TIM_CHANNEL_1
#endif

void BSP_PWM_Start(void)
{
  HAL_TIM_PWM_Start(&htim2, PWM_CH);
}

void BSP_PWM_Stop(void)
{
  HAL_TIM_PWM_Stop(&htim2, PWM_CH);
}

void BSP_PWM_SetDuty(uint16_t duty)
{
  __HAL_TIM_SET_COMPARE(&htim2, PWM_CH, duty);
}


