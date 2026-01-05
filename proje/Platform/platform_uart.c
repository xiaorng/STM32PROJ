/*
 * platform_uart.c
 *
 *  Created on: 2026年1月5日
 *      Author: SYRLIST
 */
#include "platform_uart.h"
#include "main.h"
extern UART_HandleTypeDef huart2;
int platform_uart_write(const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0) return -1;

    if (HAL_UART_Transmit(&huart2, (uint8_t*)data, (uint16_t)len, 100) != HAL_OK) {
        return -2;
    }
    return 0;
}


