/*
 * platform_uart.h
 *
 *  Created on: 2026年1月5日
 *      Author: SYRLIST
 */

#ifndef PLATFORM_UART_H_
#define PLATFORM_UART_H_
#pragma once
#include <stdint.h>
#include <stddef.h>

int      platform_uart_init(void);
int      platform_uart_write(const uint8_t *data, size_t len);
int      platform_uart_read_byte(uint8_t *out);

uint32_t platform_uart_rx_drop_count(void);
uint32_t platform_uart_tx_drop_count(void);





#endif /* PLATFORM_UART_H_ */
