/*
 * bsp_flash.h
 *
 *  Created on: 2026年1月3日
 *      Author: SYRLIST
 */

#ifndef INC_BSP_FLASH_H_
#define INC_BSP_FLASH_H_
#ifndef __BSP_FLASH_H
#define __BSP_FLASH_H

#include "main.h"

// 定义一个结构体，包含所有需要保存的参数
typedef struct {
    uint32_t magic;      // 魔数，例如 0x5AA55AA5，用于判断是否已初始化
    uint16_t pwm_duty;   // 占空比
    uint16_t breathe_en; // 呼吸使能状态
    uint32_t padding;    // 凑齐8字节或16字节对齐（可选）
} SystemParam_t;

// 声明全局参数变量，供 app_breathe.c 使用
extern SystemParam_t g_sys_param;

// 函数声明
void BSP_Flash_Init(void);  // 初始化（读取）
void BSP_Flash_Save(void);  // 保存当前参数到 Flash

#endif


#endif /* INC_BSP_FLASH_H_ */
