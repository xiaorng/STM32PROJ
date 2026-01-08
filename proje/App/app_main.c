/*
 * app_main.c
 */

#include "app_main.h"
#include "main.h"
#include "platform_uart.h"
#include "platform_pwm.h"
#include "cli.h"
#include"ringbuf.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef APP_DEBOUNCE_MS
#define APP_DEBOUNCE_MS 30u
#endif

#ifndef APP_BTN_PIN
#define APP_BTN_PIN B1_Pin
#endif

// 你这里用的是 TIM2 CH1（按你写的保持）
extern TIM_HandleTypeDef htim2;

static volatile uint8_t  s_btn_irq = 0;
static volatile uint32_t s_btn_irq_ms = 0;
static uint32_t s_btn_last_accept_ms = 0;
//static ringbuf_t s_tx_rb;
//static uint8_t   s_tx_chunk[128];      // 每次IT发送的分段大小，可调 64/128
static uint32_t s_rx_cnt = 0;
static uint32_t s_last_report_ms = 0;
static uint8_t s_pwm_idx = 0;
static const uint8_t s_pwm_table[] = {0, 10, 30, 60, 100};

// ---------- CLI commands ----------
static void cmd_help(int argc, char **argv);
static void cmd_status(int argc, char **argv);
static void cmd_led(int argc, char **argv);
static void cmd_pwm(int argc, char **argv);

void app_init(void)
{
    // PWM 绑定（启动 PWM 输出）
    if (platform_pwm_attach(&htim2, TIM_CHANNEL_1) != 0) {
        platform_uart_write((const uint8_t*)"\r\npwm attach failed",
                            sizeof("\r\npwm attach failed") - 1);
    } else {
        // 可选：上电默认 0%
        // platform_pwm_set_duty(0);
    }

    // 注册命令（cli_init 必须在 main.c 先调用过）
    (void)cli_register("help",   "show commands",   cmd_help);
    (void)cli_register("status", "show status",     cmd_status);
    (void)cli_register("led",    "led on/off/tog",  cmd_led);
    (void)cli_register("pwm",    "pwm 0~100",       cmd_pwm);

    platform_uart_write((const uint8_t*)"\r\napp init ok, type help\r\n",
                        sizeof("\r\napp init ok, type help\r\n") - 1);
}

void app_loop(void)
{
    // 把 ringbuf 里的字节喂给 CLI
    uint8_t ch;
    while (platform_uart_read_byte(&ch) == 1) {
        s_rx_cnt++;
        cli_feed_byte(ch);
    }

    // 按键去抖：IRQ 只置位，前台判定
    if (s_btn_irq) {
        s_btn_irq = 0;
        uint32_t now = HAL_GetTick();
        if (now - s_btn_last_accept_ms >= APP_DEBOUNCE_MS) {
            s_btn_last_accept_ms = now;
            s_pwm_idx++;
            if (s_pwm_idx >= (sizeof(s_pwm_table) / sizeof(s_pwm_table[0]))) {
                s_pwm_idx = 0;
            }
            platform_pwm_set_duty(s_pwm_table[s_pwm_idx]);

        }
    }

    // 1s 报告一次（验证不断流/丢包）
    uint32_t now = HAL_GetTick();
    if (now - s_last_report_ms >= 1000u) {
        s_last_report_ms = now;
        char buf[128];
        int n = snprintf(buf, sizeof(buf),
                         "\r\nrx=%lu rx_drop=%lu tx_drop=%lu duty=%u",
                         (unsigned long)s_rx_cnt,
                         (unsigned long)platform_uart_rx_drop_count(),
                         (unsigned long)platform_uart_tx_drop_count(),
                         (unsigned)platform_pwm_get_duty());
        if (n > 0) platform_uart_write((uint8_t*)buf, (size_t)n);
    }
    __WFI();
}

// Nucleo B1 默认走 EXTI，HAL 会回调到这里
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == APP_BTN_PIN) {
        s_btn_irq = 1;
        s_btn_irq_ms = HAL_GetTick();
        (void)s_btn_irq_ms;
    }
}

static void cmd_help(int argc, char **argv)
{
    (void)argc; (void)argv;
    platform_uart_write((const uint8_t*)"\r\nhelp/status/led/pwm", 21);
}

static void cmd_status(int argc, char **argv)
{
    (void)argc; (void)argv;
    char buf[128];
    int n = snprintf(buf, sizeof(buf),
                     "\r\nms=%lu rx=%lu rx_drop=%lu tx_drop=%lu duty=%u",
                     (unsigned long)HAL_GetTick(),
                     (unsigned long)s_rx_cnt,
                     (unsigned long)platform_uart_rx_drop_count(),
                     (unsigned long)platform_uart_tx_drop_count(),
                     (unsigned)platform_pwm_get_duty());

    if (n > 0) platform_uart_write((uint8_t*)buf, (size_t)n);
}

static void cmd_led(int argc, char **argv)
{
    (void)argc; (void)argv;
    platform_uart_write((const uint8_t*)"\r\nLD2 is controlled by PWM now. Use: pwm 0~100\r\n", 55);
}
static void cmd_pwm(int argc, char **argv)
{
    if (argc < 2) {
        platform_uart_write((const uint8_t*)"\r\nusage: pwm 0~100\r\n", 21);
        return;
    }

    int duty = (int)strtol(argv[1], NULL, 10);
    if (duty < 0) duty = 0;
    if (duty > 100) duty = 100;
    platform_pwm_set_duty((uint8_t)duty);



    char buf[32];
    int n = snprintf(buf, sizeof(buf), "\r\nduty=%d\r\n", duty);
    if (n > 0) platform_uart_write((uint8_t*)buf, (size_t)n);
}
