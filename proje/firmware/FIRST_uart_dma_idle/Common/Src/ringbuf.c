#include "ringbuf.h"

static inline size_t rb_next(size_t idx, size_t size)
{
    idx++;
    if (idx >= size) idx = 0;
    return idx;
}

void ringbuf_init(ringbuf_t *rb, uint8_t *mem, size_t size)
{
    rb->buf  = mem;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
}

size_t ringbuf_available(const ringbuf_t *rb)
{
    size_t h = rb->head;
    size_t t = rb->tail;
    if (h >= t) return h - t;
    return (rb->size - t) + h;
}

size_t ringbuf_write(ringbuf_t *rb, const uint8_t *data, size_t len)
{
    size_t written = 0;
    while (written < len) {
        size_t next = rb_next(rb->head, rb->size);
        if (next == rb->tail) break; // full
        rb->buf[rb->head] = data[written++];
        rb->head = next;
    }
    return written;
}

size_t ringbuf_read(ringbuf_t *rb, uint8_t *out, size_t len)
{
    size_t read = 0;
    while (read < len) {
        if (rb->tail == rb->head) break; // empty
        out[read++] = rb->buf[rb->tail];
        rb->tail = rb_next(rb->tail, rb->size);
    }
    return read;
}
