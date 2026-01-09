/*
 * bsp_flash.c
 *
 *  Created on: 2026年1月3日
 *      Author: SYRLIST
 */
#include "bsp_flash.h"
#include "app_breathe.h" // 为了获取当前的设置
#include <string.h>      // for memcpy

// 定义 Flash 存储地址：这里使用 Sector 7 的起始地址
// 对于 STM32F401RE / F411RE (512KB)，Sector 7 起始于 0x08060000
// 请根据你的具体芯片手册确认！(如果是 1MB 的 F407，可以用 Sector 11)
#define FLASH_SAVE_ADDR   0x08060000
#define FLASH_SECTOR_NUM  FLASH_SECTOR_7

#define PARAM_MAGIC       0x5AA51234  // 自定义的魔数

SystemParam_t g_sys_param;

// 从 Flash 读取参数
void BSP_Flash_Init(void)
{
    // 直接指针强转读取
    SystemParam_t *p_flash = (SystemParam_t *)FLASH_SAVE_ADDR;

    if (p_flash->magic == PARAM_MAGIC)
    {
        // 只有魔数匹配，才认为是有效数据
        memcpy(&g_sys_param, p_flash, sizeof(SystemParam_t));
    }
    else
    {
        // 第一次上电，或者 Flash 被擦除过
        // 加载默认值
        g_sys_param.magic      = PARAM_MAGIC;
        g_sys_param.pwm_duty   = 0;
        g_sys_param.breathe_en = 1; // 默认开启呼吸
    }
}

// 保存当前参数到 Flash
void BSP_Flash_Save(void)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError;

    // 1. 获取当前系统最新状态到结构体
    g_sys_param.magic      = PARAM_MAGIC;
    g_sys_param.pwm_duty   = App_Breathe_GetDuty();
    g_sys_param.breathe_en = (uint16_t)App_Breathe_GetEnable();

    // 2. 解锁 Flash
    HAL_FLASH_Unlock();

    // 3. 擦除扇区 (必须先擦后写)
    EraseInitStruct.TypeErase    = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3; // 2.7V - 3.6V
    EraseInitStruct.Sector       = FLASH_SECTOR_NUM;
    EraseInitStruct.NbSectors    = 1;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
    {
        // 擦除失败处理（可以加打印）
        HAL_FLASH_Lock();
        return;
    }

    // 4. 写入数据 (每次写 4 字节 / Word)
    uint32_t *p_source = (uint32_t *)&g_sys_param;
    uint32_t words_to_write = sizeof(SystemParam_t) / 4;

    // 如果结构体大小不是4的倍数，需向上取整
    if (sizeof(SystemParam_t) % 4 != 0) words_to_write++;

    uint32_t addr = FLASH_SAVE_ADDR;

    for (uint32_t i = 0; i < words_to_write; i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, p_source[i]) != HAL_OK)
        {
            // 写入失败
            break;
        }
        addr += 4;
    }

    // 5. 上锁
    HAL_FLASH_Lock();
}

