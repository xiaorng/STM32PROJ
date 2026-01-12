
#pragma once
#include "stm32f4xx_hal.h"
#include <stdarg.h>
int  log_printf(const char *fmt, ...);
void log_write(const uint8_t *buf, uint16_t len);
void uart_log_init(UART_HandleTypeDef *huart);
