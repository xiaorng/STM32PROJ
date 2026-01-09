/*
 * app_main.c
 *
 *  Created on: 2026年1月9日
 *      Author: SYRLIST
 */
#include "app_main.h"
#include "platform_pwm.h"
#include "main.h" // 里面一般会有 extern TIM_HandleTypeDef htim1;
#include "tim.h"
static uint32_t s_last_ms = 0;
static int s_dir = 1;
static int s_duty = 0;

void app_init(void)
{
    /* 绑定 TIM1 */
    (void)platform_pwm_bind(&htim1);

    /* 启动三路 PWM */
    (void)platform_pwm_start(TIM_CHANNEL_1);
    (void)platform_pwm_start(TIM_CHANNEL_2);
    (void)platform_pwm_start(TIM_CHANNEL_3);

    /* 初始占空比=0 */
    (void)platform_pwm_set_duty_percent(TIM_CHANNEL_1, 0);
    (void)platform_pwm_set_duty_percent(TIM_CHANNEL_2, 0);
    (void)platform_pwm_set_duty_percent(TIM_CHANNEL_3, 0);
}

void app_loop(void)
{
    uint32_t now = HAL_GetTick();
    if (now - s_last_ms < 10) return; // 10ms 一步
    s_last_ms = now;

    s_duty += s_dir;
    if (s_duty >= 100) { s_duty = 100; s_dir = -1; }
    if (s_duty <= 0)   { s_duty = 0;   s_dir = 1;  }

    /* 三相“假装错相”（只是演示：CH2=+33%, CH3=+66% 循环偏移） */
    int d1 = s_duty;
    int d2 = s_duty + 33; if (d2 > 100) d2 -= 100;
    int d3 = s_duty + 66; if (d3 > 100) d3 -= 100;

    (void)platform_pwm_set_duty_percent(TIM_CHANNEL_1, (uint8_t)d1);
    (void)platform_pwm_set_duty_percent(TIM_CHANNEL_2, (uint8_t)d2);
    (void)platform_pwm_set_duty_percent(TIM_CHANNEL_3, (uint8_t)d3);
}


