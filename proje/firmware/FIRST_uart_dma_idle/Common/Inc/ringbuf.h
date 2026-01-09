/*
 * ringbuf.h
 *
 *  Created on: 2025年12月30日
 *      Author: SYRLIST
 */

#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t *buf;
    size_t   size;          // > 1
    volatile size_t head;   // write
    volatile size_t tail;   // read
} ringbuf_t;

void   ringbuf_init(ringbuf_t *rb, uint8_t *mem, size_t size);
size_t ringbuf_write(ringbuf_t *rb, const uint8_t *data, size_t len);
size_t ringbuf_read(ringbuf_t *rb, uint8_t *out, size_t len);
size_t ringbuf_available(const ringbuf_t *rb);




