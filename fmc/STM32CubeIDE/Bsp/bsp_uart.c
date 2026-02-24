/*
 * bsp_uart.c
 *
 *  Created on: 2026年2月5日
 *      Author: SYRLIST
 */
#include "bsp_uart.h"
#include "main.h"
#include "stm32g4xx.h"

void bsp_uart_init(void) { }

int bsp_uart_write(const uint8_t *data, size_t len)
{
    if (!data || len == 0) return 0;
    for (size_t i = 0; i < len; i++) {
        while (!(USART2->ISR & USART_ISR_TXE_TXFNF)) {}
        USART2->TDR = data[i];
    }
    return (int)len;
}

void bsp_uart_poll(void) { }
