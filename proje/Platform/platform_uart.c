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

int platform_uart_read_byte(uint8_t *out)
{
    if (out == NULL) return -1;

    HAL_StatusTypeDef st = HAL_UART_Receive(&huart2, out, 1, 0);
    if (st == HAL_OK) return 1;
    if (st == HAL_TIMEOUT) return 0;

    // 清错误（常见 ORE）
    __HAL_UART_CLEAR_OREFLAG(&huart2);
    __HAL_UART_CLEAR_NEFLAG(&huart2);
    __HAL_UART_CLEAR_FEFLAG(&huart2);
    __HAL_UART_CLEAR_PEFLAG(&huart2);
    return -2;
}



