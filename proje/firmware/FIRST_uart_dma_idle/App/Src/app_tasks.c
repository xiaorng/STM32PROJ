/*
 * app_tasks.c
 *
 *  Created on: 2025年12月30日
 *      Author: SYRLIST
 */
#include "app_tasks.h"
#include "cmsis_os2.h"
#include "app_cli.h"
#include "app_breathe.h"

static osThreadId_t s_cli;
static osThreadId_t s_breathe;

void App_CreateTasks(void)
{
    const osThreadAttr_t cli_attr = {
        .name = "cli",
        .stack_size = 512 * 4,
        .priority = (osPriority_t)osPriorityNormal,
    };

    const osThreadAttr_t br_attr = {
        .name = "breathe",
        .stack_size = 256 * 4,
        .priority = (osPriority_t)osPriorityLow,
    };

    s_cli = osThreadNew(App_CliTask, NULL, &cli_attr);
    s_breathe = osThreadNew(App_BreatheTask, NULL, &br_attr);
}



