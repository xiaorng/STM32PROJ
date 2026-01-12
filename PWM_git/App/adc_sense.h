
#pragma once
// App/adc_sense.h
#pragma once
#include <stdint.h>
#include "stm32f4xx_hal.h"

// 平滑后的 ADC 平均值（你在 control_tick_1kHz 里用到）
extern volatile uint16_t adc_avg;

// 启动 ADC+DMA 采样
void adc_start_dma(ADC_HandleTypeDef *hadc);
