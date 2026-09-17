#include "circular_buffer.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void test_new_buffer_is_empty(void)
{
    circular_buffer_t buffer;

    circular_buffer_init(&buffer);

    assert(circular_buffer_is_empty(&buffer));
    assert(!circular_buffer_is_full(&buffer));
    assert(circular_buffer_count(&buffer) == 0u);

    printf("test_new_buffer_is_empty passed.\n");
}

static void test_push_and_pop(void)
{
    circular_buffer_t buffer;
    uint8_t value = 0u;

    circular_buffer_init(&buffer);

    assert(circular_buffer_push(&buffer, 10u));
    assert(circular_buffer_push(&buffer, 20u));
    assert(circular_buffer_push(&buffer, 30u));
    assert(circular_buffer_count(&buffer) == 3u);

    assert(circular_buffer_pop(&buffer, &value));
    assert(value == 10u);

    assert(circular_buffer_pop(&buffer, &value));
    assert(value == 20u);

    assert(circular_buffer_pop(&buffer, &value));
    assert(value == 30u);

    assert(circular_buffer_is_empty(&buffer));
    assert(circular_buffer_count(&buffer) == 0u);

    printf("test_push_and_pop passed.\n");
}

static void test_full_buffer(void)
{
    circular_buffer_t buffer;
    size_t index;

    circular_buffer_init(&buffer);

    for (index = 0u; index < CIRCULAR_BUFFER_CAPACITY; index++)
    {
        assert(circular_buffer_push(&buffer, (uint8_t)index));
    }

    assert(circular_buffer_is_full(&buffer));
    assert(circular_buffer_count(&buffer) == CIRCULAR_BUFFER_CAPACITY);

    assert(!circular_buffer_push(&buffer, 99u));

    printf("test_full_buffer passed.\n");
}

static void test_wrap_around(void)
{
    circular_buffer_t buffer;
    uint8_t value = 0u;
    size_t index;

    circular_buffer_init(&buffer);

    for (index = 0u; index < CIRCULAR_BUFFER_CAPACITY; index++)
    {
        assert(circular_buffer_push(&buffer, (uint8_t)index));
    }

    for (index = 0u; index < 3u; index++)
    {
        assert(circular_buffer_pop(&buffer, &value));
        assert(value == (uint8_t)index);
    }

    assert(circular_buffer_push(&buffer, 100u));
    assert(circular_buffer_push(&buffer, 101u));
    assert(circular_buffer_push(&buffer, 102u));

    for (index = 3u; index < CIRCULAR_BUFFER_CAPACITY; index++)
    {
        assert(circular_buffer_pop(&buffer, &value));
        assert(value == (uint8_t)index);
    }

    assert(circular_buffer_pop(&buffer, &value));
    assert(value == 100u);

    assert(circular_buffer_pop(&buffer, &value));
    assert(value == 101u);

    assert(circular_buffer_pop(&buffer, &value));
    assert(value == 102u);

    assert(circular_buffer_is_empty(&buffer));

    printf("test_wrap_around passed.\n");
}

static void test_invalid_input(void)
{
    circular_buffer_t buffer;
    uint8_t value = 0u;

    circular_buffer_init(&buffer);

    assert(!circular_buffer_push(NULL, 1u));
    assert(!circular_buffer_pop(NULL, &value));
    assert(!circular_buffer_pop(&buffer, NULL));
    assert(!circular_buffer_pop(&buffer, &value));

    assert(circular_buffer_is_empty(NULL));
    assert(!circular_buffer_is_full(NULL));
    assert(circular_buffer_count(NULL) == 0u);

    circular_buffer_init(NULL);

    printf("test_invalid_input passed.\n");
}

int main(void)
{
    test_new_buffer_is_empty();
    test_push_and_pop();
    test_full_buffer();
    test_wrap_around();
    test_invalid_input();

    printf("All circular buffer tests passed.\n");
    return 0;
}
