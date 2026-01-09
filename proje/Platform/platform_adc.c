/*
 * platform_adc.c
 *
 *  Created on: 2026年1月8日
 *      Author: SYRLIST
 */
#include "platform_adc.h"

static ADC_HandleTypeDef *s_hadc = NULL;

void platform_adc_bind(ADC_HandleTypeDef *hadc)
{
    s_hadc = hadc;
}

static int adc_read_once(uint32_t channel, uint32_t sample_time, uint16_t *out)
{
    if (!s_hadc || !out) return -1;

    ADC_ChannelConfTypeDef cfg = {0};
    cfg.Channel      = channel;
    cfg.Rank         = 1;
    cfg.SamplingTime = sample_time;

    if (HAL_ADC_ConfigChannel(s_hadc, &cfg) != HAL_OK) return -1;
    if (HAL_ADC_Start(s_hadc) != HAL_OK) return -1;

    if (HAL_ADC_PollForConversion(s_hadc, 10) != HAL_OK) {
        (void)HAL_ADC_Stop(s_hadc);
        return -1;
    }

    *out = (uint16_t)HAL_ADC_GetValue(s_hadc);
    (void)HAL_ADC_Stop(s_hadc);
    return 0;
}

int platform_temp_read_c10(int16_t *out_c10)
{
    if (!out_c10) return -1;

    // 内部温度通道建议采样时间长一点，直接拉满省事
    const uint32_t st = ADC_SAMPLETIME_480CYCLES;

    uint16_t raw = 0;
    if (adc_read_once(ADC_CHANNEL_TEMPSENSOR, st, &raw) != 0) return -1;

    // 简化：假设 VDDA=3.3V
    // Vsense(mV) = raw * 3300 / 4095
    const int32_t vsense_mv = (int32_t)raw * 3300 / 4095;

    // 典型参数（不校准）：V25=760mV，Avg_Slope=2.5mV/°C
    // temp(°C) = (Vsense - 760)/2.5 + 25
    // temp(0.1°C) = 250 + (Vsense-760)*4
    int32_t t10 = 250 + (vsense_mv - 760) * 4;
    *out_c10 = (int16_t)t10;
    return 0;
}

