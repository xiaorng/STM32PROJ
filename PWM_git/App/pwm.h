
#pragma once
#include "stm32f4xx_hal.h"
#include <stdint.h>
void pwm_init(TIM_HandleTypeDef *htim, uint32_t channel);
void pwm_set_percent(uint8_t percent);
void pwm_shutdown(void);
void pwm_apply_percent(uint8_t pct);
uint8_t pwm_get_percent(void);
