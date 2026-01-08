/*
 * ringbuf.h
 *
 *  Created on: 2026年1月7日
 *      Author: SYRLIST
 */

#ifndef RINGBUF_H_
#define RINGBUF_H_
#pragma once
#include <stdint.h>
#include <stddef.h>

#ifndef RB_SIZE
#define RB_SIZE 256u   // 必须是2的幂：128/256/512...
#endif

typedef struct {
    volatile uint16_t head;   // ISR 写
    volatile uint16_t tail;   // 前台 读
    volatile uint32_t drop;   // 满了丢包计数
    uint8_t buf[RB_SIZE];
} ringbuf_t;

void rb_init(ringbuf_t *rb);

// ISR: 写 1 字节，成功返回 1；满了返回 0，并累加 drop
int  rb_write_isr(ringbuf_t *rb, uint8_t b);

// 前台: 读 1 字节，成功返回 1；无数据返回 0
int  rb_read(ringbuf_t *rb, uint8_t *out);

uint32_t rb_drop_count(const ringbuf_t *rb);
size_t   rb_available(const ringbuf_t *rb);



#endif /* RINGBUF_H_ */
