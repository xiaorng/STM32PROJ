/*
 * ringbuf.c
 *
 *  Created on: 2026年1月7日
 *      Author: SYRLIST
 */
#include "ringbuf.h"

#define RB_MASK (RB_SIZE - 1u)

static inline uint16_t rb_next(uint16_t x)
{
    return (uint16_t)((x + 1u) & RB_MASK);
}

void rb_init(ringbuf_t *rb)
{
    if (!rb) return;
    rb->head = 0;
    rb->tail = 0;
    rb->drop = 0;
}

int rb_write_isr(ringbuf_t *rb, uint8_t b)
{
    if (!rb) return 0;
    uint16_t h = rb->head;
    uint16_t n = rb_next(h);

    if (n == rb->tail) {
        rb->drop++;
        return 0;
    }
    rb->buf[h] = b;
    rb->head = n;
    return 1;
}

int rb_read(ringbuf_t *rb, uint8_t *out)
{
    if (!rb || !out) return 0;
    uint16_t t = rb->tail;
    if (t == rb->head) return 0;

    *out = rb->buf[t];
    rb->tail = rb_next(t);
    return 1;
}

uint32_t rb_drop_count(const ringbuf_t *rb)
{
    return rb ? rb->drop : 0;
}

size_t rb_available(const ringbuf_t *rb)
{
    if (!rb) return 0;
    return (size_t)((rb->head - rb->tail) & RB_MASK);
}
