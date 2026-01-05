/*
 * app_main.c
 *
 *  Created on: 2026年1月5日
 *      Author: SYRLIST
 */
#include "app_main.h"
#include "log.h"
#include "main.h" // 为了 HAL_Delay

void app_init(void)
{
    log_set_level(LOG_LVL_DEBUG);
    LOGI("APP", "app_init done");
}

void app_loop(void)
{
    LOGD("APP", "tick");
    HAL_Delay(1000);
}


