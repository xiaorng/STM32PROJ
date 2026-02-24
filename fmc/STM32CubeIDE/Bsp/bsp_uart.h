/*
 * bsp_uart.h
 *
 *  Created on: 2026年2月5日
 *      Author: SYRLIST
 */

#ifndef BSP_UART_H_
#define BSP_UART_H_
#pragma once
#include <stdint.h>
#include <stddef.h>
void bsp_uart_init(void);
int  bsp_uart_write(const uint8_t *data, size_t len);

// Call periodically from the main loop (non-ISR context)
// to start/continue DMA transfers.
void bsp_uart_poll(void);


#endif /* BSP_UART_H_ */
