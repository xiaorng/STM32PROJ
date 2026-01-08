/*
 * platform_uart.c
 *
 *  Created on: 2026年1月5日
 *      Author: SYRLIST
 */
#include "platform_uart.h"
#include "main.h"
#include "ringbuf.h"

extern UART_HandleTypeDef huart2;

/* ---------------- RX ---------------- */
static ringbuf_t s_rx_rb;
static uint8_t   s_rx_byte;

/* ---------------- TX (non-blocking) ---------------- */
static ringbuf_t        s_tx_rb;
static uint8_t          s_tx_chunk[128];      // 可改 64/128
static volatile uint8_t s_tx_busy = 0;        // 0=idle 1=busy

static void uart_clear_ore(UART_HandleTypeDef *huart)
{
    volatile uint32_t tmp;
    tmp = huart->Instance->SR;
    tmp = huart->Instance->DR;
    (void)tmp;
}

static void uart_tx_kick(void)
{
    if (s_tx_busy) return;
    if (rb_available(&s_tx_rb) == 0) return;

    size_t n = 0;
    while (n < sizeof(s_tx_chunk)) {
        uint8_t b;
        if (!rb_read(&s_tx_rb, &b)) break;
        s_tx_chunk[n++] = b;
    }
    if (n == 0) return;

    s_tx_busy = 1;
    if (HAL_UART_Transmit_IT(&huart2, s_tx_chunk, (uint16_t)n) != HAL_OK) {
        s_tx_busy = 0;
    }
}

int platform_uart_init(void)
{
    rb_init(&s_rx_rb);
    rb_init(&s_tx_rb);
    s_tx_busy = 0;

    if (HAL_UART_Receive_IT(&huart2, &s_rx_byte, 1) != HAL_OK) {
        return -1;
    }
    return 0;
}

int platform_uart_write(const uint8_t *data, size_t len)
{
    if (!data || len == 0) return -1;

    size_t ok = 0;
    for (size_t i = 0; i < len; i++) {
        ok += (rb_write_isr(&s_tx_rb, data[i]) ? 1u : 0u);
    }

    uart_tx_kick();                 // 点火（非阻塞）
    return (ok == len) ? 0 : -2;    // -2：TX 缓冲满发生丢字节
}

int platform_uart_read_byte(uint8_t *out)
{
    if (!out) return -1;
    return rb_read(&s_rx_rb, out) ? 1 : 0;
}

uint32_t platform_uart_rx_drop_count(void)
{
    return rb_drop_count(&s_rx_rb);
}

uint32_t platform_uart_tx_drop_count(void)
{
    return rb_drop_count(&s_tx_rb);
}

/* ---------------- HAL callbacks ---------------- */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        s_tx_busy = 0;
        uart_tx_kick();  // 自动续发下一段
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        (void)rb_write_isr(&s_rx_rb, s_rx_byte);
        (void)HAL_UART_Receive_IT(&huart2, &s_rx_byte, 1);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        uart_clear_ore(&huart2);
        (void)HAL_UART_Receive_IT(&huart2, &s_rx_byte, 1);
    }
}
