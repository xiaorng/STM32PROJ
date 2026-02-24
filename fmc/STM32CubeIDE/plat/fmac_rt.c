/*
 * fmac_rt.c  - Real-time FMAC FIR filter for ADC current filtering
 *
 * Architecture:
 *   ADC interrupt -> fmac_rt_feed() -> filtered output
 *
 * FMAC configured once at init via HAL, then data path uses direct register
 * access (WDATA/RDATA) for minimal ISR latency - same principle as
 * CORDIC PURE benchmark (20 cyc/call vs HAL's 1327 cyc/call).
 *
 *  Created on: 2026年2月
 *      Author: SYRLIST
 */

#include "fmac_rt.h"
#include "fmac.h"
#include "main.h"
#include "stm32g4xx_hal.h"
#include <string.h>

extern FMAC_HandleTypeDef hfmac;

/* ── 滤波器系数 ── */
static int16_t rt_coeffs[FMAC_RT_TAPS];
static int16_t rt_preload_zeros[FMAC_RT_TAPS];

/* ── 统计 ── */
static volatile fmac_rt_stats_t stats;
static volatile uint8_t rt_active = 0U;

/* ── 采样环形缓冲区 (CLI dump 用) ── */
volatile fmac_rt_sample_t fmac_rt_log[FMAC_RT_LOG_SIZE];
volatile uint32_t fmac_rt_log_idx = 0;

static void fmac_rt_configure_hw(void)
{
  FMAC_FilterConfigTypeDef cfg = {0};
  cfg.InputBaseAddress  = 0;
  cfg.InputBufferSize   = FMAC_RT_TAPS + 2U;
  cfg.InputThreshold    = FMAC_THRESHOLD_1;
  cfg.CoeffBaseAddress  = FMAC_RT_TAPS + 2U;
  cfg.CoeffBufferSize   = FMAC_RT_TAPS;
  cfg.OutputBaseAddress = (2U * FMAC_RT_TAPS) + 2U;
  cfg.OutputBufferSize  = 2;
  cfg.OutputThreshold   = FMAC_THRESHOLD_1;
  cfg.pCoeffA           = NULL;
  cfg.CoeffASize        = 0;
  cfg.pCoeffB           = rt_coeffs;
  cfg.CoeffBSize        = FMAC_RT_TAPS;
  cfg.Filter            = FMAC_FUNC_CONVO_FIR;
  cfg.InputAccess       = FMAC_BUFFER_ACCESS_NONE;
  cfg.OutputAccess      = FMAC_BUFFER_ACCESS_NONE;
  cfg.Clip              = FMAC_CLIP_ENABLED;
  cfg.P                 = FMAC_RT_TAPS;
  cfg.Q                 = 0;
  cfg.R                 = 0;

  if (HAL_FMAC_FilterConfig(&hfmac, &cfg) != HAL_OK) {
    Error_Handler();
  }

  if (HAL_FMAC_FilterPreload(&hfmac, rt_preload_zeros, FMAC_RT_TAPS, NULL, 0) != HAL_OK) {
    Error_Handler();
  }

  FMAC->PARAM = FMAC_PARAM_START | FMAC_FUNC_CONVO_FIR | (uint32_t)FMAC_RT_TAPS;
}

/* ──────────────────────────────────────────────────────────────────
 *  FMAC 内部 RAM 分配 (256 × 16-bit words):
 *
 *    X1 (input/delay):  addr 0,   size = TAPS + 2 = 18
 *    X2 (coefficients): addr 18,  size = TAPS     = 16
 *    Y  (output):       addr 34,  size = 2
 *    Total: 36 / 256
 *
 *  16-tap 移动平均: 每个系数 = 1/16 in Q15 = 2048
 * ────────────────────────────────────────────────────────────────── */

void fmac_rt_init(void)
{
  /* 16-tap moving average: each coeff = 32768/16 = 2048 (Q15) */
  for (uint32_t i = 0; i < FMAC_RT_TAPS; i++) {
    rt_coeffs[i] = 2048;
  }
  memset(rt_preload_zeros, 0, sizeof(rt_preload_zeros));
  fmac_rt_reset_stats();
  fmac_rt_log_idx = 0U;
  rt_active = 0U;
  fmac_rt_configure_hw();
}

/* ──────────────────────────────────────────────────────────────────
 *  fmac_rt_feed — 在 ADC ISR 里调用, 必须快
 *
 *  输入:  adc_raw = 12-bit ADC 值 (0..4095)
 *  输出:  滤波后的 Q15 值
 *
 *  ADC → Q15 转换:
 *    mid = 2048 (零电流对应 ADC 中间值)
 *    q15 = (adc - 2048) × 16  →  范围 ±32768
 *
 *  FMAC 直接寄存器操作:
 *    WDATA ← 输入   (写入 X1 buffer)
 *    RDATA → 输出   (读自 Y buffer, 自动阻塞到计算完成)
 * ────────────────────────────────────────────────────────────────── */
int16_t fmac_rt_feed(uint16_t adc_raw)
{
  /* 12-bit → Q15, 居中 */
  int16_t raw_q15 = (int16_t)(((int32_t)adc_raw - 2048) << 4);

  /* 喂入 FMAC */
  FMAC->WDATA = (uint32_t)(uint16_t)raw_q15;

  /* 等输出就绪 (YEMPTY=0 表示有数据) */
  while (FMAC->SR & FMAC_SR_YEMPTY) { /* 通常 <10 cycles */ }

  /* 读滤波结果 */
  int16_t filt_q15 = (int16_t)(uint16_t)FMAC->RDATA;

  /* Update stats (skip settle samples). */
  stats.count++;
  if (stats.count > FMAC_RT_SETTLE_SAMPLES) {
    stats.valid_count++;
    stats.raw_sum   += raw_q15;
    stats.filt_sum  += filt_q15;
    stats.raw_sq_sum  += (int64_t)raw_q15 * raw_q15;
    stats.filt_sq_sum += (int64_t)filt_q15 * filt_q15;

    if (raw_q15 < stats.raw_min)   stats.raw_min = raw_q15;
    if (raw_q15 > stats.raw_max)   stats.raw_max = raw_q15;
    if (filt_q15 < stats.filt_min) stats.filt_min = filt_q15;
    if (filt_q15 > stats.filt_max) stats.filt_max = filt_q15;
  }

  /* ── 存入环形缓冲区 ── */
  uint32_t idx = fmac_rt_log_idx % FMAC_RT_LOG_SIZE;
  fmac_rt_log[idx].raw  = raw_q15;
  fmac_rt_log[idx].filt = filt_q15;
  fmac_rt_log_idx++;

  return filt_q15;
}

void fmac_rt_restart(void)
{
  (void)HAL_FMAC_FilterStop(&hfmac);
  fmac_rt_configure_hw();
}

fmac_rt_stats_t fmac_rt_get_stats(void)
{
  fmac_rt_stats_t s;
  __disable_irq();
  s = stats;
  __enable_irq();
  return s;
}

void fmac_rt_reset_stats(void)
{
  __disable_irq();
  memset((void*)&stats, 0, sizeof(stats));
  fmac_rt_log_idx = 0U;
  stats.raw_min  =  32767;
  stats.raw_max  = -32768;
  stats.filt_min =  32767;
  stats.filt_max = -32768;
  __enable_irq();
}

void fmac_rt_set_active(uint8_t active)
{
  __disable_irq();
  rt_active = (active != 0U) ? 1U : 0U;
  __enable_irq();
}

uint8_t fmac_rt_is_active(void)
{
  return rt_active;
}
