/*
 * platform_motor_b6612.c
 *
 *  Created on: 2026年1月9日
 *      Author: SYRLIST
 */
#include "platform_motor_b6612.h"
#include "platform_pwm.h"
#include "main.h"   // 这里才有 xxx_Pin / xxx_GPIO_Port
#include "tim.h"
#include <stdlib.h>

#define MOTOR_PWM_TIM      htim1
#define MOTOR_PWM_CH       TIM_CHANNEL_1   // 你选 CH1/CH2/CH3 都行

// 下面三个引脚：你自己在 CubeMX 给它们配成 GPIO Output
#define STBY_GPIO_Port     GPIOC
#define STBY_Pin           GPIO_PIN_0

#define AIN1_GPIO_Port     GPIOC
#define AIN1_Pin           GPIO_PIN_1

#define AIN2_GPIO_Port     GPIOC
#define AIN2_Pin           GPIO_PIN_2

static uint32_t arr_get(void){
    return __HAL_TIM_GET_AUTORELOAD(&MOTOR_PWM_TIM);
}

static void pwm_set_percent(uint8_t pct){ // 0~100
    if (pct > 100) pct = 100;
    uint32_t arr = arr_get();
    uint32_t ccr = (arr + 1) * pct / 100;
    __HAL_TIM_SET_COMPARE(&MOTOR_PWM_TIM, MOTOR_PWM_CH, ccr);
}

void motor_init(void)
{
    HAL_GPIO_WritePin(STBY_GPIO_Port, STBY_Pin, GPIO_PIN_SET); // enable TB6612
    HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_RESET);

    HAL_TIM_PWM_Start(&MOTOR_PWM_TIM, MOTOR_PWM_CH);
    pwm_set_percent(0);
}

void motor_set(int16_t duty) // -100~100
{
    if (duty > 100) duty = 100;
    if (duty < -100) duty = -100;

    if (duty == 0) {
        // 0：滑行（coast）
        HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_RESET);
        pwm_set_percent(0);
        return;
    }

    if (duty > 0) {
        HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_RESET);
        pwm_set_percent((uint8_t)duty);
    } else {
        HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_SET);
        pwm_set_percent((uint8_t)(-duty));
    }
}

void motor_brake(void)
{
    // 刹车：AIN1=1 AIN2=1 + PWM=0（不同板子可能略有差异）
    HAL_GPIO_WritePin(AIN1_GPIO_Port, AIN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(AIN2_GPIO_Port, AIN2_Pin, GPIO_PIN_SET);
    pwm_set_percent(0);
}


