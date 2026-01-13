#include "uart_log.h"

void UART_Log_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_2 | GPIO_PIN_3;         // PA2=TX, PA3=RX
    g.Mode = GPIO_MODE_AF_PP;
    g.Pull = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &g);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

// 简单 printf
int uart_printf(const char *fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) {
        if (n > (int)sizeof(buf)) n = sizeof(buf);
        HAL_UART_Transmit(&huart2, (uint8_t*)buf, (uint16_t)n, HAL_MAX_DELAY);
    }
    return n;
}

// newlib _write 重定向（可选）
int _write(int fd, char *ptr, int len) {
    UNUSED(fd);
    HAL_UART_Transmit(&huart2, (uint8_t*)ptr, (uint16_t)len, HAL_MAX_DELAY);
    return len;
}
