/*
 * platform_pwm.c
 *
 *  Created on: 2026年1月7日
 *      Author: SYRLIST
 */
#include "platform_pwm.h"

static TIM_HandleTypeDef *s_htim = NULL;
static uint32_t s_ch = 0;
static uint8_t s_duty = 0;

static void pwm_apply(uint8_t duty)
{
    if (!s_htim) return;

    if (duty > 100) duty = 100;

    // ARR = period-1, CCR = compare
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(s_htim);
    if (arr == 0) arr = 1;

    // 让 duty=100 时接近满占空比
    uint32_t ccr = (arr + 1u) * (uint32_t)duty / 100u;
    __HAL_TIM_SET_COMPARE(s_htim, s_ch, ccr);

    s_duty = duty;
}

int platform_pwm_attach(TIM_HandleTypeDef *htim, uint32_t channel)
{
    if (!htim) return -1;

    s_htim = htim;
    s_ch = channel;

    if (HAL_TIM_PWM_Start(s_htim, s_ch) != HAL_OK) {
        return -2;
    }

    pwm_apply(0);
    return 0;
}

void platform_pwm_set_duty(uint8_t duty)
{
    pwm_apply(duty);
}

uint8_t platform_pwm_get_duty(void)
{
    return s_duty;
}
