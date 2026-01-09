/*
 * platform_pwm.c
 *
 *  Created on: 2026年1月9日
 *      Author: SYRLIST
 */
#include "platform_pwm.h"

static TIM_HandleTypeDef *s_htim = NULL;
static uint8_t s_duty_ch1 = 0, s_duty_ch2 = 0, s_duty_ch3 = 0;

static uint8_t *duty_slot(uint32_t ch)
{
    if (ch == TIM_CHANNEL_1) return &s_duty_ch1;
    if (ch == TIM_CHANNEL_2) return &s_duty_ch2;
    if (ch == TIM_CHANNEL_3) return &s_duty_ch3;
    return NULL;
}

int platform_pwm_bind(TIM_HandleTypeDef *htim)
{
    if (!htim) return -1;
    s_htim = htim;
    return 0;
}

int platform_pwm_start(uint32_t channel)
{
    if (!s_htim) return -1;
    if (HAL_TIM_PWM_Start(s_htim, channel) != HAL_OK) return -2;
    return 0;
}

int platform_pwm_stop(uint32_t channel)
{
    if (!s_htim) return -1;
    if (HAL_TIM_PWM_Stop(s_htim, channel) != HAL_OK) return -2;
    return 0;
}

int platform_pwm_set_duty_percent(uint32_t channel, uint8_t duty_percent)
{
    if (!s_htim) return -1;
    if (duty_percent > 100) duty_percent = 100;

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(s_htim);
    uint32_t ccr = ((arr + 1) * (uint32_t)duty_percent) / 100;

    __HAL_TIM_SET_COMPARE(s_htim, channel, ccr);

    uint8_t *slot = duty_slot(channel);
    if (slot) *slot = duty_percent;

    return 0;
}

uint8_t platform_pwm_get_duty_percent(uint32_t channel)
{
    uint8_t *slot = duty_slot(channel);
    return slot ? *slot : 0;
}
