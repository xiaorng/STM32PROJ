/*
 * platform_uart.h
 *
 *  Created on: 2026年1月5日
 *      Author: SYRLIST
 */

#ifndef PLATFORM_UART_H_
#define PLATFORM_UART_H_
#pragma once
#include <stddef.h>
#include <stdint.h>

int platform_uart_write(const uint8_t *data, size_t len);
int platform_uart_read_byte(uint8_t *out);



#endif /* PLATFORM_UART_H_ */
