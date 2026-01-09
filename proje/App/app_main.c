/*
 * app_main.c
 */
#include "app_main.h"
#include "main.h"
#include "platform_uart.h"
#include "platform_pwm.h"
#include "platform_adc.h"
#include "cli.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef APP_DEBOUNCE_MS
#define APP_DEBOUNCE_MS 30u
#endif

#ifndef APP_BTN_PIN
#define APP_BTN_PIN B1_Pin
#endif

// --------- 如果你在 .ioc 里给板载灯起了 User Label（比如 LD2/LED），这里就会有宏 ---------
// 如果你还没生成 LD2 宏：先去 .ioc 把对应引脚设 GPIO_Output 并填 User Label，然后 Generate Code。
#ifndef LD2_GPIO_Port
// Nucleo-F411RE 常见板载灯是 PA5；如果你不是这个板子/不是这个脚，请改成你实际的宏
#define LD2_GPIO_Port GPIOA
#define LD2_Pin       GPIO_PIN_5
#endif

extern TIM_HandleTypeDef htim2;

// --------- 状态/统计 ---------
static volatile uint8_t  s_btn_irq = 0;
static uint32_t s_btn_last_accept_ms = 0;

static uint32_t s_rx_cnt = 0;
static uint32_t s_last_report_ms = 0;

static uint8_t  s_pwm_idx = 0;
static const uint8_t s_pwm_table[] = {0, 10, 30, 60, 100};

// --------- PWM效果模式 ---------
typedef enum {
    PWM_MODE_MANUAL = 0,
    PWM_MODE_RAMP   = 1,
    PWM_MODE_BREATH = 2,
} pwm_mode_t;

static pwm_mode_t s_pwm_mode = PWM_MODE_MANUAL;

// ramp
static uint8_t  s_ramp_start = 0;
static uint8_t  s_ramp_target = 0;
static uint32_t s_ramp_t0 = 0;
static uint32_t s_ramp_dur = 0;

// breath
static uint32_t s_breath_period_ms = 2000;
static uint8_t  s_breath_min = 0;
static uint8_t  s_breath_max = 100;
static uint32_t s_breath_last_ms = 0;
static int8_t   s_breath_dir = +1;
static int32_t  s_breath_q16 = 0; // duty * 65536

// ---------- CLI commands ----------
static void cmd_help(int argc, char **argv);
static void cmd_status(int argc, char **argv);
static void cmd_led(int argc, char **argv);
static void cmd_pwm(int argc, char **argv);
static void cmd_breath(int argc, char **argv);
static void cmd_temp(int argc, char **argv);

// --------- PWM封装 ---------
static void pwm_apply(uint8_t duty)
{
    if (duty > 100) duty = 100;
    platform_pwm_set_duty(duty);
}

static void pwm_set_manual(uint8_t duty)
{
    s_pwm_mode = PWM_MODE_MANUAL;
    pwm_apply(duty);
}

static void pwm_start_ramp(uint8_t target, uint32_t dur_ms)
{
    if (target > 100) target = 100;
    if (dur_ms < 20) dur_ms = 20;

    s_pwm_mode    = PWM_MODE_RAMP;
    s_ramp_start  = platform_pwm_get_duty();
    s_ramp_target = target;
    s_ramp_t0     = HAL_GetTick();
    s_ramp_dur    = dur_ms;
}

static void pwm_breath_on(void)
{
    s_pwm_mode = PWM_MODE_BREATH;
    s_breath_last_ms = HAL_GetTick();
    s_breath_dir = +1;
    s_breath_q16 = ((int32_t)s_breath_min) << 16;
    pwm_apply(s_breath_min);
}

static void pwm_effect_update(uint32_t now_ms)
{
    if (s_pwm_mode == PWM_MODE_RAMP) {
        uint32_t dt = now_ms - s_ramp_t0;
        if (dt >= s_ramp_dur) {
            pwm_set_manual(s_ramp_target);
            return;
        }
        int32_t a = (int32_t)s_ramp_start;
        int32_t b = (int32_t)s_ramp_target;
        int32_t v = a + (b - a) * (int32_t)dt / (int32_t)s_ramp_dur;
        pwm_apply((uint8_t)v);
        return;
    }

    if (s_pwm_mode == PWM_MODE_BREATH) {
        const uint32_t step_ms = 10;
        if ((now_ms - s_breath_last_ms) < step_ms) return;
        s_breath_last_ms += step_ms;

        uint8_t minv = s_breath_min, maxv = s_breath_max;
        if (maxv <= minv) maxv = minv + 1;

        uint32_t half_ms = s_breath_period_ms / 2;
        if (half_ms < step_ms) half_ms = step_ms;
        uint32_t steps_half = half_ms / step_ms;
        if (steps_half == 0) steps_half = 1;

        int32_t min_q16 = ((int32_t)minv) << 16;
        int32_t max_q16 = ((int32_t)maxv) << 16;
        int32_t step_q16 = (max_q16 - min_q16) / (int32_t)steps_half;
        if (step_q16 <= 0) step_q16 = 1;

        s_breath_q16 += (int32_t)s_breath_dir * step_q16;

        if (s_breath_q16 >= max_q16) { s_breath_q16 = max_q16; s_breath_dir = -1; }
        if (s_breath_q16 <= min_q16) { s_breath_q16 = min_q16; s_breath_dir = +1; }

        pwm_apply((uint8_t)(s_breath_q16 >> 16));
        return;
    }
}

void app_init(void)
{
    // 只做业务装配：PWM attach + 注册命令
    if (platform_pwm_attach(&htim2, TIM_CHANNEL_1) != 0) {
        platform_uart_write((const uint8_t*)"\r\npwm attach failed\r\n",
                            sizeof("\r\npwm attach failed\r\n") - 1);
    }

    (void)cli_register("help",   "show commands",             cmd_help);
    (void)cli_register("status", "show status",               cmd_status);
    (void)cli_register("led",    "led on/off/tog",            cmd_led);
    (void)cli_register("pwm",    "pwm <d> | pwm ramp d ms",   cmd_pwm);
    (void)cli_register("breath", "breath on/off/cfg ...",     cmd_breath);
    (void)cli_register("temp",   "read internal temperature", cmd_temp);

    platform_uart_write((const uint8_t*)"\r\napp init ok, type help\r\n",
                        sizeof("\r\napp init ok, type help\r\n") - 1);
}

void app_loop(void)
{
	platform_uart_rx_poll();

	uint8_t ch;
	while (platform_uart_read_byte(&ch) == 1) {
	    cli_feed_byte(ch);
	}


    // 2) 按键：切换几个固定占空比（会退出呼吸/渐变，回到手动）
    if (s_btn_irq) {
        s_btn_irq = 0;
        uint32_t now = HAL_GetTick();
        if (now - s_btn_last_accept_ms >= APP_DEBOUNCE_MS) {
            s_btn_last_accept_ms = now;
            s_pwm_idx++;
            if (s_pwm_idx >= (sizeof(s_pwm_table) / sizeof(s_pwm_table[0]))) s_pwm_idx = 0;
            pwm_set_manual(s_pwm_table[s_pwm_idx]);
        }
    }

    // 3) 效果更新（ramp/breath）
    pwm_effect_update(HAL_GetTick());

    // 4) 1s 报告（你嫌刷屏就删）
    uint32_t now = HAL_GetTick();
    if (now - s_last_report_ms >= 1000u) {
        s_last_report_ms = now;

        const char *mode =
            (s_pwm_mode == PWM_MODE_MANUAL) ? "man" :
            (s_pwm_mode == PWM_MODE_RAMP)   ? "ramp" : "breath";

        char buf[160];
        int n = snprintf(buf, sizeof(buf),
                         "\r\nrx=%lu rx_drop=%lu duty=%u mode=%s",
                         (unsigned long)s_rx_cnt,
                         (unsigned long)platform_uart_rx_drop_count(),
                         (unsigned)platform_pwm_get_duty(),
                         mode);
        if (n > 0) platform_uart_write((uint8_t*)buf, (size_t)n);
    }

    __WFI();
}

// Nucleo B1 默认走 EXTI
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == APP_BTN_PIN) {
        s_btn_irq = 1;
    }
}

/* ---------------- commands ---------------- */

static void cmd_help(int argc, char **argv)
{
    (void)argc; (void)argv;
    platform_uart_write((const uint8_t*)
                        "\r\nCommands:"
                        "\r\n  help"
                        "\r\n  status"
                        "\r\n  led on|off|tog"
                        "\r\n  pwm <0~100>"
                        "\r\n  pwm ramp <0~100> <ms>"
                        "\r\n  breath on|off"
                        "\r\n  breath cfg <period_ms> <min> <max>"
                        "\r\n  temp\r\n",
                        sizeof("\r\nCommands:"
                               "\r\n  help"
                               "\r\n  status"
                               "\r\n  led on|off|tog"
                               "\r\n  pwm <0~100>"
                               "\r\n  pwm ramp <0~100> <ms>"
                               "\r\n  breath on|off"
                               "\r\n  breath cfg <period_ms> <min> <max>"
                               "\r\n  temp\r\n") - 1);
}

static void cmd_status(int argc, char **argv)
{
    (void)argc; (void)argv;

    const char *mode =
        (s_pwm_mode == PWM_MODE_MANUAL) ? "man" :
        (s_pwm_mode == PWM_MODE_RAMP)   ? "ramp" : "breath";

    char buf[200];
    int n = snprintf(buf, sizeof(buf),
                     "\r\nms=%lu rx=%lu rx_drop=%lu duty=%u mode=%s",
                     (unsigned long)HAL_GetTick(),
                     (unsigned long)s_rx_cnt,
                     (unsigned long)platform_uart_rx_drop_count(),
                     (unsigned)platform_pwm_get_duty(),
                     mode);
    if (n > 0) platform_uart_write((uint8_t*)buf, (size_t)n);
}

static void cmd_led(int argc, char **argv)
{
    if (argc < 2) {
        platform_uart_write((const uint8_t*)"\r\nusage: led on|off|tog\r\n",
                            sizeof("\r\nusage: led on|off|tog\r\n") - 1);
        return;
    }

    if (strcmp(argv[1], "on") == 0) {
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
        platform_uart_write((const uint8_t*)"\r\nled=on\r\n", sizeof("\r\nled=on\r\n") - 1);
        return;
    }

    if (strcmp(argv[1], "off") == 0) {
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
        platform_uart_write((const uint8_t*)"\r\nled=off\r\n", sizeof("\r\nled=off\r\n") - 1);
        return;
    }

    if (strcmp(argv[1], "tog") == 0) {
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        platform_uart_write((const uint8_t*)"\r\nled=tog\r\n", sizeof("\r\nled=tog\r\n") - 1);
        return;
    }

    platform_uart_write((const uint8_t*)"\r\nusage: led on|off|tog\r\n",
                        sizeof("\r\nusage: led on|off|tog\r\n") - 1);
}

static void cmd_pwm(int argc, char **argv)
{
    if (argc < 2) {
        platform_uart_write((const uint8_t*)"\r\nusage: pwm <0~100> | pwm ramp <0~100> <ms>\r\n",
                            sizeof("\r\nusage: pwm <0~100> | pwm ramp <0~100> <ms>\r\n") - 1);
        return;
    }

    if (strcmp(argv[1], "ramp") == 0) {
        if (argc < 4) {
            platform_uart_write((const uint8_t*)"\r\nusage: pwm ramp <0~100> <ms>\r\n",
                                sizeof("\r\nusage: pwm ramp <0~100> <ms>\r\n") - 1);
            return;
        }
        int target = (int)strtol(argv[2], NULL, 10);
        int ms     = (int)strtol(argv[3], NULL, 10);
        if (target < 0) target = 0;
        if (target > 100) target = 100;
        if (ms < 20) ms = 20;

        // ramp 会退出呼吸
        pwm_start_ramp((uint8_t)target, (uint32_t)ms);
        return;
    }

    int duty = (int)strtol(argv[1], NULL, 10);
    if (duty < 0) duty = 0;
    if (duty > 100) duty = 100;

    // 手动设置会退出 ramp/breath
    pwm_set_manual((uint8_t)duty);

    char buf[40];
    int n = snprintf(buf, sizeof(buf), "\r\nduty=%d\r\n", duty);
    if (n > 0) platform_uart_write((uint8_t*)buf, (size_t)n);
}

static void cmd_breath(int argc, char **argv)
{
    if (argc < 2) {
        platform_uart_write((const uint8_t*)"\r\nusage: breath on|off | breath cfg <period_ms> <min> <max>\r\n",
                            sizeof("\r\nusage: breath on|off | breath cfg <period_ms> <min> <max>\r\n") - 1);
        return;
    }

    if (strcmp(argv[1], "on") == 0) {
        pwm_breath_on();
        return;
    }

    if (strcmp(argv[1], "off") == 0) {
        s_pwm_mode = PWM_MODE_MANUAL; // 保持当前 duty
        return;
    }

    if (strcmp(argv[1], "cfg") == 0 && argc >= 5) {
        int period = (int)strtol(argv[2], NULL, 10);
        int minv   = (int)strtol(argv[3], NULL, 10);
        int maxv   = (int)strtol(argv[4], NULL, 10);

        if (period < 200) period = 200;
        if (minv < 0)   minv = 0;
        if (minv > 100) minv = 100;

        if (maxv < 0)   maxv = 0;
        if (maxv > 100) maxv = 100;

        if (maxv <= minv) maxv = minv + 1;

        s_breath_period_ms = (uint32_t)period;
        s_breath_min = (uint8_t)minv;
        s_breath_max = (uint8_t)maxv;

        platform_uart_write((const uint8_t*)"\r\nbreath cfg ok\r\n",
                            sizeof("\r\nbreath cfg ok\r\n") - 1);
        return;
    }

    platform_uart_write((const uint8_t*)"\r\nusage: breath on|off | breath cfg <period_ms> <min> <max>\r\n",
                        sizeof("\r\nusage: breath on|off | breath cfg <period_ms> <min> <max>\r\n") - 1);
}

static void cmd_temp(int argc, char **argv)
{
    (void)argc; (void)argv;

    int16_t t10 = 0;
    if (platform_temp_read_c10(&t10) != 0) {
        platform_uart_write((const uint8_t*)"\r\ntemp read fail\r\n",
                            sizeof("\r\ntemp read fail\r\n") - 1);
        return;
    }

    char buf[64];
    int n = snprintf(buf, sizeof(buf), "\r\ntemp=%d.%d C\r\n",
                     (int)(t10/10), (int)abs(t10%10));
    platform_uart_write((const uint8_t*)buf, (size_t)n);
}
