#ifndef ADC_SENSE_H
#define ADC_SENSE_H
#include "stm32f4xx_hal.h"
#include <stdint.h>

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

#define ADC_DMA_BUF_LEN  64

void ADC1_DMA_Init_PA0(void);
uint16_t adc_get_latest(void);
uint16_t adc_get_avg(void);

#endif
