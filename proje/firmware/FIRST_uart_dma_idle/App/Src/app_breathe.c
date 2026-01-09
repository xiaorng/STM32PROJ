#include "app_breathe.h"
#include "cmsis_os2.h"
#include "bsp_pwm.h"

// 状态变量依然保留，用于保存当前状态供 Flash 读取
static uint16_t s_duty = 0;
static bool     s_en   = true;

// 【新增】定义队列句柄
osMessageQueueId_t g_breathe_queue;

void App_Breathe_Init(uint16_t duty, bool en)
{
    s_duty = duty;
    s_en   = en;
    BSP_PWM_SetDuty(s_duty);
}

// 【新增】发送函数：CLI 调用这个函数把消息塞进队列
void App_Breathe_SendCmd(BreatheCmdType type, uint16_t val)
{
    BreatheMsg_t msg;
    msg.type  = type;
    msg.value = val;
    // 发送消息，超时时间 0 (不等待)
    osMessageQueuePut(g_breathe_queue, &msg, 0, 0);
}

// Get 函数保持不变，给 Flash 用
uint16_t App_Breathe_GetDuty(void)  { return s_duty; }
bool App_Breathe_GetEnable(void)    { return s_en; }

void App_BreatheTask(void *argument)
{
    (void)argument;

    // 1. 【关键】创建消息队列 (深度为 10，每个单元大小是 sizeof(BreatheMsg_t))
    g_breathe_queue = osMessageQueueNew(10, sizeof(BreatheMsg_t), NULL);

    BreatheMsg_t msg;
    int step = 5;
    int dir = 1;

    for (;;)
    {
        // 2. 尝试从队列获取消息
        // 如果 s_en == true (呼吸模式)，我们用 5ms 超时，顺便做延时
        // 如果 s_en == false (手动模式)，我们要一直等待 (osWaitForever)，不占 CPU
        uint32_t timeout = s_en ? 5 : osWaitForever;

        osStatus_t status = osMessageQueueGet(g_breathe_queue, &msg, NULL, timeout);

        if (status == osOK)
        {
            // --- 收到新命令，处理它 ---
            if (msg.type == CMD_SET_ENABLE) {
                s_en = (bool)msg.value;
            }
            else if (msg.type == CMD_SET_DUTY) {
                s_duty = msg.value;
                if (s_duty > 999) s_duty = 999;
                // 手动设了亮度，就更新硬件并自动关呼吸
                s_en = false;
                BSP_PWM_SetDuty(s_duty);
                osDelay(500);
            }
        }

        // --- 呼吸逻辑 ---
        // 只有当接收超时（没新命令）且 s_en 为真时，才执行呼吸
        if (s_en)
        {
            int next = (int)s_duty + dir * step;
            if (next >= 999) { next = 999; dir = -1; }
            if (next <= 0)   { next = 0;   dir =  1; }
            s_duty = (uint16_t)next;
            BSP_PWM_SetDuty(s_duty);
        }
    }
}
