/*
 * platform_adc.h
 *
 *  Created on: 2026年1月8日
 *      Author: SYRLIST
 */

#ifndef PLATFORM_ADC_H_
#define PLATFORM_ADC_H_
#pragma once
#include <stdint.h>
#include "stm32f4xx_hal.h"

void platform_adc_bind(ADC_HandleTypeDef *hadc);

/**
 * 读内部温度（单位：0.1°C），成功返回 0，失败返回 -1
 * 说明：内部温感精度一般，适合趋势/演示
 */
int platform_temp_read_c10(int16_t *out_c10);

#endif /* PLATFORM_ADC_H_ */
