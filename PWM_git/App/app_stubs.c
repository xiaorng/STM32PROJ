/*
 * app_stubs.c
 *
 *  Created on: 2026年1月12日
 *      Author: SYRLIST
 */
/*
 * app_stubs.c
 */
#include "main.h"
#include "stm32f4xx_hal.h"

// --- 删除或注释掉下面这部分 ---
/*
void SystemClock_Config(void)
{
    // 保持复位默认时钟
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) { }
}
*/
// --- 删除结束 ---

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file; (void)line;
}
#endif

