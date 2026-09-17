#ifndef BIT_OPS_H
#define BIT_OPS_H

#include <stdbool.h>
#include <stdint.h>

/*
 * Set the specific bit to 1.
 *
 * Returns true on success.
 * Returns false if value is NULL or bit is out of range.
*/
bool bit_set_u32(uint32_t *value, uint8_t bit);

/*
 * Clear the specific bit to 0.
 *
 * Returns true on success.
 * Returns false if value is NULL or bit is out of range.
*/
bool bit_clear_u32(uint32_t *value, uint8_t bit);

/*
 * Check whether the specific bit is set to 1.
 *
 * Returns true if the bit is set.
 * Returns false if value is NULL, bit is out of range, or the bit is not set.
*/
bool bit_is_set_u32(uint32_t *value, uint8_t bit);

/*
 * Reverse the specific bit.
 *
 * Returns true on success.
 * Returns false if value is NULL or bit is out of range.
*/
bool bit_toggle_u32(uint32_t *value, uint8_t bit);

#endif 
