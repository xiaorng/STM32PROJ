/*
 * app_main.c
 *
 *  Created on: 2026年1月9日
 *      Author: SYRLIST
 */
#include "app_main.h"
#include "platform_pwm.h"
#include "tim.h"
#include "main.h"
#include <stdint.h>

static uint32_t s_last_ms = 0;
static int s_dir = 1;
static int s_duty = 0;

void app_init(void)
{
    /* start TIM1 PWM channels (CubeMX must enable CH1/CH2/CH3) */
    (void)HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    (void)HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
    (void)HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);

    (void)platform_pwm_set_duty_percent(TIM_CHANNEL_1, 0);
    (void)platform_pwm_set_duty_percent(TIM_CHANNEL_2, 0);
    (void)platform_pwm_set_duty_percent(TIM_CHANNEL_3, 0);
}

void app_loop(void)
{
    uint32_t now = HAL_GetTick();
    if ((now - s_last_ms) < 10) return; // 10ms step
    s_last_ms = now;

    s_duty += s_dir;
    if (s_duty >= 100) { s_duty = 100; s_dir = -1; }
    if (s_duty <= 0)   { s_duty = 0;   s_dir = 1;  }

    /* 3ch “phase-like” demo: d2=+33%, d3=+66% offset */
    int d1 = s_duty;
    int d2 = s_duty + 33; if (d2 > 100) d2 -= 100;
    int d3 = s_duty + 66; if (d3 > 100) d3 -= 100;

    (void)platform_pwm_set_duty_percent(TIM_CHANNEL_1, (uint8_t)d1);
    (void)platform_pwm_set_duty_percent(TIM_CHANNEL_2, (uint8_t)d2);
    (void)platform_pwm_set_duty_percent(TIM_CHANNEL_3, (uint8_t)d3);
}


