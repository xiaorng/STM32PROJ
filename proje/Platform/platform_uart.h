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
void     platform_uart_rx_poll(void);              // NEW：DMA环形缓冲搬运
int      platform_uart_write(const uint8_t *data, size_t len);   // return: queued bytes
int      platform_uart_read_byte(uint8_t *out);                  // 1 ok / 0 empty

uint32_t platform_uart_rx_drop_count(void);
uint32_t platform_uart_tx_drop_count(void);








#endif /* PLATFORM_UART_H_ */
