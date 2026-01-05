/*
 * app_main.c
 *
 *  Created on: 2026年1月5日
 *      Author: SYRLIST
 */
#include "app_main.h"
#include "log.h"
#include "main.h"
#include "platform_uart.h"

#include <string.h>

static uint32_t s_uptime_s = 0;
static uint32_t s_last_beat_ms = 0;

// 简单命令行缓存
static char s_cmd_buf[64];
static size_t s_cmd_len = 0;

static void handle_cmd(const char *cmd)
{
    if (cmd == NULL || cmd[0] == '\0') return;

    if (strcmp(cmd, "help") == 0) {
        LOGI("HELP", "Commands:");
        LOGI("HELP", "  help");
        LOGI("HELP", "  status");
        return;
    }

    if (strcmp(cmd, "status") == 0) {
        LOGI("STATUS", "uptime=%lu s", (unsigned long)s_uptime_s);
        return;
    }

    LOGW("CMD", "unknown: %s", cmd);
}

void app_init(void)
{
    log_set_level(LOG_LVL_DEBUG);
    LOGI("APP", "app_init done");

    s_last_beat_ms = HAL_GetTick();
}

void app_loop(void)
{
    // 1) 心跳：每 1s 更新 uptime 并打印一次状态
    uint32_t now = HAL_GetTick();
    if ((now - s_last_beat_ms) >= 1000) {
        s_last_beat_ms += 1000;
        s_uptime_s++;
        LOGI("STATUS", "uptime=%lu s", (unsigned long)s_uptime_s);
    }

    // 2) 轮询收串口字符，拼一行命令
    uint8_t ch;
    int r = platform_uart_read_byte(&ch);
    if (r == 1) {
        if (ch == '\r' || ch == '\n') {
            s_cmd_buf[s_cmd_len] = '\0';
            // 简单去掉末尾空格
            while (s_cmd_len > 0 && s_cmd_buf[s_cmd_len - 1] == ' ') {
                s_cmd_buf[--s_cmd_len] = '\0';
            }
            handle_cmd(s_cmd_buf);
            s_cmd_len = 0;
        } else {
            if (s_cmd_len < sizeof(s_cmd_buf) - 1) {
                s_cmd_buf[s_cmd_len++] = (char)ch;
            } else {
                // 缓冲满了，丢弃并报警
                s_cmd_len = 0;
                LOGW("CMD", "buffer overflow");
            }
        }
    }

    // 小延时，避免空转占满 CPU（后面上RTOS会换掉）
    HAL_Delay(1);
}



