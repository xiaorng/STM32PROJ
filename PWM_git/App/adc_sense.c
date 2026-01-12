
// App/adc_sense.c
#include "adc_sense.h"

#ifndef ADC_DMA_SAMPLES
#define ADC_DMA_SAMPLES 64   // 可按需改 32/64/128
#endif

static uint16_t s_dma_buf[ADC_DMA_SAMPLES];
volatile uint16_t adc_avg = 0;

void adc_start_dma(ADC_HandleTypeDef *hadc){
    HAL_ADC_Start_DMA(hadc, (uint32_t*)s_dma_buf, ADC_DMA_SAMPLES);
}

// 在 stm32f4xx_it.c 里已经有 DMA/ADC 中断向 HAL 转发，
// 所以这里用 HAL 的回调进行均值计算。
// 注意：这个是半传输/全传输回调里二选一都可以，你按需保留一个。

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc){
    // DMA 缓冲区填满：算一次平均
    uint32_t sum = 0;
    for (int i = 0; i < ADC_DMA_SAMPLES; ++i) sum += s_dma_buf[i];
    adc_avg = (uint16_t)(sum / ADC_DMA_SAMPLES);
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc){
    // 半缓冲也可以做一次平均，增加刷新率
    uint32_t sum = 0;
    for (int i = 0; i < ADC_DMA_SAMPLES/2; ++i) sum += s_dma_buf[i];
    uint16_t half_avg = (uint16_t)(sum / (ADC_DMA_SAMPLES/2));
    // 简单一阶 IIR，减抖
    adc_avg = (adc_avg + half_avg) >> 1;
}
