#ifndef PWM_H
#define PWM_H
#include "stm32f4xx_hal.h"

extern TIM_HandleTypeDef htim2;

void PWM_Init_TIM2_PA5_20kHz(void);
void PWM_SetDuty_Percent(uint8_t duty); // 0..100

#endif
