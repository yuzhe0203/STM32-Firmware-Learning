#include "bit_ops.h"

bool bit_set_u32(uint32_t *value, uint8_t bit){
    if (value == NULL || bit > 31)
        return false;

    *value |= (1u << bit);
    return true;
}

bool bit_clear_u32(uint32_t *value, uint8_t bit){
    if (value == NULL || bit > 31)
        return false;

    *value &= ~(1u << bit);
    return true;
}

bool bit_is_set_u32(uint32_t *value, uint8_t bit){
    if (value == NULL || bit > 31)
        return false;

    return (*value & (1u << bit)) != 0u;
}

bool bit_toggle_u32(uint32_t *value, uint8_t bit){
    if (value == NULL || bit > 31)
	return false;
    
    *value ^= (1u << bit);
    return true;
}
