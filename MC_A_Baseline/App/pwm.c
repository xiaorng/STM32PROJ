#include "pwm.h"


void PWM_Init_TIM2_PA5_20kHz(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();

    // PA5 -> TIM2_CH1 (AF1)
    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_5;
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &g);

    // 计时器：期望 PWM ~20kHz
    // 假设 APB1 定时器时钟 = 84MHz（典型配置），则：
    // 84MHz / (PSC+1) / (ARR+1) = 20k
    // 取 PSC=83 => 84MHz/(84)=1MHz; 1MHz/(ARR+1)=20k => ARR=49
    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 83;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 49;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&htim2);

    TIM_OC_InitTypeDef sOC = {0};
    sOC.OCMode = TIM_OCMODE_PWM1;
    sOC.Pulse = 0; // 初始 0%
    sOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim2, &sOC, TIM_CHANNEL_1);

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

void PWM_SetDuty_Percent(uint8_t duty) {
    if (duty > 100) duty = 100;
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim2);
    uint32_t ccr = (arr + 1) * duty / 100;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, ccr);
}
