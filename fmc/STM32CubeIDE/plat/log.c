/*
 * log.c
 *
 *  Created on: 2026年2月5日
 *      Author: SYRLIST
 */
#include "log.h"
#include "bsp_uart.h"
#include <stdio.h>
#include <string.h>


void log_init(void)
{
	bsp_uart_init();
}

void log_printf(const char *fmt, ...)
{
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  if (n <= 0) return;
  if (n >= (int)sizeof(buf)) n = (int)sizeof(buf) - 1;
  bsp_uart_write((const uint8_t*)buf, (size_t)n);
}

