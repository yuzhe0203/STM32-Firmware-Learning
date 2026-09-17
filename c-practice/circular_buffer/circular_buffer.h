#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CIRCULAR_BUFFER_CAPACITY 8u

typedef struct {
    uint8_t data[CIRCULAR_BUFFER_CAPACITY];
    size_t head;
    size_t tail;
    size_t count;
} circular_buffer_t;

void circular_buffer_init(circular_buffer_t *buffer);

bool circular_buffer_push(circular_buffer_t *buffer, uint8_t value);

bool circular_buffer_pop(circular_buffer_t *buffer, uint8_t *value);

bool circular_buffer_is_empty(const circular_buffer_t *buffer);

bool circular_buffer_is_full(const circular_buffer_t *buffer);

size_t circular_buffer_count(const circular_buffer_t *buffer);	

#endif
