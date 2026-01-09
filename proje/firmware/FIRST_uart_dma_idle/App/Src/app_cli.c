#include "app_cli.h"
#include "bsp_uart2.h"
#include "app_breathe.h"
#include "cmsis_os2.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include "bsp_flash.h"
static char   g_line[64];
static size_t g_len = 0;
static int    g_inited = 0;

static void cli_write(const char *s)
{
    BSP_UART2_Write((const uint8_t*)s, strlen(s));
}

static void cli_printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) BSP_UART2_Write((const uint8_t*)buf, (size_t)n);
}

static void trim(char *s)
{
    // 去掉末尾空格
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == ' ' || s[n-1] == '\t')) s[--n] = 0;

    // 去掉开头空格：左移
    size_t i = 0;
    while (s[i] == ' ' || s[i] == '\t') i++;
    if (i) memmove(s, s + i, strlen(s + i) + 1);
}

static void cli_help(void)
{
    cli_write(
        "\r\n--- CLI ---\r\n"
        "help\r\n"
        "status\r\n"
        "led on\r\n"
        "led off\r\n"
        "breathe on\r\n"
        "breathe off\r\n"
        "pwm <0..999>\r\n"
        "save\r\n" // 【新增】
    );
}

static void cli_status(void)
{
    cli_printf("breathe=%d duty=%u\r\n",
               (int)App_Breathe_GetEnable(),
               (unsigned)App_Breathe_GetDuty());
}

static void cli_handle_line(char *cmd)
{
    trim(cmd);
    if (cmd[0] == 0) return;

    if (strcmp(cmd, "help") == 0) {
        cli_help();
    }
    else if (strcmp(cmd, "status") == 0) {
        cli_status();
    }
    else if (strcmp(cmd, "led on") == 0) {
            // 原来: App_Breathe_SetEnable(false); ...
            // 现在: 发送 设置亮度 999 命令 (内部会自动关呼吸)
            App_Breathe_SendCmd(CMD_SET_DUTY, 999);
            cli_write("OK\r\n");
        }
        else if (strcmp(cmd, "led off") == 0) {
            App_Breathe_SendCmd(CMD_SET_DUTY, 0);
            cli_write("OK\r\n");
        }
        else if (strcmp(cmd, "breathe on") == 0) {
            App_Breathe_SendCmd(CMD_SET_ENABLE, true);
            cli_write("OK\r\n");
        }
        else if (strcmp(cmd, "breathe off") == 0) {
            App_Breathe_SendCmd(CMD_SET_ENABLE, false);
            cli_write("OK\r\n");
        }
        else if (strncmp(cmd, "pwm ", 4) == 0) {
            int v = atoi(cmd + 4); // 或者你之前换成的 strtol
            if (v < 0) v = 0;
            if (v > 999) v = 999;

            App_Breathe_SendCmd(CMD_SET_DUTY, (uint16_t)v);
            cli_write("OK\r\n");
        }
        else if (strcmp(cmd, "combo") == 0) {
                // 瞬间发送 5 个指令，中间没有延时
                App_Breathe_SendCmd(CMD_SET_DUTY, 100);
                App_Breathe_SendCmd(CMD_SET_DUTY, 300);
                App_Breathe_SendCmd(CMD_SET_DUTY, 600);
                App_Breathe_SendCmd(CMD_SET_DUTY, 900);
                App_Breathe_SendCmd(CMD_SET_DUTY, 0);
                cli_write("Combo sent!\r\n");
            }
}

void App_CliTask(void *argument)
{
    (void)argument;

    uint8_t buf[32];

    if (!g_inited) {
        cli_write("\r\nCLI ready. type 'help'\r\n> ");
        g_inited = 1;
    }

    for (;;)
    {
        size_t n = BSP_UART2_Read(buf, sizeof(buf));
        for (size_t i = 0; i < n; i++)
        {
            char c = (char)buf[i];

            // 回显
            BSP_UART2_Write((uint8_t*)&c, 1);

            if (c == '\r' || c == '\n') {
                if (g_len > 0) {
                    g_line[g_len] = '\0';
                    cli_handle_line(g_line);
                    g_len = 0;
                }
                cli_write("\r\n> ");
            }
            else if (c == '\b' || c == 0x7F) {
                if (g_len > 0) g_len--;
            }
            else {
                if (g_len < sizeof(g_line) - 1) g_line[g_len++] = c;
            }
        }

        osDelay(1);
    }
}
