/*
 * log.h
 *
 *  Created on: 2026年2月5日
 *      Author: SYRLIST
 */

#ifndef LOG_H_
#define LOG_H_
#pragma once
#include <stdarg.h>

void log_init(void);
void log_printf(const char *fmt, ...);

// 先给三个级别，后面再扩展
#define LOGI(...) do { log_printf("[I] " __VA_ARGS__); log_printf("\r\n"); } while(0)
#define LOGW(...) do { log_printf("[W] " __VA_ARGS__); log_printf("\r\n"); } while(0)
#define LOGE(...) do { log_printf("[E] " __VA_ARGS__); log_printf("\r\n"); } while(0)


#endif /* LOG_H_ */
