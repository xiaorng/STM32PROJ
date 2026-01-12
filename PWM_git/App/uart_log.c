
#include "uart_log.h"
#include <stdio.h>

static UART_HandleTypeDef *g_uart = NULL;

void uart_log_init(UART_HandleTypeDef *huart){ g_uart = huart; }

int log_printf(const char *fmt, ...){
    if(!g_uart) return 0;
    char buf[128];
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if(n < 0) return n;
    if(n > (int)sizeof(buf)) n = sizeof(buf);
    HAL_UART_Transmit(g_uart, (uint8_t*)buf, (uint16_t)n, 10);
    return n;
}

void log_write(const uint8_t *buf, uint16_t len){
    if(!g_uart) return;
    HAL_UART_Transmit(g_uart, (uint8_t*)buf, len, 10);
}
