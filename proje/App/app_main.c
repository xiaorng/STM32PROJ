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
    // 心跳：别刷屏（你之前已经改成闪灯就保持）
    uint32_t now = HAL_GetTick();
    if ((now - s_last_beat_ms) >= 1000) {
        s_last_beat_ms += 1000;
        s_uptime_s++;
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    }

    // 串口：一次把所有可读字节读完，防止溢出丢数据
    uint8_t ch;
    while (platform_uart_read_byte(&ch) == 1) {

        if (ch == '\r' || ch == '\n') {
            s_cmd_buf[s_cmd_len] = '\0';
            while (s_cmd_len > 0 && s_cmd_buf[s_cmd_len - 1] == ' ') {
                s_cmd_buf[--s_cmd_len] = '\0';
            }
            handle_cmd(s_cmd_buf);
            s_cmd_len = 0;
            continue;
        }

        // 可选：支持退格
        if (ch == '\b' || ch == 0x7F) {
            if (s_cmd_len > 0) s_cmd_len--;
            continue;
        }

        if (s_cmd_len < sizeof(s_cmd_buf) - 1) {
            s_cmd_buf[s_cmd_len++] = (char)ch;
        } else {
            s_cmd_len = 0;
            LOGW("CMD", "buffer overflow");
        }
    }

    // 别 delay，delay 会让你更容易丢字节
}




