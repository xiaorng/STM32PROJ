/*
 * app_breathe.h
 *
 *  Created on: 2025年12月30日
 *      Author: SYRLIST
 */

#ifndef __APP_BREATHE_H
#define __APP_BREATHE_H

#include <stdint.h>
#include <stdbool.h>

// 【新增】定义消息类型
typedef enum {
    CMD_SET_DUTY,   // 设置亮度
    CMD_SET_ENABLE  // 开关呼吸
} BreatheCmdType;

// 【新增】定义消息结构体
typedef struct {
    BreatheCmdType type; // 命令类型
    uint16_t value;      // 数值 (亮度值 或 0/1)
} BreatheMsg_t;

// 函数声明
void App_Breathe_Init(uint16_t duty, bool en);
void App_BreatheTask(void *argument);

// 【新增】发送命令的函数接口
void App_Breathe_SendCmd(BreatheCmdType type, uint16_t val);

// 为了 Flash 保存，Get 接口保留
uint16_t App_Breathe_GetDuty(void);
bool App_Breathe_GetEnable(void);

#endif
