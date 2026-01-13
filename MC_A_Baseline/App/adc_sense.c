#include "adc_sense.h"



static __IO uint16_t s_adc_buf[ADC_DMA_BUF_LEN];

void ADC1_DMA_Init_PA0(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    // PA0 -> ADC1_IN0
    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_0;
    g.Mode = GPIO_MODE_ANALOG;
    g.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &g);

    // DMA2 Stream0 Channel0 为 ADC1
    hdma_adc1.Instance = DMA2_Stream0;
    hdma_adc1.Init.Channel = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_adc1.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    HAL_DMA_Init(&hdma_adc1);

    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);

    // NVIC for DMA
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

    // ADC1 基本配置
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4; // 84MHz/4=21MHz
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.NbrOfDiscConversion = 0;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef sChan = {0};
    sChan.Channel = ADC_CHANNEL_0; // PA0
    sChan.Rank = 1;
    sChan.SamplingTime = ADC_SAMPLETIME_56CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sChan);

    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)s_adc_buf, ADC_DMA_BUF_LEN);
}

uint16_t adc_get_latest(void) {
    // DMA 循环缓冲，读取当前最后一个样本
    uint32_t idx = (ADC_DMA_BUF_LEN - 1) & 0xFFFFFFFFu;
    return s_adc_buf[idx];
}

uint16_t adc_get_avg(void) {
    uint32_t acc = 0;
    for (int i = 0; i < ADC_DMA_BUF_LEN; ++i) acc += s_adc_buf[i];
    return (uint16_t)(acc / ADC_DMA_BUF_LEN);
}
