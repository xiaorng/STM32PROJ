/*
 * cli.c
 *
 *  Created on: 2026年2月5日
 *      Author: SYRLIST
 */

#include "fmac.h"
#include "cli.h"
#include "log.h"
#include "main.h"
#include "cordic.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include "fmac_rt.h"
#include "tim.h"
#include "adc.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#define CORDIC_MAX_N 5000
#define FMAC_TAPS        32
#define FMAC_INPUT_N     5000
#define PI M_PI
#define CLI_LINE_MAX 96
extern CORDIC_HandleTypeDef hcordic;
extern FMAC_HandleTypeDef hfmac;


static inline uint32_t dwt_cycles(void) { return DWT->CYCCNT; }

static void cordic_sincos_init(void)
{
  CORDIC_ConfigTypeDef cfg = {0};
  cfg.Function  = CORDIC_FUNCTION_COSINE;     // 输出 cos + sin
  cfg.Precision = CORDIC_PRECISION_6CYCLES;
  cfg.Scale     = CORDIC_SCALE_0;
  cfg.NbWrite   = CORDIC_NBWRITE_1;
  cfg.NbRead    = CORDIC_NBREAD_2;
  cfg.InSize    = CORDIC_INSIZE_32BITS;
  cfg.OutSize   = CORDIC_OUTSIZE_32BITS;

  if (HAL_CORDIC_Configure(&hcordic, &cfg) != HAL_OK) {
    LOGE("cordic cfg fail");
  }
}




static char line[CLI_LINE_MAX];
static volatile uint16_t linelen = 0;
static volatile uint8_t line_ready = 0;

// ---- DWT cycle counter (for bench) ----
static void dwt_init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}



static uint32_t bench_loop_cycles(uint32_t n)
{
  volatile uint32_t s = 0;
  uint32_t t0 = DWT->CYCCNT;
  for (uint32_t i = 0; i < n; i++) s += i;
  uint32_t t1 = DWT->CYCCNT;
  (void)s;
  return (t1 - t0);
}
static inline int32_t float_to_q31(float x)
{
  // x in [-1, 1)
  if (x >= 0.999999f) x = 0.999999f;
  if (x < -1.0f) x = -1.0f;
  return (int32_t)(x * 2147483648.0f); // 2^31
}

static inline float q31_to_float(int32_t q)
{
  return (float)q / 2147483648.0f;
}

// angle: rad -> normalized to [-pi, pi] then to q31 where 1.0 == pi (常用映射)
static inline int32_t rad_to_cordic_q31(float rad)
{
  // wrap to [-pi, pi]
  while (rad >  PI) rad -= 2.0f*PI;
  while (rad < -PI) rad += 2.0f*PI;
  // map rad/pi to [-1,1)
  return float_to_q31(rad / PI);
}


static uint32_t bench_soft_sincos(uint32_t n)
{
  volatile float s=0, c=0;
  (void)s;
  (void)c;
  uint32_t t0 = DWT->CYCCNT;
  for (uint32_t i=0;i<n;i++){
    float a = (float)i * 0.001f;
    s = sinf(a);
    c = cosf(a);
  }
  uint32_t t1 = DWT->CYCCNT;
  return (t1 - t0);
}


static uint32_t bench_cordic_sincos(uint32_t n)
{
  // CORDIC sin/cos: input angle q31, output [cos, sin] q31（具体顺序依配置）
  int32_t in, out[2];

  uint32_t t0 = DWT->CYCCNT;
  for (uint32_t i=0;i<n;i++){
    float a = (float)i * 0.001f;
    in = rad_to_cordic_q31(a);

    // HAL API：根据Cube配置可能是 Calculate / CORDIC_Calculate 等
    // 常用是：HAL_CORDIC_Calculate(&hcordic, &in, out, 1, HAL_MAX_DELAY);
    HAL_CORDIC_Calculate(&hcordic, (int32_t*)&in, (int32_t*)out, 1, 1000);

    // 防止被优化掉
    volatile float cs = q31_to_float(out[0]);
    volatile float sn = q31_to_float(out[1]);
    (void)cs; (void)sn;
  }
  uint32_t t1 = DWT->CYCCNT;
  return (t1 - t0);
}
/* ── CORDIC 直接寄存器版 ─────────────────────────────────────────────
 *
 * CORDIC CSR 配置 (一次写入, 之后只操作 WDATA/RDATA):
 *   FUNC[3:0]   = 0  (Cosine → 输出 cos, sin)
 *   PREC[3:0]   = 6  (6 次迭代, ~20-bit 精度)
 *   SCALE[2:0]  = 0
 *   NRES        = 1  (读 2 个结果: cos + sin)
 *   NARGS       = 0  (写 1 个参数: 角度)
 *   RESSIZE     = 0  (32-bit 输出)
 *   ARGSIZE     = 0  (32-bit 输入)
 *
 * CSR = (6 << 4) | (1 << 19) = 0x00080060
 *
 * 每次调用只需:  WDATA = angle;  cos = RDATA;  sin = RDATA;
 * 硬件延迟: 6 个时钟周期 (CORDIC pipeline)
 * ──────────────────────────────────────────────────────────────────── */
#define CORDIC_CSR_SINCOS_6ITER  0x00080060u

static void cordic_reg_init(void)
{
  CORDIC->CSR = CORDIC_CSR_SINCOS_6ITER;
}

static uint32_t bench_cordic_sincos_reg(uint32_t n)
{
  volatile int32_t cos_q31, sin_q31;

  /* 确保 CSR 配置正确 */
  CORDIC->CSR = CORDIC_CSR_SINCOS_6ITER;

  uint32_t t0 = DWT->CYCCNT;
  for (uint32_t i = 0; i < n; i++) {
    float a = (float)i * 0.001f;
    int32_t in = rad_to_cordic_q31(a);

    CORDIC->WDATA = (uint32_t)in;     /* 写入角度 → 硬件开始计算 */
    cos_q31 = (int32_t)CORDIC->RDATA; /* 读 cos (阻塞到计算完成) */
    sin_q31 = (int32_t)CORDIC->RDATA; /* 读 sin */
  }
  uint32_t t1 = DWT->CYCCNT;

  (void)cos_q31; (void)sin_q31;
  return (t1 - t0);
}

/* 纯寄存器版, 跳过 float→q31 转换, 测纯硬件吞吐 */
static uint32_t bench_cordic_pure_reg(uint32_t n)
{
  volatile int32_t c, s;
  CORDIC->CSR = CORDIC_CSR_SINCOS_6ITER;

  /* 用 Q31 整数相位累加, 零浮点开销 */
  uint32_t phase = 0;
  const uint32_t step = (uint32_t)(0.001f / PI * 2147483648.0f);

  uint32_t t0 = DWT->CYCCNT;
  for (uint32_t i = 0; i < n; i++) {
    CORDIC->WDATA = phase;
    c = (int32_t)CORDIC->RDATA;
    s = (int32_t)CORDIC->RDATA;
    phase += step;
  }
  uint32_t t1 = DWT->CYCCNT;

  (void)c; (void)s;
  return (t1 - t0);
}

static void bench_cordic_reg_breakdown(uint32_t n,
                                       uint32_t *cyc_pack,
                                       uint32_t *cyc_hw,
                                       uint32_t *cyc_unpack)
{
  if (n == 0) { *cyc_pack = *cyc_hw = *cyc_unpack = 0; return; }

  /* CORDIC 配置 */
  CORDIC->CSR = CORDIC_CSR_SINCOS_6ITER;

  volatile int32_t cos_q31, sin_q31;
  volatile float cs, sn;

  uint32_t pack = 0, hw = 0, unpack = 0;

  for (uint32_t i = 0; i < n; i++) {
    /* 1) float -> q31 (含你的 rad wrap + 缩放) */
    uint32_t t0 = DWT->CYCCNT;
    float a = (float)i * 0.001f;
    int32_t in = rad_to_cordic_q31(a);
    uint32_t t1 = DWT->CYCCNT;
    pack += (t1 - t0);

    /* 2) 纯硬件：写入 + 两次读出 */
    t0 = DWT->CYCCNT;
    CORDIC->WDATA = (uint32_t)in;
    cos_q31 = (int32_t)CORDIC->RDATA;
    sin_q31 = (int32_t)CORDIC->RDATA;
    t1 = DWT->CYCCNT;
    hw += (t1 - t0);

    /* 3) q31 -> float */
    t0 = DWT->CYCCNT;
    cs = q31_to_float(cos_q31);
    sn = q31_to_float(sin_q31);
    t1 = DWT->CYCCNT;
    unpack += (t1 - t0);
  }

  (void)cs; (void)sn;

  *cyc_pack = pack;
  *cyc_hw = hw;
  *cyc_unpack = unpack;
}

static int16_t g_x[FMAC_INPUT_N];
static int16_t g_y_soft[FMAC_INPUT_N];
static int16_t g_y_hw[FMAC_INPUT_N];

static uint32_t lcg_state = 1;
static int16_t prng_q15(void)
{
  // 伪随机，范围大概在 [-0.5, 0.5)
  lcg_state = 1664525u * lcg_state + 1013904223u;
  int32_t v = (int32_t)(lcg_state >> 16) - 32768;
  return (int16_t)(v >> 1);
}

static void gen_input(uint32_t n)
{
  if (n > FMAC_INPUT_N) n = FMAC_INPUT_N;
  for (uint32_t i = 0; i < n; i++) g_x[i] = prng_q15();
}

static void fir_soft_q15_ma32(const int16_t *x, int16_t *y, uint32_t n)
{
  // 32tap 移动平均：y = (sum x[k]) / 32
  // q15 * q15 的严格实现很麻烦；这里用“平均”避开系数乘法：累加后右移 5 位
  for (uint32_t i = 0; i < n; i++) {
    int32_t acc = 0;
    for (uint32_t k = 0; k < FMAC_TAPS; k++) {
      int32_t idx = (int32_t)i - (int32_t)k;
      int16_t xv = (idx >= 0) ? x[idx] : 0;
      acc += xv;
    }
    // 平均
    acc >>= 5;
    // 饱和到 q15
    if (acc > 32767) acc = 32767;
    if (acc < -32768) acc = -32768;
    y[i] = (int16_t)acc;
  }
}

static uint32_t bench_soft_fir(uint32_t n)
{
  uint32_t t0 = dwt_cycles();
  fir_soft_q15_ma32(g_x, g_y_soft, n);
  uint32_t t1 = dwt_cycles();
  return (t1 - t0);
}

static int16_t fmac_coeffs[FMAC_TAPS];  /* 32-tap 移动平均系数 */
static int16_t fmac_preload_zeros[FMAC_TAPS];

static void fmac_fir_init(void)
{
	memset(fmac_preload_zeros, 0, sizeof(fmac_preload_zeros));
  /* 32-tap 移动平均: 每个系数 = 1/32 = 1024 in Q15 */
  for (int i = 0; i < FMAC_TAPS; i++) {
    fmac_coeffs[i] = (int16_t)1024;   /* round(32768 / 32) */
  }
}

/*
 * FMAC 内部 RAM = 256 x 16-bit words
 * 分配:
 *   X1 (input buffer):  addr 0,   size 64  (32 delay + 32 watermark)
 *   X2 (coefficients):  addr 64,  size 32
 *   Y  (output buffer): addr 96,  size 64
 *   Total = 160 / 256, OK
 */
static uint32_t bench_fmac_fir(uint32_t n)
{
  if (n > (FMAC_INPUT_N - 1)) n = FMAC_INPUT_N - 1;

  memset(g_y_hw, 0, sizeof(int16_t) * (n + 1));

  FMAC_FilterConfigTypeDef cfg = {0};
  cfg.InputBaseAddress  = 0;
  cfg.InputBufferSize   = 64;
  cfg.InputThreshold    = FMAC_THRESHOLD_1;
  cfg.CoeffBaseAddress  = 64;
  cfg.CoeffBufferSize   = FMAC_TAPS;
  cfg.OutputBaseAddress = 96;
  cfg.OutputBufferSize  = 64;
  cfg.OutputThreshold   = FMAC_THRESHOLD_1;
  cfg.pCoeffA           = NULL;
  cfg.CoeffASize        = 0;
  cfg.pCoeffB           = fmac_coeffs;
  cfg.CoeffBSize        = FMAC_TAPS;
  cfg.Filter            = FMAC_FUNC_CONVO_FIR;
  cfg.InputAccess       = FMAC_BUFFER_ACCESS_POLLING;
  cfg.OutputAccess      = FMAC_BUFFER_ACCESS_POLLING;
  cfg.Clip              = FMAC_CLIP_ENABLED;
  cfg.P                 = FMAC_TAPS;
  cfg.Q                 = 0;
  cfg.R                 = 0;

  if (HAL_FMAC_FilterConfig(&hfmac, &cfg) != HAL_OK) {
    LOGE("FMAC config fail");
    return 0;
  }

  if (HAL_FMAC_FilterPreload(&hfmac, fmac_preload_zeros, FMAC_TAPS, NULL, 0) != HAL_OK) {
    LOGE("FMAC preload fail");
    return 0;
  }

  /* 请求 n+1 个输出: [0]=pipeline零, [1..n]=真实结果 */
  uint16_t outRemain = (uint16_t)(n + 1);
  if (HAL_FMAC_FilterStart(&hfmac, g_y_hw, &outRemain) != HAL_OK) {
    LOGE("FMAC start fail");
    return 0;
  }

  uint32_t t0 = DWT->CYCCNT;

  uint16_t fed = 0;
  while (fed < n) {
    uint16_t chunk = (uint16_t)(n - fed);
    HAL_StatusTypeDef st = HAL_FMAC_AppendFilterData(&hfmac, &g_x[fed], &chunk);
    if (st == HAL_OK) {
      fed += chunk;
    } else if (st == HAL_BUSY) {
      HAL_FMAC_PollFilterData(&hfmac, 10);
    } else {
      LOGE("FMAC append fail (%d) at %u", st, fed);
      break;
    }
  }

  HAL_FMAC_PollFilterData(&hfmac, 1000);

  uint32_t t1 = DWT->CYCCNT;

  HAL_FMAC_FilterStop(&hfmac);

  volatile int16_t sink = g_y_hw[n];
  (void)sink;

  return (t1 - t0);
}


/* diff_count removed: fmaccmp now uses inline comparison with pipeline offset */
static int32_t in_vec[CORDIC_MAX_N];
static int32_t out_vec[CORDIC_MAX_N * 2];

static uint32_t bench_cordic_vec(uint32_t n)
{
  if (n > CORDIC_MAX_N) n = CORDIC_MAX_N;

  // 生成输入：用 Q31 相位累加，避免 while wrap 的开销
  // Q31 表示 (rad / PI) in [-1,1)
  const float step_rad = 0.001f;
  uint32_t step_u = (uint32_t)((step_rad / PI) * 2147483648.0f); // 2^31 * step/PI
  uint32_t phase_u = 0;

  for (uint32_t i = 0; i < n; i++) {
    in_vec[i] = (int32_t)phase_u;   // 直接解释为有符号 Q31
    phase_u += step_u;              // uint32 自然回绕，无 UB
  }

  uint32_t t0 = DWT->CYCCNT;
  HAL_CORDIC_Calculate(&hcordic, in_vec, out_vec, n, HAL_MAX_DELAY);
  uint32_t t1 = DWT->CYCCNT;

  // 防止优化
  volatile int32_t sink = out_vec[0] ^ out_vec[1];
  (void)sink;

  return (t1 - t0);
}
static uint32_t bench_soft_vec(uint32_t n)
{
  if (n > CORDIC_MAX_N) n = CORDIC_MAX_N;

  volatile float s = 0.0f, c = 0.0f;

  // 计时区间只做 sinf/cosf（输入生成放外面）
  float a = 0.0f;
  const float step = 0.001f;

  uint32_t t0 = DWT->CYCCNT;
  for (uint32_t i = 0; i < n; i++) {
    s = sinf(a);
    c = cosf(a);
    a += step;
  }
  uint32_t t1 = DWT->CYCCNT;

  (void)s; (void)c;
  return (t1 - t0);
}

void cli_init(void)
{
  dwt_init();
  cordic_sincos_init();
  cordic_reg_init();
  fmac_fir_init();
  LOGI("cli ready: help / tick / bench <n> / cordic* / fmaccmp <n>");
}

void cli_on_rx_byte(uint8_t b)
{
  if (b == '\r' || b == '\n') {
    if (linelen > 0) line_ready = 1;
    return;
  }

  // 退格
  if (b == 0x08 || b == 0x7F) {
    if (linelen > 0) linelen--;
    return;
  }

  /* Ignore non-printable control bytes to keep command buffer clean. */
  if (b < 0x20 || b > 0x7E) {
    return;
  }

  if (linelen < (CLI_LINE_MAX - 1)) {
    line[linelen++] = (char)b;
    line[linelen] = 0;
  } else {
    // 太长就丢弃本行
    linelen = 0;
  }
}

static void exec_cmd(char *cmd)
{
  // 去前导空格
	while (*cmd && isspace((unsigned char)*cmd)) cmd++;

  if (cmd[0] == 0) return;

  if (strcmp(cmd, "help") == 0) {
    LOGI("cmds:");
    LOGI("  help");
    LOGI("  tick");
    LOGI("  bench <n>   (e.g. bench 10000)");
    LOGI("  cordic <n>  (HAL vs soft. e.g. cordic 1000)");
    LOGI("  cordicr <n> (REG vs HAL vs soft. e.g. cordicr 1000)");
    LOGI("  cordicv <n> (vector, one HAL call. e.g. cordicv 5000)");
    LOGI("  cordiccmp <n> (soft vs hw vec. e.g. cordiccmp 5000)");
    LOGI("  cordicbd <n> (REG breakdown: pack/hw/unpack. e.g. cordicbd 1000)");
    LOGI("  fmacdbg    (diagnostic: prints actual soft vs hw values)");
    LOGI("  fmaccmp <n> (e.g. fmaccmp 5000)");
    LOGI("  adcstart    (start ADC+FMAC @ 20kHz)");
    LOGI("  adcstop     (stop ADC)");
    LOGI("  adcstat     (show noise statistics)");
    LOGI("  adcdump <n> (print raw vs filtered, default 32)");
    return;
  }

  if (strcmp(cmd, "adcstart") == 0) {
      fmac_rt_set_active(0U);
      fmac_rt_reset_stats();
      fmac_rt_restart();
      fmac_rt_set_active(1U);
      LOGI("ADC+FMAC capture enabled (source: ADC1_2 IRQ)");
      return;
    }

    if (strcmp(cmd, "adcstop") == 0) {
      fmac_rt_set_active(0U);
      fmac_rt_stats_t s = fmac_rt_get_stats();
      LOGI("ADC+FMAC stopped, %lu samples collected",
           (unsigned long)s.valid_count);
      return;
    }

    if (strcmp(cmd, "adcstat") == 0) {
      fmac_rt_stats_t s = fmac_rt_get_stats();
      if (s.valid_count == 0U) {
        LOGI("no settled samples yet (run adcstart and wait %u samples)",
             (unsigned)FMAC_RT_SETTLE_SAMPLES);
        return;
      }

      int32_t raw_mean  = (int32_t)(s.raw_sum / (int64_t)s.valid_count);
      int32_t filt_mean = (int32_t)(s.filt_sum / (int64_t)s.valid_count);

      /* peak-to-peak 噪声 */
      int32_t raw_pp  = (int32_t)s.raw_max - (int32_t)s.raw_min;
      int32_t filt_pp = (int32_t)s.filt_max - (int32_t)s.filt_min;

      /* RMS 噪声 (简化: 用 variance = E[x^2] - E[x]^2) */
      uint32_t raw_var  = (uint32_t)(s.raw_sq_sum / s.valid_count - (int64_t)raw_mean * raw_mean);
      uint32_t filt_var = (uint32_t)(s.filt_sq_sum / s.valid_count - (int64_t)filt_mean * filt_mean);

      LOGI("── ADC+FMAC stats (valid=%lu total=%lu) ──",
           (unsigned long)s.valid_count,
           (unsigned long)s.count);
      LOGI("         raw        filtered");
      LOGI("  mean:  %6ld      %6ld", (long)raw_mean, (long)filt_mean);
      LOGI("  min:   %6d      %6d", (int)s.raw_min, (int)s.filt_min);
      LOGI("  max:   %6d      %6d", (int)s.raw_max, (int)s.filt_max);
      LOGI("  p-p:   %6ld      %6ld", (long)raw_pp, (long)filt_pp);
      LOGI("  var:   %6lu      %6lu", (unsigned long)raw_var, (unsigned long)filt_var);

      if (filt_pp > 0) {
        LOGI("  noise reduction (p-p): %.1fx", (double)raw_pp / (double)filt_pp);
      }
      if (filt_var > 0) {
        LOGI("  noise reduction (var): %.1fx", (double)raw_var / (double)filt_var);
      }
      return;
    }

    if (strncmp(cmd, "adcdump", 7) == 0 && (cmd[7] == 0 || cmd[7] == ' ')) {
      uint32_t n = 32;
      char *p = cmd + 7;
      while (*p == ' ') p++;
      if (*p) n = (uint32_t)strtoul(p, NULL, 10);
      if (n == 0) n = 32;
      if (n > FMAC_RT_LOG_SIZE) n = FMAC_RT_LOG_SIZE;

      uint32_t end = fmac_rt_log_idx;
      if (end < n) n = end;   /* 还没采够 */

      LOGI("── last %lu samples ──", (unsigned long)n);
      LOGI("  idx     raw    filt    diff");
      for (uint32_t i = 0; i < n; i++) {
        uint32_t idx = (end - n + i) % FMAC_RT_LOG_SIZE;
        int16_t r = fmac_rt_log[idx].raw;
        int16_t f = fmac_rt_log[idx].filt;
        LOGI("  [%3lu] %6d  %6d  %6d", (unsigned long)i,
             (int)r, (int)f, (int)(r - f));
      }
      return;
    }

  if (strcmp(cmd, "tick") == 0) {
    LOGI("tick=%lu", (unsigned long)HAL_GetTick());
    return;
  }
  if (strncmp(cmd, "cordicv", 7) == 0 && (cmd[7] == 0 || cmd[7] == ' ')) {
    uint32_t n = 5000;
    char *p = cmd + 7;          // <-- 正确：跳过 "cordicv"

    while (*p == ' ') p++;      // 跳过空格
    if (*p) n = (uint32_t)strtoul(p, NULL, 10);
    if (n == 0) n = 5000;

    uint32_t hw = bench_cordic_vec(n);
    LOGI("cordicv n=%lu hw=%lu cyc (one-call)",
         (unsigned long)n, (unsigned long)hw);
    return;
  }

  if (strncmp(cmd, "cordicbd", 8) == 0 && (cmd[8] == 0 || isspace((unsigned char)cmd[8]))) {
    uint32_t n = 1000;
    char *p = cmd + 8;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p) n = (uint32_t)strtoul(p, NULL, 10);
    if (n == 0) n = 1000;

    uint32_t pack=0, hw=0, unpack=0;
    bench_cordic_reg_breakdown(n, &pack, &hw, &unpack);

    uint32_t total = pack + hw + unpack;
    LOGI("── cordicbd n=%lu ──", (unsigned long)n);
    LOGI("  pack   (float->q31): %7lu cyc  (%lu/call)", (unsigned long)pack,   (unsigned long)(pack/n));
    LOGI("  hw     (WDATA/RDATA): %7lu cyc  (%lu/call)", (unsigned long)hw,     (unsigned long)(hw/n));
    LOGI("  unpack (q31->float): %7lu cyc  (%lu/call)", (unsigned long)unpack, (unsigned long)(unpack/n));
    LOGI("  total             : %7lu cyc  (%lu/call)", (unsigned long)total,  (unsigned long)(total/n));
    return;
  }

  if (strncmp(cmd, "cordicr", 7) == 0 && (cmd[7] == 0 || cmd[7] == ' ')) {
    uint32_t n = 1000;
    char *p = cmd + 7;
    while (*p == ' ') p++;
    if (*p) n = (uint32_t)strtoul(p, NULL, 10);
    if (n == 0) n = 1000;

    uint32_t soft = bench_soft_sincos(n);
    uint32_t hal  = bench_cordic_sincos(n);
    uint32_t reg  = bench_cordic_sincos_reg(n);
    uint32_t pure = bench_cordic_pure_reg(n);

    LOGI("── cordicr n=%lu ──", (unsigned long)n);
    LOGI("  sinf+cosf  : %7lu cyc  (%lu/call)",
         (unsigned long)soft, (unsigned long)(soft/n));
    LOGI("  CORDIC HAL : %7lu cyc  (%lu/call)",
         (unsigned long)hal, (unsigned long)(hal/n));
    LOGI("  CORDIC REG : %7lu cyc  (%lu/call)",
         (unsigned long)reg, (unsigned long)(reg/n));
    LOGI("  CORDIC PURE: %7lu cyc  (%lu/call)  [no float conv]",
         (unsigned long)pure, (unsigned long)(pure/n));
    LOGI("  speedup REG vs soft : %lu.%02lux",
         (unsigned long)(soft*100/reg/100), (unsigned long)(soft*100/reg%100));
    LOGI("  speedup PURE vs soft: %lu.%02lux",
         (unsigned long)(soft*100/pure/100), (unsigned long)(soft*100/pure%100));
    return;
  }

  if (strncmp(cmd, "cordiccmp", 9) == 0 && (cmd[9] == 0 || cmd[9] == ' ')) {
    uint32_t n = 5000;
    char *p = cmd + 9;
    if (*p) n = (uint32_t)strtoul(p, NULL, 10);
    if (n == 0) n = 5000;   // 防止 n 被解析成0
    while (*p == ' ') p++;
    if (*p) n = (uint32_t)strtoul(p, NULL, 10);
    if (n == 0) n = 5000;

    uint32_t soft = bench_soft_vec(n);
    uint32_t hw   = bench_cordic_vec(n);

    uint32_t sp_x10 = (hw == 0) ? 0 : (soft * 10u / hw);
    LOGI("cordiccmp n=%lu soft=%lu cyc hw=%lu cyc speedup=%lu.%lux",
         (unsigned long)n,
         (unsigned long)soft,
         (unsigned long)hw,
         (unsigned long)(sp_x10/10),
         (unsigned long)(sp_x10%10));
    return;
  }

  if (strncmp(cmd, "cordic", 6) == 0 && (cmd[6] == 0 || cmd[6] == ' ')) {
    uint32_t n = 5000;
    char *p = cmd + 6;
    if (*p) n = (uint32_t)strtoul(p, NULL, 10);
    if (n == 0) n = 5000;   // 防止 n 被解析成0
    while (*p == ' ') p++;
    if (*p) n = (uint32_t)strtoul(p, NULL, 10);

    uint32_t soft = bench_soft_sincos(n);
    uint32_t hw   = bench_cordic_sincos(n);

    // speedup = soft/hw (简单整数比)
    uint32_t sp_x10 = (hw == 0) ? 0 : (soft * 10u / hw);
    LOGI("cordic n=%lu soft=%lu cyc hw=%lu cyc speedup=%lu.%lux",
         (unsigned long)n,
         (unsigned long)soft,
         (unsigned long)hw,
         (unsigned long)(sp_x10/10),
         (unsigned long)(sp_x10%10));
    return;
  }
  if (strncmp(cmd, "fmacdbg", 7) == 0 && (cmd[7] == 0 || cmd[7] == ' ')) {
    /* 诊断命令: 用已知输入打印 soft 和 hw 的实际值 */
    /* FMAC 有 1 个样本的 pipeline 延迟: hw[0] 是 preload 零输出, 真实结果从 hw[1] 开始 */
    uint32_t n = 64;

    /* 简单输入: 全部设为 1000 (已知值, 好算) */
    for (uint32_t i = 0; i < n; i++) g_x[i] = 1000;

    bench_soft_fir(n);
    bench_fmac_fir(n);

    LOGI("── fmacdbg (input=1000 const, 32-tap MA) ──");
    LOGI("  hw[0] is pipeline zero, real data starts at hw[1]");
    LOGI("  idx  soft   hw[i+1]  diff");
    for (uint32_t i = 0; i < 36 && i < n; i++) {
      int16_t d = g_y_hw[i + 1] - g_y_soft[i];
      LOGI("  [%2lu] %6d  %6d  %6d", (unsigned long)i,
           (int)g_y_soft[i], (int)g_y_hw[i + 1], (int)d);
    }

    /* 再测一个: 脉冲输入 */
    memset(g_x, 0, sizeof(int16_t) * n);
    g_x[0] = 16384;  /* 半量程脉冲 */

    bench_soft_fir(n);
    bench_fmac_fir(n);

    LOGI("── fmacdbg (impulse x[0]=16384) ──");
    LOGI("  idx  soft   hw[i+1]  diff");
    for (uint32_t i = 0; i < 36 && i < n; i++) {
      int16_t d = g_y_hw[i + 1] - g_y_soft[i];
      LOGI("  [%2lu] %6d  %6d  %6d", (unsigned long)i,
           (int)g_y_soft[i], (int)g_y_hw[i + 1], (int)d);
    }
    return;
  }

  if (strncmp(cmd, "fmaccmp", 7) == 0 && (cmd[7] == 0 || cmd[7] == ' ')) {
    uint32_t n = 1000;
    char *p = cmd + 7;
    while (*p == ' ') p++;
    if (*p) n = (uint32_t)strtoul(p, NULL, 10);
    if (n == 0) n = 1000;
    if (n > (FMAC_INPUT_N - 1)) n = FMAC_INPUT_N - 1;

    gen_input(n);

    uint32_t soft = bench_soft_fir(n);
    uint32_t hw   = bench_fmac_fir(n);
    uint32_t diff = 0;
    int16_t max_abs = 0;

    /* FMAC pipeline 延迟 1 样本: g_y_hw[1..n] 对应 g_y_soft[0..n-1]
     * 最后 ~FMAC_TAPS 个样本可能卡在 Y buffer 里输出为零, 跳过这些 */
    uint32_t valid = (n > FMAC_TAPS) ? (n - FMAC_TAPS) : 0;
    for (uint32_t i = 0; i < valid; i++) {
      int16_t d = g_y_hw[i + 1] - g_y_soft[i];
      if (d < 0) d = -d;
      if (d != 0) diff++;
      if (d > max_abs) max_abs = d;
    }

    // speedup 用浮点只是打印，不参与滤波
    float sp = (hw == 0 || hw == 0xFFFFFFFFu) ? 0.0f : ((float)soft / (float)hw);

    LOGI("fmaccmp n=%lu soft=%lu cyc hw=%lu cyc speedup=%.2fx",
         (unsigned long)n,
         (unsigned long)soft,
         (unsigned long)hw,
         (double)sp);
    LOGI("  compared=%lu/%lu  diff_count=%lu  max_abs_diff=%d",
         (unsigned long)valid, (unsigned long)n,
         (unsigned long)diff, (int)max_abs);
    return;
  }

  if (strncmp(cmd, "bench", 5) == 0) {
    uint32_t n = 10000;
    char *p = cmd + 5;
    if (*p) n = (uint32_t)strtoul(p, NULL, 10);
    if (n == 0) n = 5000;   // 防止 n 被解析成0

    while (*p == ' ') p++;
    if (*p) n = (uint32_t)strtoul(p, NULL, 10);

    uint32_t cyc = bench_loop_cycles(n);
    LOGI("bench n=%lu cycles=%lu", (unsigned long)n, (unsigned long)cyc);
    return;
  }

  LOGW("unknown: %s", cmd);
}

void cli_poll(void)
{
  if (!line_ready) return;

  __disable_irq();
  line_ready = 0;
  char local[CLI_LINE_MAX];
  strncpy(local, line, sizeof(local));
  local[sizeof(local) - 1] = 0;
  linelen = 0;
  __enable_irq();

  exec_cmd(local);
}
