/*
 * cli.h
 *
 *  Created on: 2026年1月6日
 *      Author: SYRLIST
 */

#ifndef CLI_H_
#define CLI_H_
#pragma once
#include <stdint.h>
#include"platform_uart.h"
typedef void (*cli_cmd_fn_t)(int argc, char **argv);

void cli_init(void);
void cli_feed_byte(uint8_t b);

// 固定命令表（无 malloc）
// 成功返回 0；满了返回 -1；参数非法返回 -2
int  cli_register(const char *name, const char *help, cli_cmd_fn_t fn);




#endif /* CLI_H_ */
