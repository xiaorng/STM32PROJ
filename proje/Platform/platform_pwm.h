/*
 * platform_pwm.h
 *
 *  Created on: 2026年1月7日
 *      Author: SYRLIST
 */

#ifndef PLATFORM_PWM_H_
#define PLATFORM_PWM_H_
#pragma once
#include <stdint.h>
#include "stm32f4xx_hal.h"
 #include "stm32f4xx_hal_tim.h"

// 绑定一个PWM通道（会自动 Start）
// 返回0成功，负数失败
int platform_pwm_attach(TIM_HandleTypeDef *htim, uint32_t channel);

// duty: 0~100
void platform_pwm_set_duty(uint8_t duty);

// 读当前 duty（0~100）
uint8_t platform_pwm_get_duty(void);





#endif /* PLATFORM_PWM_H_ */
