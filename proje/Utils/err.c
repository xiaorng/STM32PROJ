/*
 * err.c
 *
 *  Created on: 2026年1月5日
 *      Author: SYRLIST
 */
#include "err.h"

const char* err_str(err_t e)
{
    switch (e) {
    case ERR_OK:          return "OK";
    case ERR_FAIL:        return "FAIL";
    case ERR_TIMEOUT:     return "TIMEOUT";
    case ERR_INVALID_ARG: return "INVALID_ARG";
    case ERR_NO_MEM:      return "NO_MEM";
    case ERR_BUSY:        return "BUSY";
    case ERR_UART_TX:     return "UART_TX";
    case ERR_UART_RX:     return "UART_RX";
    default:              return "UNKNOWN";
    }
}


