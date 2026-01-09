/*
 * platform_pwm.h
 *
 *  Created on: 2026年1月9日
 *      Author: SYRLIST
 */

#pragma once
#include "stm32f4xx_hal.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int  platform_pwm_bind(TIM_HandleTypeDef *htim);
int  platform_pwm_start(uint32_t channel);
int  platform_pwm_stop(uint32_t channel);

/* duty_percent: 0~100 */
int  platform_pwm_set_duty_percent(uint32_t channel, uint8_t duty_percent);
uint8_t platform_pwm_get_duty_percent(uint32_t channel);

#ifdef __cplusplus
}
#endif
 /* PLATFORM_PWM_H_ */
