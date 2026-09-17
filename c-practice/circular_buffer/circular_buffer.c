#include "circular_buffer.h"

void circular_buffer_init(circular_buffer_t *buffer){
    if (buffer == NULL)
	    return;
    
    buffer->head = 0u;
    buffer->tail = 0u;
    buffer->count = 0u;
}

bool circular_buffer_push(circular_buffer_t *buffer, uint8_t value){
    if (buffer == NULL || circular_buffer_is_full(buffer))
        return false;

    buffer->data[buffer->head] = value;
    buffer->head = (buffer->head + 1u) % CIRCULAR_BUFFER_CAPACITY;
    buffer->count++;
    return true;
}

bool circular_buffer_pop(circular_buffer_t *buffer, uint8_t *value){
    if (buffer == NULL || value == NULL || circular_buffer_is_empty(buffer))
        return false;

    *value = buffer->data[buffer->tail];
    buffer->tail = (buffer->tail + 1u) % CIRCULAR_BUFFER_CAPACITY;
    buffer->count--;
    return true;
}

bool circular_buffer_is_empty(const circular_buffer_t *buffer){
    return (buffer == NULL || buffer->count == 0u);
}

bool circular_buffer_is_full(const circular_buffer_t *buffer){
    return (buffer != NULL && buffer->count == CIRCULAR_BUFFER_CAPACITY);
}

size_t circular_buffer_count(const circular_buffer_t *buffer){
    return (buffer == NULL) ? 0u : buffer->count;
}
