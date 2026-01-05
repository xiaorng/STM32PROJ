/*
 * log.c
 *
 *  Created on: 2026年1月5日
 *      Author: SYRLIST
 */
#include "log.h"
#include "platform_uart.h"
#include <stdio.h>
#include <string.h>

static log_level_t g_log_level = LOG_LVL_INFO;

void log_init(void)
{
    // 预留：后面可加互斥锁、DMA发送等
}

void log_set_level(log_level_t lvl)
{
    g_log_level = lvl;
}

static const char* lvl_str(log_level_t lvl)
{
    switch (lvl) {
    case LOG_LVL_ERROR: return "E";
    case LOG_LVL_WARN:  return "W";
    case LOG_LVL_INFO:  return "I";
    case LOG_LVL_DEBUG: return "D";
    default:            return "?";
    }
}

void log_printf(log_level_t lvl, const char *tag, const char *fmt, ...)
{
    if (lvl > g_log_level) return;
    if (tag == NULL) tag = "TAG";
    if (fmt == NULL) return;

    char buf[256];
    int n = snprintf(buf, sizeof(buf), "[%s][%s] ", lvl_str(lvl), tag);
    if (n < 0) return;

    va_list ap;
    va_start(ap, fmt);
    int m = vsnprintf(buf + n, sizeof(buf) - (size_t)n, fmt, ap);
    va_end(ap);
    if (m < 0) return;

    size_t len = strnlen(buf, sizeof(buf));
    if (len < sizeof(buf) - 2) {
        buf[len++] = '\r';
        buf[len++] = '\n';
        buf[len] = '\0';
    }

    (void)platform_uart_write((const uint8_t*)buf, len);
}


