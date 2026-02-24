/*
 * cli.h
 *
 *  Created on: 2026年2月5日
 *      Author: SYRLIST
 */

#ifndef CLI_H_
#define CLI_H_
#pragma once
#include <stdint.h>

void cli_init(void);
void cli_poll(void);                 // 主循环里反复调用
void cli_on_rx_byte(uint8_t b);       // 在UART RX回调里调用



#endif /* CLI_H_ */
