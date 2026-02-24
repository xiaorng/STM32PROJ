/*
 * fmac_rt.h  - Real-time FMAC FIR filter for ADC current filtering
 *
 * Usage:
 *   fmac_rt_init()       -> call once after MX_FMAC_Init()
 *   fmac_rt_restart()    -> clear FMAC delay line before a capture session
 *   fmac_rt_feed()       -> call from ADC ISR
 *   fmac_rt_get_stats()  -> fetch current statistics snapshot
 */

#ifndef FMAC_RT_H_
#define FMAC_RT_H_

#include <stdint.h>

/* Filter parameters */
#define FMAC_RT_TAPS             16U
#define FMAC_RT_SETTLE_SAMPLES   (FMAC_RT_TAPS + 2U)

/* 统计数据 */
typedef struct {
  uint32_t count;         /* total samples (including settle phase) */
  uint32_t valid_count;   /* settled samples used in stats */
  int64_t  raw_sum;       /* 原始值累加 (用于算均值) */
  int64_t  filt_sum;      /* 滤波值累加 */
  int64_t  raw_sq_sum;    /* 原始值平方和 (用于算方差/噪声) */
  int64_t  filt_sq_sum;   /* 滤波值平方和 */
  int16_t  raw_min, raw_max;
  int16_t  filt_min, filt_max;
} fmac_rt_stats_t;

/* 初始化 FMAC 为实时 FIR 模式 */
void fmac_rt_init(void);

/* 重启 FMAC: 清空延迟线, 在 adcstart 时调用 */
void fmac_rt_restart(void);

/* 喂一个 ADC 原始值 (12-bit → 内部转 Q15), 返回滤波后的 Q15 值
 * 在 ADC ISR 里调用, 必须快 */
int16_t fmac_rt_feed(uint16_t adc_raw);

/* 获取/重置统计 */
fmac_rt_stats_t fmac_rt_get_stats(void);
void fmac_rt_reset_stats(void);
void fmac_rt_set_active(uint8_t active);
uint8_t fmac_rt_is_active(void);

/* 采样环形缓冲区 (给 CLI dump 用) */
#define FMAC_RT_LOG_SIZE  256
typedef struct {
  int16_t raw;
  int16_t filt;
} fmac_rt_sample_t;

extern volatile fmac_rt_sample_t fmac_rt_log[FMAC_RT_LOG_SIZE];
extern volatile uint32_t fmac_rt_log_idx;

#endif /* FMAC_RT_H_ */
