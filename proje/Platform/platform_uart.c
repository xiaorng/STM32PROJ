#include "platform_uart.h"
#include "main.h"
#include "stm32f4xx_hal.h"

extern UART_HandleTypeDef huart2;

/* ====================== 简易 ring buffer（内部用，不依赖 Utils/ringbuf） ====================== */
typedef struct {
    uint8_t *buf;
    uint16_t cap;    // 必须 2^n
    uint16_t mask;   // cap-1
    volatile uint16_t head; // 写入位置
    volatile uint16_t tail; // 读出位置
} rb_t;

static void rb_init(rb_t *rb, uint8_t *mem, uint16_t cap)
{
    rb->buf = mem;
    rb->cap = cap;
    rb->mask = (uint16_t)(cap - 1);
    rb->head = rb->tail = 0;
}

static int rb_write_byte(rb_t *rb, uint8_t b) // 1 ok / 0 full
{
    uint16_t h = rb->head;
    uint16_t n = (uint16_t)((h + 1) & rb->mask);
    if (n == rb->tail) return 0; // full (留一格)
    rb->buf[h] = b;
    rb->head = n;
    return 1;
}

static int rb_read_byte(rb_t *rb, uint8_t *out) // 1 ok / 0 empty
{
    uint16_t t = rb->tail;
    if (t == rb->head) return 0; // empty
    *out = rb->buf[t];
    rb->tail = (uint16_t)((t + 1) & rb->mask);
    return 1;
}

/* ====================== RX: DMA circular -> rx_rb ====================== */
#define UART_RX_DMA_SZ   256u
#define UART_RX_RB_SZ    1024u   // 2^n

static uint8_t  s_rx_dma[UART_RX_DMA_SZ];
static uint16_t s_rx_last = 0;

static uint8_t  s_rx_rb_mem[UART_RX_RB_SZ];
static rb_t     s_rx_rb;

static uint32_t s_rx_drop = 0;

static inline uint16_t rx_dma_pos(void)
{
    return (uint16_t)(UART_RX_DMA_SZ - __HAL_DMA_GET_COUNTER(huart2.hdmarx));
}

void platform_uart_rx_poll(void)
{
    uint16_t pos = rx_dma_pos();

    while (pos != s_rx_last) {
        uint8_t b = s_rx_dma[s_rx_last];
        s_rx_last++;
        if (s_rx_last >= UART_RX_DMA_SZ) s_rx_last = 0;

        if (!rb_write_byte(&s_rx_rb, b)) {
            s_rx_drop++;
        }
    }
}

int platform_uart_read_byte(uint8_t *out)
{
    if (!out) return 0;
    return rb_read_byte(&s_rx_rb, out);
}

uint32_t platform_uart_rx_drop_count(void)
{
    return s_rx_drop;
}

/* ====================== TX: tx_rb -> IT chunk ====================== */
#define UART_TX_RB_SZ    1024u   // 2^n
#define UART_TX_CHUNK    64u

static uint8_t  s_tx_rb_mem[UART_TX_RB_SZ];
static rb_t     s_tx_rb;

static uint8_t          s_tx_chunk[UART_TX_CHUNK];
static volatile uint8_t s_tx_busy = 0;
static uint32_t         s_tx_drop = 0;

uint32_t platform_uart_tx_drop_count(void)
{
    return s_tx_drop;
}

static void uart_clear_ore(UART_HandleTypeDef *huart)
{
    if (__HAL_UART_GET_FLAG(huart, UART_FLAG_ORE)) {
        __HAL_UART_CLEAR_OREFLAG(huart);
    }
}

static void tx_kick(void)
{
    if (s_tx_busy) return;

    uint16_t n = 0;
    while (n < UART_TX_CHUNK) {
        uint8_t b;
        if (!rb_read_byte(&s_tx_rb, &b)) break;
        s_tx_chunk[n++] = b;
    }
    if (n == 0) return;

    s_tx_busy = 1;
    if (HAL_UART_Transmit_IT(&huart2, s_tx_chunk, n) != HAL_OK) {
        s_tx_busy = 0;
    }
}

int platform_uart_write(const uint8_t *data, size_t len)
{
    if (!data || len == 0) return 0;

    int w = 0;

    __disable_irq(); // TX rb 和 TxCpltCallback 并发，写入时关中断最省事
    for (size_t i = 0; i < len; i++) {
        if (!rb_write_byte(&s_tx_rb, data[i])) {
            s_tx_drop++;
            break;
        }
        w++;
    }
    __enable_irq();

    tx_kick();
    return w;
}

/* ====================== init + HAL callbacks ====================== */

int platform_uart_init(void)
{
    rb_init(&s_rx_rb, s_rx_rb_mem, UART_RX_RB_SZ);
    rb_init(&s_tx_rb, s_tx_rb_mem, UART_TX_RB_SZ);

    if (HAL_UART_Receive_DMA(&huart2, s_rx_dma, UART_RX_DMA_SZ) != HAL_OK) {
        return -1;
    }

    // 我们走 poll 搬运，不靠 HT/TC 中断
    __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);
    __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_TC);

    s_rx_last = rx_dma_pos();
    return 0;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        s_tx_busy = 0;
        tx_kick();
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        uart_clear_ore(&huart2);
        // 如遇到 RX DMA 停掉，再加重启（一般不需要）
        // HAL_UART_DMAStop(&huart2);
        // HAL_UART_Receive_DMA(&huart2, s_rx_dma, UART_RX_DMA_SZ);
    }
}


