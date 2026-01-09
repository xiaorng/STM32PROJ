/*
 * bsp_uart2.h
 *
 *  Created on: 2025年12月30日
 *      Author: SYRLIST
 */

#ifndef __BSP_UART2_H
#define __BSP_UART2_H

#include "main.h" // 包含 HAL 库定义

// 定义 DMA 原始接收缓冲区大小 (必须与 stm32f4xx_it.c 中的一致)
#define RX_BUFFER_SIZE  256

// 暴露变量给中断文件使用
extern uint8_t RxBuffer[RX_BUFFER_SIZE];

// 函数声明
void BSP_UART2_Init(void);
size_t BSP_UART2_Read(uint8_t *dst, size_t maxlen);
void BSP_UART2_Write(const uint8_t *src, size_t len);
void BSP_UART2_WriteStr(const char *s);

// 【新增】供中断 ISR 调用的函数，用于把 DMA 数据搬运进 RingBuf
void BSP_UART2_WriteFIFO(uint8_t *data, size_t len);

#endif






