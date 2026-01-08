/*
 * app_main.h
 *
 *  Created on: 2026年1月5日
 *      Author: SYRLIST
 */
#include <stdint.h>
#ifndef APP_MAIN_H_
#define APP_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

void app_init(void);
void app_loop(void);
uint32_t platform_uart_tx_drop_count(void);
void     platform_uart_tx_poll(void);
#ifdef __cplusplus
}
#endif

#endif /* APP_MAIN_H_ */
