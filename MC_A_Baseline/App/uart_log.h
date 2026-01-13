#ifndef UART_LOG_H
#define UART_LOG_H
#include "stm32f4xx_hal.h"
#include <stdarg.h>
#include <stdio.h>

extern UART_HandleTypeDef huart2;

void UART_Log_Init(void);
int  uart_printf(const char *fmt, ...);

#endif
