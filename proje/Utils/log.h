/*
 * log.h
 *
 *  Created on: 2026年1月5日
 *      Author: SYRLIST
 */

#ifndef LOG_H_
#define LOG_H_
#pragma once
#include <stdarg.h>

typedef enum {
    LOG_LVL_ERROR = 0,
    LOG_LVL_WARN  = 1,
    LOG_LVL_INFO  = 2,
    LOG_LVL_DEBUG = 3,
} log_level_t;

void log_init(void);
void log_set_level(log_level_t lvl);
void log_printf(log_level_t lvl, const char *tag, const char *fmt, ...);

#define LOGE(tag, fmt, ...) log_printf(LOG_LVL_ERROR, tag, fmt, ##__VA_ARGS__)
#define LOGW(tag, fmt, ...) log_printf(LOG_LVL_WARN,  tag, fmt, ##__VA_ARGS__)
#define LOGI(tag, fmt, ...) log_printf(LOG_LVL_INFO,  tag, fmt, ##__VA_ARGS__)
#define LOGD(tag, fmt, ...) log_printf(LOG_LVL_DEBUG, tag, fmt, ##__VA_ARGS__)



#endif /* LOG_H_ */
