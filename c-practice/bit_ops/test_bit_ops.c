#include "bit_ops.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static void test_set_bit(void)
{
    uint32_t value = 0u;

    assert(bit_set_u32(&value, 3u));
    assert(value == 0x00000008u);

    assert(bit_set_u32(&value, 0u));
    assert(value == 0x00000009u);

    printf("test_set_bit passed.\n");
}

static void test_clear_bit(void)
{
    uint32_t value = 0x00000009u;

    assert(bit_clear_u32(&value, 3u));
    assert(value == 0x00000001u);

    assert(bit_clear_u32(&value, 0u));
    assert(value == 0x00000000u);

    printf("test_clear_bit passed.\n");
}

static void test_check_bit(void)
{
    uint32_t value = 0x00000009u;

    assert(bit_is_set_u32(&value, 3u));
    assert(bit_is_set_u32(&value, 0u));
    assert(!bit_is_set_u32(&value, 1u));
    assert(!bit_is_set_u32(&value, 2u));

    printf("test_check_bit passed.\n");
}

static void test_invalid_input(void)
{
    uint32_t value = 0u;

    assert(!bit_set_u32(NULL, 0u));
    assert(!bit_set_u32(&value, 32u));

    assert(!bit_clear_u32(NULL, 0u));
    assert(!bit_clear_u32(&value, 32u));

    assert(!bit_is_set_u32(&value, 32u));

    printf("test_invalid_input passed.\n");
}

static void test_toggle_bit(void)
{
    uint32_t value = 0u;

    assert(bit_toggle_u32(&value, 2u));
    assert(value == 0x00000004u);

    assert(bit_toggle_u32(&value, 2u));
    assert(value == 0x00000000u);

    assert(!bit_toggle_u32(NULL, 2u));
    assert(!bit_toggle_u32(&value, 32u));

    printf("test_toggle_bit passed.\n");
}

int main(void)
{
    test_set_bit();
    test_clear_bit();
    test_check_bit();
    test_invalid_input();
    test_toggle_bit();

    printf("All bit_ops tests passed.\n");
    return 0;
}
