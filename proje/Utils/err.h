/*
 * err.h
 *
 *  Created on: 2026年1月5日
 *      Author: SYRLIST
 */

#ifndef ERR_H_
#define ERR_H_
#pragma once
#include <stdint.h>

typedef enum {
    ERR_OK = 0,
    ERR_FAIL = 1,
    ERR_TIMEOUT = 2,
    ERR_INVALID_ARG = 3,
    ERR_NO_MEM = 4,
    ERR_BUSY = 5,

    ERR_UART_TX = 100,
    ERR_UART_RX = 101,
} err_t;

const char* err_str(err_t e);



#endif /* ERR_H_ */
