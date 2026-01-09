#include "bsp_uart2.h"
#include "main.h"
#include "ringbuf.h"
#include <string.h>
#include <stdio.h>
extern UART_HandleTypeDef huart2;

// 1. 定义 DMA 原始接收缓冲区 (Linear Buffer)
// 这个数组会被 DMA 直接写入，必须是全局变量以便中断访问
uint8_t RxBuffer[RX_BUFFER_SIZE];

// 2. 定义软件环形缓冲区 (Ring Buffer)
// 只有 BSP 内部使用，用于缓存解析任务读取的数据
#define UART2_RB_SIZE  512
static ringbuf_t s_rb;
static uint8_t   s_rb_mem[UART2_RB_SIZE];

void BSP_UART2_Init(void)
{
    // 初始化软件 RingBuf
	 ringbuf_init(&s_rb, s_rb_mem, UART2_RB_SIZE);

	    // 尝试启动 DMA
	    HAL_StatusTypeDef status = HAL_UART_Receive_DMA(&huart2, RxBuffer, RX_BUFFER_SIZE);

	    if (status != HAL_OK)
	    {
	        // 如果这里进来了，说明 DMA 启动失败！
	        // 可以在这里打断点，或者让 LED 狂闪
	        while(1) {
	        	HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5); // 假设 LD2 是你的 LED
	            HAL_Delay(100);
	        }
	    }
}

// 供主任务调用：从 RingBuf 取数据
size_t BSP_UART2_Read(uint8_t *dst, size_t maxlen)
{
    return ringbuf_read(&s_rb, dst, maxlen);
}

// 供 IDLE 中断调用：把 DMA 数据塞入 RingBuf
void BSP_UART2_WriteFIFO(uint8_t *data, size_t len)
{
    ringbuf_write(&s_rb, data, len);
}

// 发送函数保持不变
void BSP_UART2_Write(const uint8_t *src, size_t len)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)src, (uint16_t)len, 1000);
}

void BSP_UART2_WriteStr(const char *s)
{
    BSP_UART2_Write((const uint8_t*)s, strlen(s));
}

// 【删除】 不再需要 HAL_UART_RxCpltCallback，因为我们不用中断接收了
